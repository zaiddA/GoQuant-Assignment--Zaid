import pandas as pd
import json
from sklearn.linear_model import LogisticRegression
from sklearn.model_selection import train_test_split
from sklearn.metrics import accuracy_score

# Load the slippage features file
INPUT_FILE = "features_slippage.csv"
OUTPUT_FILE = "maker_taker_model.json"

# Load and preprocess data
df = pd.read_csv(INPUT_FILE)
df = df[["spread", "depth_top5", "slippage_100"]].dropna()

# Create binary label: taker if slippage > 0
df["label"] = (df["slippage_100"] > 0).astype(int)

X = df[["spread", "depth_top5"]]
y = df["label"]

# Train/test split
X_train, X_test, y_train, y_test = train_test_split(X, y, test_size=0.2, random_state=42)

# Train logistic regression classifier
clf = LogisticRegression()
clf.fit(X_train, y_train)

# Evaluate
y_pred = clf.predict(X_test)
accuracy = accuracy_score(y_test, y_pred)
print(f"Test Accuracy: {accuracy:.4f}")

# Export model as JSON
model_json = {
    "intercept": float(clf.intercept_[0]),
    "weights": {
        "spread": float(clf.coef_[0][0]),
        "depth_top5": float(clf.coef_[0][1])
    }
}

with open(OUTPUT_FILE, "w") as f:
    json.dump(model_json, f, indent=2)

print(f"Saved model to {OUTPUT_FILE}")
