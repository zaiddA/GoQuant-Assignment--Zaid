# train_slippage_model.py

import pandas as pd
from sklearn.linear_model import LinearRegression
from sklearn.model_selection import train_test_split
from sklearn.metrics import mean_squared_error, mean_absolute_error
import json
import os

def main():
    csv_file = "features_slippage.csv"
    if not os.path.exists(csv_file):
        print(f"Error: {csv_file} not found. Run extract_features.py first.")
        return

    # 1) Load data
    df = pd.read_csv(csv_file)
    df = df.dropna(subset=["slippage_100"])
    print(f"Training on {len(df)} samples")

    # 2) Features & target
    X = df[["spread", "depth_top5"]]
    y = df["slippage_100"]

    # 3) Train/test split
    X_train, X_test, y_train, y_test = train_test_split(
        X, y, test_size=0.2, random_state=42
    )

    # 4) Fit linear regression
    model = LinearRegression()
    model.fit(X_train, y_train)

    # 5) Evaluate
    y_pred = model.predict(X_test)
    mse = mean_squared_error(y_test, y_pred)
    mae = mean_absolute_error(y_test, y_pred)
    print(f"Test MSE: {mse:.3e}, MAE: {mae:.3e}")

    # 6) Export coefficients
    coeffs = {
        "intercept": model.intercept_,
        "weights": {
            "spread": model.coef_[0],
            "depth_top5": model.coef_[1]
        }
    }
    with open("slippage_model.json", "w") as f:
        json.dump(coeffs, f, indent=2)
    print("Saved model to slippage_model.json:")
    print(json.dumps(coeffs, indent=2))

if __name__ == "__main__":
    main()
