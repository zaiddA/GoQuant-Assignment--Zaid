// bench_orderbook.cpp

#include <benchmark/benchmark.h>
#include <Test/OrderBook.h>

static void BM_SimulateMarketBuy(benchmark::State &state)
{
    OrderBook b;
    // populate book with 1000 ask levels
    for (int i = 0; i < 1000; ++i)
    {
        b.updateLevel(true, 100.0 + i, 1.0);
    }
    for (auto _ : state)
    {
        // simulate a $10,000 market buy and prevent compiler optimizations
        benchmark::DoNotOptimize(b.simulateMarketBuy(10000.0));
    }
}

BENCHMARK(BM_SimulateMarketBuy);

BENCHMARK_MAIN();
