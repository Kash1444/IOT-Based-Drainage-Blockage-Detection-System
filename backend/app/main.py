"""DrainGuard API.

The API deliberately distinguishes missing telemetry from a measured zero.  A
model is never used until a complete, validated model artifact is available.
"""
from __future__ import annotations
import asyncio, csv, io, json, math, os, sqlite3
from contextlib import asynccontextmanager
from datetime import datetime, timezone
from pathlib import Path
from typing import Optional
from fastapi import FastAPI, HTTPException, Query
from fastapi.middleware.cors import CORSMiddleware
from fastapi.responses import StreamingResponse
from pydantic import BaseModel, Field
from .blynk import BlynkAdapter
from dotenv import load_dotenv

load_dotenv()

DB_PATH = Path(os.getenv("DRAINGUARD_DB", Path(__file__).resolve().parents[1] / "drainguard.db"))
CONFIGURED_MODE = os.getenv("DRAINGUARD_MODE", "auto").lower()
DEMO_MODE = CONFIGURED_MODE == "demo"
STALE_AFTER_SECONDS = int(os.getenv("DRAINGUARD_STALE_SECONDS", "120"))
MODEL_PATH = Path(os.getenv("DRAINGUARD_MODEL_PATH", Path(__file__).resolve().parents[1] / "models" / "model.joblib"))
DEFAULT_POLL_SECONDS = "10" if os.getenv("BLYNK_TOKEN", "").strip() and CONFIGURED_MODE != "demo" else "0"
POLL_SECONDS = max(0, int(os.getenv("DRAINGUARD_POLL_SECONDS", DEFAULT_POLL_SECONDS)))
_latest = None
_poll_task = None
_poll_state = {"connected": False, "last_attempt": None, "last_success": None, "error": None}
EXPERIMENT_LABELS = {"NORMAL", "PARTIAL_BLOCKAGE", "SEVERE_BLOCKAGE", "RISING_WATER", "SENSOR_FAULT", "NOISY_SENSOR", "RECOVERY"}

def db():
    con = sqlite3.connect(DB_PATH)
    con.row_factory = sqlite3.Row
    con.execute("""CREATE TABLE IF NOT EXISTS readings (
      id INTEGER PRIMARY KEY, timestamp TEXT NOT NULL, flow1 REAL, flow2 REAL,
      water_level REAL, pump_status TEXT, full_level_count INTEGER DEFAULT 0,
      source TEXT NOT NULL, flow_difference REAL, flow_ratio REAL,
      health_json TEXT, experiment_id INTEGER, blynk_json TEXT)""")
    con.execute("""CREATE TABLE IF NOT EXISTS experiments (
      id INTEGER PRIMARY KEY, label TEXT NOT NULL, notes TEXT DEFAULT '',
      started_at TEXT NOT NULL, stopped_at TEXT, status TEXT NOT NULL DEFAULT 'running')""")
    con.execute("""CREATE TABLE IF NOT EXISTS experiment_samples (
      id INTEGER PRIMARY KEY, experiment_id INTEGER NOT NULL, reading_id INTEGER,
      timestamp TEXT NOT NULL, label TEXT NOT NULL, flow1 REAL, flow2 REAL,
      water_level REAL, features_json TEXT, FOREIGN KEY(experiment_id) REFERENCES experiments(id))""")
    # Migrate databases from the first release.
    for column, definition in (("health_json", "TEXT"), ("experiment_id", "INTEGER"), ("blynk_json", "TEXT")):
        try: con.execute(f"ALTER TABLE readings ADD COLUMN {column} {definition}")
        except sqlite3.OperationalError: pass
    con.commit()
    return con

def features(flow1, flow2, water_level, history=None):
    if flow1 is None or flow2 is None: return {"flow_difference": None, "flow_ratio": None}
    diff = float(flow1) - float(flow2)
    return {"flow_difference": diff, "flow_ratio": float(flow2) / float(flow1) if flow1 > 0 else None}

