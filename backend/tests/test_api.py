from fastapi.testclient import TestClient
from datetime import datetime, timezone, timedelta
from app.main import app
client = TestClient(app)
def test_health(): assert client.get("/api/health").status_code == 200
def test_prediction_rule():
    r = client.post("/api/ai/predict", json={"flow1":52,"flow2":10,"water_level":67})
    assert r.status_code == 200 and r.json()["condition"] in ("BLOCKAGE","DEVELOPING BLOCKAGE")
def test_simulation_is_explicit():
    r = client.post("/api/simulation", json={"blockage_severity":.8,"initial_water_level":20,"inflow":50})
    assert r.status_code == 200 and "DEVELOPMENT" in r.json()["mode"]
def test_ingest_exposes_derived_features():
    r = client.post("/api/ingest", json={"flow1":50,"flow2":25,"water_level":40,"timestamp":datetime.now(timezone.utc).isoformat()})
    assert r.status_code == 200
    assert r.json()["reading"]["flow_difference"] == 25
    assert client.get("/api/sensors/history?limit=1").json()["items"][0]["flow_ratio"] == .5
def test_cross_sensor_fault_is_not_called_blockage():
    r = client.post("/api/ai/predict", json={"flow1":50,"flow2":0,"water_level":40})
    assert r.status_code == 200
    assert r.json()["condition"] == "SENSOR_FAULT"

def test_offline_state_does_not_fabricate_live_values():
    old = (datetime.now(timezone.utc) - timedelta(hours=1)).isoformat()
    client.post("/api/ingest", json={"flow1": 12, "flow2": 11, "water_level": 20, "timestamp": old})
    latest = client.get("/api/sensors/latest").json()
    assert latest["freshness"]["status"] == "STALE"
    assert latest["sensor_health"]["flow1"] == "OFFLINE"

def test_experiment_lifecycle_and_quality_guard():
    started = client.post("/api/experiments", json={"label": "PARTIAL_BLOCKAGE"}).json()
    eid = started["id"]
    sample = client.post(f"/api/experiments/{eid}/samples",
                         json={"label": "PARTIAL_BLOCKAGE", "flow1": 40, "flow2": 2, "water_level": 70})
    assert sample.status_code == 200
    assert client.post(f"/api/experiments/{eid}/stop").status_code == 200
    quality = client.get("/api/dataset/quality").json()
    assert quality["rows"] >= 1 and "PARTIAL_BLOCKAGE" in quality["labels"]
