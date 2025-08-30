
### Deliverables

1. **Complete Source Code with Documentation**

   - All project files including:

     - `main.cpp`, `OrderBook.cpp/.h`, `SimulatorEngine` (if applicable)
     - Model JSON files: `slippage_model.json`, `maker_taker_model.json`
     - Build scripts or `CMakeLists.txt`

   - README with updated build instructions, dependency setup, and runtime usage
   - In-code comments for clarity and maintainability

2. **Video Recording Demonstrating:**

   - **System Functionality:**

     - Launching the simulator executable
     - Connecting to OKX WebSocket
     - Displaying real-time simulation outputs

   - **Code Review:**

     - Overview of architecture
     - Key logic in `main.cpp` and `OrderBook`
     - Model loading and application flow

   - **Implementation Explanation:**

     - Slippage model structure and formula
     - Logistic regression in maker/taker prediction
     - Use of Almgren-Chriss for market impact estimation

3. **(Optional Bonus) Performance Analysis**

   - **Performance Report:**

     - Memory and CPU profiling logs
     - WebSocket latency tracking over time

   - **Benchmarking Results:**

     - Simulation of 1,000 orders
     - Average computation time per cycle

   - **Optimization Documentation:**

     - Description of implemented optimizations
     - Before/after performance comparisons

---


## Features

- ✅ Real-time L2 data ingestion (WebSocket)
- ✅ Thread-safe order book
- ✅ Market order simulation with VWAP
- ✅ Trained JSON model for slippage prediction
- ✅ Logistic regression classifier for maker/taker estimation
- ✅ Rule-based fee model
- ✅ Almgren-Chriss impact simulation
- ✅ Logging with `spdlog`
- ✅ JSON parsing via `nlohmann/json`
- ✅ Unit testing with Catch2
- ✅ Performance benchmarking via Google Benchmark

## Requirements

- C++17 compiler (MinGW or MSVC)
- CMake >= 3.14
- vcpkg for dependency management

Dependencies:

- ixwebsocket
- spdlog
- nlohmann-json
- OpenSSL
- Catch2 (testing)
- benchmark (optional)

## Setup
After Cloning ->
```bash
cd C:/dev
# Clone vcpkg
https://github.com/microsoft/vcpkg.git
cd vcpkg
./bootstrap-vcpkg.bat

# Install dependencies
./vcpkg install ixwebsocket[ssl] nlohmann-json spdlog openssl catch2 benchmark
```

## Build

```bash
cd Test
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=C:/dev/vcpkg/scripts/buildsystems/vcpkg.cmake -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release
```

## Run

```bash
   cd Test/build/Debug

  Run->  ./simulator.exe --symbol BTC-USDT --notional 100 --fee 10 --vol 1000000000 --delay 5 --interval 5
```

### Example Output:

```
[info] Sim ▶ VWAP-slip=0.000048% , Model-slip=0.000006% ($0.000012), Fee=$0.16, AC Impact=$0.00, Net(AC)=$0.16
[info] Maker/Taker ▶ Taker Probability = 98.97%
```

## Model Training

To train regression models using Python:

```bash
python extract_features.py
python train_model.py
```

Generates:

- `slippage_model.json`
- `maker_taker_model.json`

## Testing

```bash
ctest -C Debug
```

## Benchmarking

```bash
cmake --build . --config Release --target orderbook_bench
./Release/orderbook_bench.exe
```

## Author

This project was developed by Zaid (B.Tech, IIIT Pune).

---