def temporal_features(row, prior):
    """Derive only from observed history; unavailable windows remain null."""
    out=features(row.get("flow1"),row.get("flow2"),row.get("water_level"))
    history=[r for r in prior if r.get("flow1") is not None and r.get("flow2") is not None]
    if history:
        prev=history[-1]
        out["flow1_change_rate"]=row["flow1"]-prev["flow1"]
        out["flow2_change_rate"]=row["flow2"]-prev["flow2"]
        out["water_level_change_rate"]=row["water_level"]-prev["water_level"]
        out["flow_difference_change_rate"]=out["flow_difference"]-(prev.get("flow_difference") if prev.get("flow_difference") is not None else prev["flow1"]-prev["flow2"])
    else:
        out.update({k:None for k in ("flow1_change_rate","flow2_change_rate","water_level_change_rate","flow_difference_change_rate")})
    window=history[-4:]+[row]
    for key in ("flow1","flow2","water_level","flow_difference"):
        vals=[v.get(key) for v in window if v.get(key) is not None]
        out[f"{key}_mean_5"]=sum(vals)/len(vals) if vals else None
        if key=="flow_difference": out["flow_difference_std_5"]=float(__import__("numpy").std(vals)) if len(vals)>1 else 0.0 if vals else None
    return out

def age_seconds(timestamp):
    if not timestamp: return None
    try: return max(0, (datetime.now(timezone.utc) - datetime.fromisoformat(str(timestamp).replace("Z", "+00:00"))).total_seconds())
    except (ValueError, TypeError): return None

def freshness(timestamp):
    age = age_seconds(timestamp)
    return {"age_seconds": round(age, 1) if age is not None else None,
            "fresh": age is not None and age <= STALE_AFTER_SECONDS,
            "status": "FRESH" if age is not None and age <= STALE_AFTER_SECONDS else "STALE" if age is not None else "OFFLINE"}

def _history(limit=20):
    with db() as con: return [dict(r) for r in con.execute("SELECT * FROM readings ORDER BY id DESC LIMIT ?", (limit,))]

def sensor_states(item, rows=None):
    f = freshness(item.get("timestamp"))
    states = {k: "HEALTHY" for k in ("flow1", "flow2", "ultrasonic")}
    if not f["fresh"]: return {k: "OFFLINE" for k in states}
    vals = [r for r in (rows or []) if r.get("flow1") is not None and r.get("flow2") is not None]
    if item.get("flow1") is None or item.get("flow2") is None or item.get("water_level") is None:
        return {k: "MISSING" for k in states}
    if item["flow1"] > 10000 or item["flow2"] > 10000 or not 0 <= item["water_level"] <= 100:
        return {k: "IMPOSSIBLE" for k in states}
    if len(vals) >= 4 and all(abs(v["flow1"]-vals[0]["flow1"]) < 1e-9 and abs(v["flow2"]-vals[0]["flow2"]) < 1e-9 for v in vals[:4]):
        states["flow1"] = states["flow2"] = "STUCK"
    if len(vals) >= 2 and (abs(item["flow1"]-vals[1]["flow1"]) > 500 or abs(item["flow2"]-vals[1]["flow2"]) > 500):
        states["flow1"] = states["flow2"] = "JUMP"
    if item["flow1"] >= 20 and item["flow2"] == 0 and item["water_level"] < 70: states["flow2"] = "CROSS_SENSOR_INCONSISTENT"
    return states

def diagnose(flow1, flow2, water_level, sensor_health=None):
    if flow1 is None or flow2 is None or water_level is None:
        return {"condition":"NO_TELEMETRY","risk_score":None,"confidence":0,"sensor_reliability":0,
                "severity":"UNKNOWN","reasons":["No complete telemetry is available"],"recommended_action":"Connect or inspect sensors",
                "features":features(flow1,flow2,water_level),"model_used":False,"model_status":"OFFLINE"}
    ft = features(flow1, flow2, water_level); health = sensor_health or {}
    bad = [v for v in health.values() if v not in ("HEALTHY",)]
    reliability = 1.0 if not bad else 0.0 if any(v in ("OFFLINE","MISSING") for v in bad) else .55
    if bad:
        return {"condition":"SENSOR_FAULT","risk_score":None,"confidence":0,"sensor_reliability":reliability,
          "severity":"UNKNOWN","reasons":["Telemetry health is not trustworthy"],"recommended_action":"Inspect sensor before dispatching blockage maintenance",
          "features":ft,"model_used":False,"model_status":"RULE_BASED_FALLBACK"}
    if water_level >= 90: condition,severity,risk="CRITICAL WATER ACCUMULATION","CRITICAL",min(1,.7+water_level/300)
    elif flow1 >= 20 and ft["flow_difference"] >= 15:
        risk=min(.99,.45+ft["flow_difference"]/80+max(0,water_level-50)/200)
        condition="BLOCKAGE" if ft["flow_difference"] >= 30 or ft["flow_ratio"] < .35 else "DEVELOPING BLOCKAGE"
        severity="HIGH" if condition=="BLOCKAGE" else "MEDIUM"
    else: condition,severity,risk="NORMAL","LOW",max(0,water_level/300)
    reasons=["Reduced downstream flow","Increasing flow difference"] if condition in ("BLOCKAGE","DEVELOPING BLOCKAGE") else ["Flow relationship is coherent"]
    return {"condition":condition,"risk_score":round(risk,3),"confidence":round(.9 if condition!="NORMAL" else .95,3),"sensor_reliability":reliability,
      "severity":severity,"reasons":reasons,"recommended_action":"Inspect downstream section" if "BLOCKAGE" in condition else "Continue monitoring",
      "features":ft,"model_used":False,"model_status":"RULE_BASED_FALLBACK"}

