# DrainGuard AI

## Run the API

```powershell
python -m venv .venv
.venv\Scripts\Activate.ps1
pip install -r backend\requirements.txt
uvicorn app.main:app --app-dir backend --reload
```

Copy `.env.example` to `.env`. `DRAINGUARD_MODE=demo` is the safe default. Set
`DRAINGUARD_MODE=live` only when telemetry is connected; simulation ingestion is
then rejected. The Blynk adapter requires `BLYNK_TOKEN` and is intentionally
opt-in. Set `DRAINGUARD_POLL_SECONDS` to a positive interval (for example `10`)
to enable the FastAPI background poller in live mode. It reads the documented
V0, V1, V2, V5, and V6 datastreams and records each successful sample in
SQLite. Poll failures are exposed through `/api/status` and do not crash the
application.

## Run the dashboard

```powershell
cd frontend
npm install
npm run dev
```

Set `VITE_API_URL` when the API is not at `http://localhost:8000/api`.
The dashboard never fabricates telemetry: empty cards mean no readings have
been ingested. Use `POST /api/ingest` with a real or explicitly simulated
reading to populate the database.

Use the **Experiments** page while physically operating the prototype. Select
one of `NORMAL`, `PARTIAL_BLOCKAGE`, `SEVERE_BLOCKAGE`, `RISING_WATER`,
`SENSOR_FAULT`, `NOISY_SENSOR`, or `RECOVERY`, start recording, perform the
controlled experiment, and stop recording. Live/Blynk samples are associated
with the active experiment automatically. The dataset quality endpoint
(`GET /api/dataset/quality`) reports whether the collected physical data is
adequate for training.

## Tests

```powershell
pytest backend/tests
npm run build --prefix frontend
```

The fallback diagnosis is documented and rule-based. A model is not claimed
until `backend.app.train.train()` has been run against labeled experimental
CSV data; experiment IDs are split chronologically to reduce leakage. To train
and persist a real Random Forest model and its metrics:

```powershell
python -c "import sys; sys.path.insert(0, 'backend'); from app.train import train; print(train('ml\data\readings.csv', 'backend\models\model.joblib'))"
```

The CSV must contain `blockage_label` and at least two supported sensor
features. Do not use simulated readings as training evidence.
