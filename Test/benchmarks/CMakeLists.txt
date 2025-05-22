find_package(benchmark CONFIG REQUIRED)
add_executable(orderbook_bench bench_orderbook.cpp)
target_link_libraries(orderbook_bench PRIVATE
  benchmark::benchmark
  simulator_lib   # or your OrderBook implementation
)