def model_overlay(result, item):
    """Use an artifact only when both its metadata and all inputs are valid."""
    metadata_path = MODEL_PATH.with_suffix(".json")
    try:
        if not MODEL_PATH.exists() or not metadata_path.exists(): return result
        meta=json.loads(metadata_path.read_text())
        if meta.get("status") != "trained" or not meta.get("honest"): return result
        artifact=__import__("joblib").load(MODEL_PATH); model=artifact.get("model")
        names=artifact.get("features") or meta.get("features") or []
        values=dict(item); values.update(features(item.get("flow1"),item.get("flow2"),item.get("water_level")))
        if not model or any(n not in values or values[n] is None or not math.isfinite(float(values[n])) for n in names): return result
        x=[[float(values[n]) for n in names]]; label=model.predict(x)[0]
        result=dict(result); result["model_used"]=True; result["model_status"]="TRAINED_VALID"
        result["model_prediction"]=str(label)
        if hasattr(model,"predict_proba"):
            result["model_probabilities"]={str(k):round(float(v),4) for k,v in zip(model.classes_,model.predict_proba(x)[0])}
        result["explainability"]={"feature_importance":{n:round(float(v),6) for n,v in zip(names,model.feature_importances_)}}
    except Exception:
        return result
    return result

class Reading(BaseModel):
    flow1: Optional[float] = Field(None, ge=0, le=10000); flow2: Optional[float] = Field(None, ge=0, le=10000)
    water_level: Optional[float] = Field(None, ge=0, le=100); pump_status: str = "ON"
    full_level_count: int = Field(default=0, ge=0); timestamp: Optional[datetime] = None; source: str = "api"
    blockage_status: Optional[str] = None; water_level_status: Optional[str] = None
    blue_led: Optional[int] = Field(None, ge=0, le=1); green_led: Optional[int] = Field(None, ge=0, le=1)
    yellow_led: Optional[int] = Field(None, ge=0, le=1); red_led: Optional[int] = Field(None, ge=0, le=1)
    blynk_datastreams: Optional[dict[str, str]] = None
class PredictionRequest(BaseModel):
    flow1: float = Field(ge=0); flow2: float = Field(ge=0); water_level: float = Field(ge=0, le=100)
class SimulationRequest(BaseModel):
    blockage_severity: float = Field(ge=0, le=1); initial_water_level: float = Field(ge=0, le=100); inflow: float = Field(ge=0, le=100)
class ExperimentRequest(BaseModel):
    label: str = Field(min_length=1, max_length=80); notes: str = ""
class SampleRequest(Reading):
    label: str = Field(min_length=1, max_length=80)

def latest():
    global _latest
    if _latest is not None: return _latest
    rows=_history(1)
    if rows: return rows[0]
    return {"flow1":None,"flow2":None,"water_level":None,"pump_status":"UNKNOWN","full_level_count":0,"timestamp":None,"source":"none","flow_difference":None,"flow_ratio":None}

