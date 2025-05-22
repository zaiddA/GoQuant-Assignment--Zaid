# Go-Quant-Assignment-Zaid

Overview
This project simulates market buy executions using live L2 orderbook data from OKX. It calculates actual and predicted slippage, applies the Almgren–Chriss model for market impact, and classifies each trade as maker or taker using a logistic regression model.
Features
- Live WebSocket subscription to OKX
- VWAP-based market simulation
- Slippage regression model (pre-trained)
- Maker/Taker classifier (pre-trained)
- Almgren–Chriss market impact model
- Structured logging with `spdlog`
Requirements
- Windows with MinGW-w64
- CMake ≥ 3.14
- Libraries: ixwebsocket, spdlog, nlohmann-json
Folder Structure
```
GoQuant/
├── Test
├── OrderBook.cpp / OrderBook.h
├── slippage_model.json
├── maker_taker_model.json
├── CMakeLists.txt
└── build/ (for simulator.exe)
```
Build Instructions
Open PowerShell or terminal:
```bash
cd GoQuant
mkdir build && cd build
```
Run the Simulator
```bash
./simulator.exe --symbol BTC-USDT --notional 100 --fee 10 --vol 1000000000 --delay 5 --interval 5
```
Example Output
```
[info] Sim ▶ VWAP-slip=0.000048%, Model-slip=0.000006% ($0.000012), Fee=$0.16, AC Impact=$0.00, Net(AC)=$0.16
[info] Maker/Taker ▶ Taker Probability = 98.97%
```
Model Training (Optional)
- `train_slippage_model.py` → generates slippage_model.json
- `train_maker_taker_model.py` → generates maker_taker_model.json
