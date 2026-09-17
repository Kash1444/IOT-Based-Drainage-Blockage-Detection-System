from fastapi.testclient import TestClient
from datetime import datetime, timezone, timedelta
from app.main import app
from app.blynk import BlynkAdapter
client = TestClient(app)
def test_health(): assert client.get("/api/health").status_code == 200
def test_prediction_rule():
    r = client.post("/api/ai/predict", json={"flow1":52,"flow2":10,"water_level":67})
    assert r.status_code == 200 and r.json()["condition"] in ("BLOCKAGE","DEVELOPING BLOCKAGE")
def test_simulation_is_explicit():
    r = client.post("/api/simulation", json={"blockage_severity":.8,"initial_water_level":20,"inflow":50})
    if r.status_code == 200:
        assert "DEVELOPMENT" in r.json()["mode"]
    else:
        assert r.status_code == 403
def test_ingest_exposes_derived_features():
    r = client.post("/api/ingest", json={"flow1":50,"flow2":25,"water_level":40,"timestamp":datetime.now(timezone.utc).isoformat()})
    assert r.status_code == 200
    assert r.json()["reading"]["flow_difference"] == 25
    assert client.get("/api/sensors/history?limit=1").json()["items"][0]["flow_ratio"] == .5
def test_cross_sensor_fault_is_not_called_blockage():
    r = client.post("/api/ai/predict", json={"flow1":50,"flow2":0,"water_level":40})
    assert r.status_code == 200
    assert r.json()["condition"] == "SENSOR_FAULT"

def test_zero_flow_is_uncertain_not_confirmed_fault():
    r = client.post("/api/ai/predict", json={"flow1":0,"flow2":0,"water_level":0})
    assert r.status_code == 200
    assert r.json()["condition"] == "UNCERTAIN"

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

def test_blynk_adapter_uses_documented_virtual_pin_query(monkeypatch):
    monkeypatch.setenv("BLYNK_TOKEN", "redacted-test-token")
    adapter = BlynkAdapter()
    captured = {}

    class Response:
        text = "42"
        def raise_for_status(self): pass

    def fake_get(url, params, timeout):
        captured.update(url=url, params=params, timeout=timeout)
        return Response()

    monkeypatch.setattr("app.blynk.httpx.get", fake_get)
    assert adapter.read("V0") == "42"
    assert ("v0", "") in captured["params"]
    assert captured["timeout"] == 10

def test_latest_exposes_persisted_blynk_datastreams():
    streams = {f"V{i}": str(i * 10) for i in range(8)}
    response = client.post("/api/ingest", json={
        "flow1": 42, "flow2": 21, "water_level": 32,
        "blynk_datastreams": streams, "source": "blynk",
    })
    assert response.status_code == 200
    latest = client.get("/api/sensors/latest").json()
    assert latest["blynk_datastreams"] == streams
    assert {f"V{i}" for i in range(8)} <= set(latest["blynk_datastreams"])

def test_alerts_are_transition_deduplicated_and_have_evidence():
    client.post("/api/ingest", json={"flow1": 80, "flow2": 5, "water_level": 75})
    first = client.get("/api/alerts").json()["items"]
    second = client.get("/api/alerts").json()["items"]
    assert first and len(first) == len(second)
    assert {"timestamp", "status", "evidence"} <= set(first[0])
    assert isinstance(first[0]["evidence"], list)

def test_csv_export_handles_derived_and_blynk_fields():
    response = client.get("/api/export.csv")
    assert response.status_code == 200
    assert "flow_difference" in response.text

def test_model_status_does_not_claim_unvalidated_artifact():
    info = client.get("/api/ai/model-info").json()
    assert info["honest"] is True
    assert info["status"] in ("RULE_BASED_FALLBACK", "TRAINED_VALID")