def _ingest(reading, experiment_id=None, sample_label=None):
    data=reading.model_dump(); data["timestamp"]=(reading.timestamp or datetime.now(timezone.utc)).isoformat()
    data.update(temporal_features(data,_history(5)))
    global _latest; _latest=data
    with db() as con:
        if experiment_id is None:
            active=con.execute("SELECT id,label FROM experiments WHERE status='running' ORDER BY id DESC LIMIT 1").fetchone()
            if active: experiment_id, sample_label=active["id"], active["label"]
        cur=con.execute("INSERT INTO readings(timestamp,flow1,flow2,water_level,pump_status,full_level_count,source,flow_difference,flow_ratio,experiment_id,blynk_json) VALUES(?,?,?,?,?,?,?,?,?,?,?)",
          (data["timestamp"],data["flow1"],data["flow2"],data["water_level"],data["pump_status"],data["full_level_count"],data["source"],data["flow_difference"],data["flow_ratio"],experiment_id,json.dumps(data.get("blynk_datastreams")) if data.get("blynk_datastreams") else None))
        data["id"]=cur.lastrowid
        if experiment_id:
            con.execute("INSERT INTO experiment_samples(experiment_id,reading_id,timestamp,label,flow1,flow2,water_level,features_json) SELECT id,?,?,?,?,?,?,? FROM experiments WHERE id=? AND status='running'",
              (cur.lastrowid,data["timestamp"],sample_label or "telemetry",data["flow1"],data["flow2"],data["water_level"],json.dumps(data),experiment_id))
    return data

@asynccontextmanager
async def lifespan(app):
    global _poll_task
    if POLL_SECONDS and not DEMO_MODE: _poll_task=asyncio.create_task(_poll_loop())
    yield
    if _poll_task: _poll_task.cancel()
app=FastAPI(title="DrainGuard AI",version="2.0.0",lifespan=lifespan)
app.add_middleware(CORSMiddleware,allow_origins=["*"],allow_methods=["*"],allow_headers=["*"])

async def _poll_loop():
    while True:
        _poll_state["last_attempt"] = datetime.now(timezone.utc).isoformat()
        try:
            a=BlynkAdapter(); raw=a.read_datastreams()
            _ingest(Reading(flow1=float(raw["V0"]),flow2=float(raw["V1"]),water_level=float(raw["V2"]),
                            pump_status=str(raw["V5"]),full_level_count=int(float(raw["V6"])),
                            blockage_status=str(raw["V4"]),water_level_status=str(raw["V7"]),
                            blue_led=int(float(raw["V8"])) if "V8" in raw else None,
                            green_led=int(float(raw["V9"])) if "V9" in raw else None,
                            yellow_led=int(float(raw["V10"])) if "V10" in raw else None,
                            red_led=int(float(raw["V11"])) if "V11" in raw else None,
                            blynk_datastreams={key: str(value) for key, value in raw.items()},
                            source="blynk"))
            _poll_state.update(connected=True, last_success=datetime.now(timezone.utc).isoformat(), error=None)
        except Exception as exc:
            _poll_state.update(connected=False, error=str(exc))
        await asyncio.sleep(POLL_SECONDS)

@app.get("/api/health")
def health(): return {"status":"ok","mode":("demo" if DEMO_MODE else "live"),"database":str(DB_PATH),"polling_seconds":POLL_SECONDS,"blynk":_poll_state}
@app.get("/api/sensors/latest")
def sensors_latest():
    item=latest(); item["freshness"]=freshness(item.get("timestamp")); item["sensor_health"]=sensor_health()["sensors"]
    item["diagnosis"]=diagnose(item.get("flow1"),item.get("flow2"),item.get("water_level"),item["sensor_health"]); return item
@app.get("/api/sensors/history")
def sensors_history(limit:int=Query(100,ge=1,le=1000)):
    rows=list(reversed(_history(limit)))
    for i,r in enumerate(rows): r.update(temporal_features(r,rows[:i]))
    return {"items":rows,"count":len(rows)}
@app.post("/api/ingest")
def ingest(reading:Reading):
    if not DEMO_MODE and reading.source=="simulation": raise HTTPException(403,"Simulation ingestion is disabled in live mode")
    return {"accepted":True,"reading":_ingest(reading)}
