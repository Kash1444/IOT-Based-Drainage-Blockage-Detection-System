"""Honest Random Forest training for experiment-labelled telemetry."""
from pathlib import Path
import json
import joblib
import pandas as pd
from sklearn.ensemble import RandomForestClassifier
from sklearn.metrics import accuracy_score, confusion_matrix, precision_recall_fscore_support

FEATURES = ["flow1","flow2","flow_difference","flow_ratio","water_level",
            "water_level_change_rate","flow_difference_change_rate",
            "flow1_mean_5","flow2_mean_5","water_level_mean_5",
            "flow_difference_std_5"]
MIN_ROWS = 20

def _validate(frame, label):
    if label not in frame: raise ValueError("CSV must contain blockage_label")
    cols=[x for x in FEATURES if x in frame]
    if len(cols) < 4: raise ValueError("CSV needs at least four supported sensor features")
    if len(frame) < MIN_ROWS: raise ValueError(f"at least {MIN_ROWS} labelled rows are required")
    if frame[label].isna().any(): raise ValueError("labels must not be missing")
    if frame[label].nunique() < 2: raise ValueError("at least two classes are required")
    counts=frame[label].value_counts()
    if counts.min() < 2: raise ValueError("each class needs at least two rows")
    if frame[cols].isna().any().any(): raise ValueError("sensor features contain missing values; complete them before training")
    return cols

def train(csv_path, output_path="model.joblib"):
    frame=pd.read_csv(csv_path)
    label="blockage_label"; cols=_validate(frame,label)
    if "experiment_id" not in frame or frame.experiment_id.nunique() < 2:
        raise ValueError("experiment_id is required to prevent temporal leakage")
    experiments=sorted(frame.experiment_id.dropna().unique())
    if len(experiments) < 3: raise ValueError("at least three experiments are required for train/test separation")
    split=max(1,int(len(experiments)*.8)); split=min(split,len(experiments)-1)
    train_df=frame[frame.experiment_id.isin(experiments[:split])]
    test_df=frame[frame.experiment_id.isin(experiments[split:])]
    if train_df[label].nunique()<2 or test_df[label].nunique()<2:
        raise ValueError("train and test experiment splits must each contain both classes")
    model=RandomForestClassifier(n_estimators=200,random_state=42,class_weight="balanced",n_jobs=-1)
    model.fit(train_df[cols],train_df[label]); pred=model.predict(test_df[cols])
    labels=sorted(frame[label].unique().tolist())
    precision,recall,f1,_=precision_recall_fscore_support(test_df[label],pred,labels=labels,average="weighted",zero_division=0)
    payload={"status":"trained","model":"Random Forest Classifier","features":cols,
      "experiments":len(experiments),"train_experiments":len(experiments[:split]),"test_experiments":len(experiments[split:]),
      "training_rows":len(train_df),"test_rows":len(test_df),
      "metrics":{"accuracy":float(accuracy_score(test_df[label],pred)),"precision":float(precision),"recall":float(recall),"f1":float(f1),
                 "confusion_matrix":confusion_matrix(test_df[label],pred,labels=labels).tolist(),"class_labels":labels},
      "honest":True,"note":"Held-out experiments; no synthetic values or claims beyond this split"}
    output=Path(output_path); output.parent.mkdir(parents=True,exist_ok=True)
    joblib.dump({"model":model,"features":cols,"metadata":payload},output)
    output.with_suffix(".json").write_text(json.dumps(payload,indent=2))
    return payload
