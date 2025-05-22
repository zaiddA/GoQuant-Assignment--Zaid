import json
import pandas as pd
import os

# 1) Point this to wherever your JSONL lives:
RAW_FILE = "build/raw_l2.jsonl"

def simulate_vwap(asks, notional=100.0):
    """Simulate a market buy VWAP spending exactly `notional` USD."""
    remaining = notional
    cost = 0.0
    acquired = 0.0

    for price, size in asks:
        level_notional = price * size
        if level_notional >= remaining:
            # partial fill at this level
            size_filled = remaining / price
            cost += price * size_filled
            acquired += size_filled
            remaining = 0.0
            break
        # full level
        cost += level_notional
        acquired += size
        remaining -= level_notional

    if remaining > 1e-9 or acquired == 0.0:
        return None
    return cost / acquired  # VWAP = total USD / total units

def main():
    if not os.path.exists(RAW_FILE):
        print(f"Error: raw file not found at {RAW_FILE}")
        return

    records = []
    with open(RAW_FILE, "r") as f:
        for line in f:
            try:
                msg = json.loads(line)
            except json.JSONDecodeError:
                continue

            # OKX v5 puts book updates under "data"
            for entry in msg.get("data", []):
                asks_raw = entry.get("asks", [])
                bids_raw = entry.get("bids", [])

                # Filter valid levels (each lvl should be [price, size, ...])
                asks = []
                for lvl in asks_raw:
                    if isinstance(lvl, (list, tuple)) and len(lvl) >= 2:
                        try:
                            asks.append((float(lvl[0]), float(lvl[1])))
                        except:
                            pass
                bids = []
                for lvl in bids_raw:
                    if isinstance(lvl, (list, tuple)) and len(lvl) >= 2:
                        try:
                            bids.append((float(lvl[0]), float(lvl[1])))
                        except:
                            pass

                if not asks or not bids:
                    continue

                best_ask, best_bid = asks[0][0], bids[0][0]
                mid_price  = (best_ask + best_bid) / 2.0
                spread     = best_ask - best_bid
                depth_top5 = sum(size for _, size in asks[:5])

                vwap = simulate_vwap(asks, 100.0)
                slippage = (vwap - mid_price) / mid_price if vwap is not None else None

                records.append({
                    "timestamp": entry.get("ts", msg.get("timestamp")),
                    "mid_price": mid_price,
                    "spread": spread,
                    "depth_top5": depth_top5,
                    "vwap_100": vwap,
                    "slippage_100": slippage
                })

    df = pd.DataFrame(records)
    print(f"Extracted features for {len(df)} snapshots")
    print(df.head(10))

    # Export to CSV
    out_csv = "features_slippage.csv"
    df.to_csv(out_csv, index=False)
    print(f"Saved features to {out_csv}")

if __name__ == "__main__":
    main()