@app.post("/api/ingest/blynk")
def ingest_blynk():
    a=BlynkAdapter()
    if not a.configured: raise HTTPException(503,"Blynk adapter is not configured")
    try:
        # Blynk Cloud documented endpoint: /external/api/get?token=...&v=V0
        raw=a.read_datastreams()
        return {"accepted":True,"reading":_ingest(Reading(flow1=float(raw["V0"]),flow2=float(raw["V1"]),water_level=float(raw["V2"]),
            pump_status=str(raw["V5"]),full_level_count=int(float(raw["V6"])),blockage_status=str(raw["V4"]),
            water_level_status=str(raw["V7"]),blynk_datastreams={key: str(value) for key, value in raw.items()},source="blynk"))}
    except Exception as exc: raise HTTPException(502,f"Blynk read failed: {exc}") from exc
@app.get("/api/status")
def status():
    x=latest(); f=freshness(x.get("timestamp"))
    current_mode = "live" if f["fresh"] and x.get("source") == "blynk" else "demo" if DEMO_MODE else "offline"
    return {"mode":current_mode,"data_fresh":f["fresh"],"freshness":f,
            "source":x.get("source"),"pump_status":x.get("pump_status"),"database":"ok",
            "model_status":"trained" if MODEL_PATH.exists() else "RULE_BASED_FALLBACK",
            "connection_message":None if f["fresh"] else "IoT connection unavailable. Live sensor data cannot currently be retrieved.",
            "blynk":_poll_state,
            "polling":{"enabled":bool(POLL_SECONDS and not DEMO_MODE),"interval_seconds":POLL_SECONDS}}
@app.get("/api/ai/prediction")
def prediction():
    x=latest()
    return model_overlay(diagnose(x.get("flow1"),x.get("flow2"),x.get("water_level"),sensor_health()["sensors"]),x)
@app.post("/api/ai/predict")
def predict(request:PredictionRequest):
    item={"flow1":request.flow1,"flow2":request.flow2,"water_level":request.water_level}
    return model_overlay(diagnose(request.flow1,request.flow2,request.water_level,sensor_states({**item,"timestamp":datetime.now(timezone.utc).isoformat()},[])),item)
@app.get("/api/sensors/health")
def sensor_health():
    x=latest(); states=sensor_states(x,_history(20)); f=freshness(x.get("timestamp"))
    good=all(v=="HEALTHY" for v in states.values()); return {"sensors":states,"reliability":1.0 if good else .0 if not f["fresh"] else .55,"freshness":f,"cross_sensor_consistency":"COHERENT" if good else "INCONSISTENT"}
@app.get("/api/drain-health")
def drain_health():
    x=latest(); p=prediction(); risk=p.get("risk_score"); return {"score":None if risk is None else round(.5*max(0,100-abs(p["features"]["flow_difference"])*2)+.25*max(0,100-x["water_level"])+.25*(1-risk)*100),"formula":"50% flow + 25% water-level + 25% inverse blockage risk","dimensions":{} if risk is None else {"flow_health":round(max(0,100-abs(p["features"]["flow_difference"])*2)),"water_level_health":round(max(0,100-x["water_level"])),"blockage_risk":round(risk*100)},"freshness":freshness(x.get("timestamp"))}
@app.get("/api/maintenance/priority")
def priority():
    p=prediction(); r=p.get("risk_score"); return {"node":"NODE-A","risk":None if r is None else round(r*100),"confidence":round(p["confidence"]*100),"priority":"UNKNOWN" if r is None else "IMMEDIATE" if r>=.75 else "HIGH" if r>=.45 else "ROUTINE"}
@app.get("/api/alerts")
def alerts():
    p=prediction(); return {"items":[{"severity":p["severity"],"condition":p["condition"],"message":r} for r in p["reasons"]] if p["condition"] not in ("NORMAL","NO_TELEMETRY") else []}
@app.get("/api/analytics")
def analytics(): return {"history":sensors_history(100)["items"],"model_status":"Awaiting validated training data; rule fallback active"}
@app.get("/api/ai/model-info")
def model_info():
    metadata=MODEL_PATH.with_suffix(".json")
    if not MODEL_PATH.exists() or not metadata.exists(): return {"status":"Awaiting validated training data","model":None,"features":[],"metrics":{},"honest":True}
    return {**json.loads(metadata.read_text()),"path_configured":str(MODEL_PATH),"honest":True}
@app.post("/api/ai/train")
def train_model(csv_path: str):
    """Train from an operator-supplied, labelled export; guards live in train.py."""
    try:
        from .train import train
        return train(csv_path, str(MODEL_PATH))
    except (ValueError, OSError, KeyError) as exc:
        raise HTTPException(422, str(exc)) from exc

@app.post("/api/experiments")
def start_experiment(req:ExperimentRequest):
    label=req.label.strip().upper()
    if label not in EXPERIMENT_LABELS:
        raise HTTPException(422, f"label must be one of: {', '.join(sorted(EXPERIMENT_LABELS))}")
    now=datetime.now(timezone.utc).isoformat()
    with db() as con: cur=con.execute("INSERT INTO experiments(label,notes,started_at) VALUES(?,?,?)",(label,req.notes,now))
    return {"id":cur.lastrowid,"label":label,"status":"running","started_at":now}
@app.post("/api/experiments/{experiment_id}/stop")
def stop_experiment(experiment_id:int):
    now=datetime.now(timezone.utc).isoformat()
    with db() as con: cur=con.execute("UPDATE experiments SET stopped_at=?,status='stopped' WHERE id=? AND status='running'",(now,experiment_id))
    if not cur.rowcount: raise HTTPException(404,"Running experiment not found")
    return {"id":experiment_id,"status":"stopped","stopped_at":now}
@app.get("/api/experiments")
def experiments():
    with db() as con: rows=[dict(r) for r in con.execute("SELECT e.*,count(s.id) sample_count FROM experiments e LEFT JOIN experiment_samples s ON s.experiment_id=e.id GROUP BY e.id ORDER BY e.id DESC")]
    return {"items":rows}
@app.post("/api/experiments/{experiment_id}/samples")
def experiment_sample(experiment_id:int, reading:SampleRequest):
    with db() as con:
        row=con.execute("SELECT id FROM experiments WHERE id=? AND status='running'",(experiment_id,)).fetchone()
    if not row: raise HTTPException(404,"Running experiment not found")
    return {"accepted":True,"reading":_ingest(reading,experiment_id,reading.label)}
@app.get("/api/dataset/quality")
def dataset_quality():
    with db() as con:
        rows=[dict(r) for r in con.execute("SELECT * FROM experiment_samples ORDER BY id")]
        experiments=[dict(r) for r in con.execute("SELECT * FROM experiments")]
    labels={r["label"] for r in rows}; missing=sum(any(r[k] is None for k in ("flow1","flow2","water_level")) for r in rows)
    experiment_ids={r["experiment_id"] for r in rows}
    issues=[]
    if not rows: issues.append("no samples")
    if missing: issues.append("missing sensor values")
    if len(labels)<2: issues.append("need at least two labels")
    if len(rows)<20: issues.append("at least 20 samples are required")
    if len(experiment_ids)<3: issues.append("at least three physical experiments are required")
    if len({r["timestamp"] for r in rows}) != len(rows): issues.append("duplicate timestamps found")
    if any(r["label"] not in EXPERIMENT_LABELS for r in rows): issues.append("invalid experiment label")
    if any(e["status"] != "stopped" for e in experiments if e["id"] in experiment_ids): issues.append("all experiments must be stopped before training")
    if labels and min(sum(r["label"] == label for r in rows) for label in labels) < 2: issues.append("each class needs at least two samples")
    return {"valid":not issues,"rows":len(rows),"labels":sorted(labels),"missing_values":missing,
            "experiments":len(experiment_ids),"issues":issues}

@app.post("/api/simulation")
def simulation(request:SimulationRequest):
    if not DEMO_MODE: raise HTTPException(403,"Simulation is only available in demo mode")
    level=min(100,request.initial_water_level+request.inflow*(.5+request.blockage_severity)); p=diagnose(50,50*(1-request.blockage_severity),level)
    return {"mode":"SIMULATION / DEVELOPMENT MODE","predicted_water_level":round(level,1),"predicted_risk":p["condition"],"risk_score":p["risk_score"],"estimated_time_to_critical_minutes":None,"note":"Rule-based scenario estimate; not a trained predictive model"}
@app.get("/api/export.csv")
def export_csv():
    rows=sensors_history(1000)["items"]; out=io.StringIO(); w=csv.DictWriter(out,fieldnames=["timestamp","flow1","flow2","flow_difference","flow_ratio","water_level","pump_status","full_level_count","source"]); w.writeheader(); w.writerows(rows)
    return StreamingResponse(iter([out.getvalue()]),media_type="text/csv",headers={"Content-Disposition":"attachment; filename=drainguard-readings.csv"})
