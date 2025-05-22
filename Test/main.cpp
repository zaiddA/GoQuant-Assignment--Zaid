// main.cpp

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <winsock2.h>
#include <windows.h>
#endif

#include <algorithm>
#include <csignal>
#include <atomic>
#include <fstream>
#include <iostream>
#include <thread>
#include <chrono>
#include <cmath>
#include <string>

#include "OrderBook.h"

#include <ixwebsocket/IXNetSystem.h>
#include <ixwebsocket/IXWebSocket.h>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

using json = nlohmann::json;

// Graceful shutdown flag
static std::atomic<bool> keepRunning{true};
static void handle_signal(int) { keepRunning = false; }

// Almgren–Chriss impact model
double almgrenChrissImpact(double Q, double V, double sigma, double lambda)
{
    double kappa = 0.1;
    double perm = kappa * sigma / V * Q * Q;
    double risk = lambda * sigma * sigma * Q * Q;
    return perm + risk;
}

// Command-line usage help
static void print_help()
{
    std::cout
        << "Usage: simulator [options]\n"
        << "  -s,--symbol    SYMBOL     Instrument (default: BTC-USDT)\n"
        << "  -n,--notional  USD        Notional (default: 100)\n"
        << "  -f,--fee       BPS        Taker fee in bps (default: 10)\n"
        << "  -v,--vol       USD        Daily volume (default: 1e9)\n"
        << "  -d,--delay     SECONDS    Warmup delay (default: 5)\n"
        << "  -i,--interval  SECONDS    Simulation interval (default: 5)\n"
        << "  --volatility   FLOAT      Daily sigma (default: 0.005)\n"
        << "  --risk         FLOAT      Risk aversion lambda (default: 1e-6)\n"
        << "  -h,--help                 Show this help message\n";
}

// Maker/Taker classifier
struct MakerTakerModel {
    double intercept = 0.0;
    double w_spread = 0.0;
    double w_depth = 0.0;

    bool load(const std::string& filename) {
        std::ifstream in(filename);
        if (!in) return false;
        json j;
        in >> j;
        intercept = j["intercept"];
        w_spread = j["weights"]["spread"];
        w_depth = j["weights"]["depth_top5"];
        return true;
    }

    double predict(double spread, double depth_top5) const {
        double z = intercept + w_spread * spread + w_depth * depth_top5;
        return 1.0 / (1.0 + std::exp(-z));
    }
};

int main(int argc, char *argv[])
{
    std::signal(SIGINT, handle_signal);

#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    GetConsoleMode(hOut, &mode);
    SetConsoleMode(hOut, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
#endif

    std::string symbol = "BTC-USDT";
    double notionalUsd = 100.0, takerBps = 10.0, dailyVolUsd = 1e9;
    int warmupSec = 5, intervalSec = 5;
    double sigma = 0.005, lambda = 1e-6;

    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if ((a == "-s" || a == "--symbol") && i + 1 < argc) symbol = argv[++i];
        else if ((a == "-n" || a == "--notional") && i + 1 < argc) notionalUsd = std::stod(argv[++i]);
        else if ((a == "-f" || a == "--fee") && i + 1 < argc) takerBps = std::stod(argv[++i]);
        else if ((a == "-v" || a == "--vol") && i + 1 < argc) dailyVolUsd = std::stod(argv[++i]);
        else if ((a == "-d" || a == "--delay") && i + 1 < argc) warmupSec = std::stoi(argv[++i]);
        else if ((a == "-i" || a == "--interval") && i + 1 < argc) intervalSec = std::stoi(argv[++i]);
        else if (a == "--volatility" && i + 1 < argc) sigma = std::stod(argv[++i]);
        else if (a == "--risk" && i + 1 < argc) lambda = std::stod(argv[++i]);
        else if (a == "-h" || a == "--help") { print_help(); return 0; }
        else { std::cerr << "Unknown option: " << a << "\n"; print_help(); return 1; }
    }

    spdlog::info("Params ▶ symbol={}, notional=${:.2f}, fee={}bps, vol=${:.0f}, delay={}s, interval={}s, sigma={:.3%}, lambda={:.1e}",
                 symbol, notionalUsd, takerBps, dailyVolUsd, warmupSec, intervalSec, sigma, lambda);

    double intercept = 0.0, w_spread = 0.0, w_depth = 0.0;
    {
        std::ifstream in("slippage_model.json");
        if (!in) spdlog::warn("Could not open slippage_model.json; model slippage disabled");
        else {
            json m;
            in >> m;
            intercept = m.value("intercept", 0.0);
            w_spread = m["weights"].value("spread", 0.0);
            w_depth = m["weights"].value("depth_top5", 0.0);
            spdlog::info("Loaded slippage model ▶ intercept={:.3e}, spread_w={:.3e}, depth_w={:.3e}", intercept, w_spread, w_depth);
        }
    }

    MakerTakerModel mt_model;
    if (!mt_model.load("maker_taker_model.json"))
        spdlog::warn("Maker/taker model not found; classification disabled.");
    else
        spdlog::info("Loaded maker/taker model ▶ intercept={:.3e}, spread_w={:.3e}, depth_w={:.3e}",
                     mt_model.intercept, mt_model.w_spread, mt_model.w_depth);

    ix::initNetSystem();
    ix::WebSocket ws;
    OrderBook book;
    spdlog::set_level(spdlog::level::info);

    ws.setUrl("wss://ws.okx.com:8443/ws/v5/public");
    ws.setOnMessageCallback([&](const ix::WebSocketMessagePtr &msg) {
        using namespace ix;
        if (msg->type == WebSocketMessageType::Open) {
            spdlog::info("Connected – subscribing to {}", symbol);
            json sub = { {"op","subscribe"}, {"args", {{{"channel","books"}, {"instId",symbol}}} } };
            ws.send(sub.dump());
        } else if (msg->type == WebSocketMessageType::Message) {
            static std::ofstream rawOut("raw_l2.jsonl", std::ios::app);
            rawOut << msg->str << "\n";
            try {
                auto j = json::parse(msg->str);
                if (j.contains("data")) {
                    for (auto &e : j["data"]) {
                        if (e.contains("asks")) {
                            for (auto &lvl : e["asks"]) {
                                double p = std::stod(lvl[0].get<std::string>());
                                double s = std::stod(lvl[1].get<std::string>());
                                book.updateLevel(true, p, s);
                            }
                        }
                        if (e.contains("bids")) {
                            for (auto &lvl : e["bids"]) {
                                double p = std::stod(lvl[0].get<std::string>());
                                double s = std::stod(lvl[1].get<std::string>());
                                book.updateLevel(false, p, s);
                            }
                        }
                    }
                }
            } catch (const std::exception &ex) {
                spdlog::error("JSON parse error: {}", ex.what());
            }
        } else if (msg->type == WebSocketMessageType::Error) {
            spdlog::error("WebSocket error: {}", msg->errorInfo.reason);
        }
    });
    ws.start();

    std::this_thread::sleep_for(std::chrono::seconds(warmupSec));
    spdlog::info("Starting simulation loop every {} seconds", intervalSec);

    while (keepRunning.load()) {
        auto t0 = std::chrono::steady_clock::now();

        double vwap = book.simulateMarketBuy(notionalUsd);
        double bestAsk = book.getBestAsk();
        double bestBid = book.getBestBid();
        double spread = book.getSpread();
        double depth5 = book.getDepthTopAsks(5);
        double mid = (bestAsk + bestBid) / 2.0;

        if (std::isnan(vwap) || std::isnan(bestAsk) || std::isnan(bestBid)) {
            spdlog::warn("Not enough depth for ${}", notionalUsd);
        } else {
            double slip_vwap = (vwap - mid) / mid * 100.0;
            double slip_mod_pct = intercept + w_spread * spread + w_depth * depth5;
            double slip_mod_usd = slip_mod_pct / 100.0 * notionalUsd;
            double fee_usd = notionalUsd * (takerBps / 10000.0);
            double ac_cost = almgrenChrissImpact(notionalUsd, dailyVolUsd, sigma, lambda);
            double net_ac = slip_mod_usd + fee_usd + ac_cost;

            spdlog::info("Sim ▶ VWAP-slip={:.6f}% , Model-slip={:.6f}% (${:.6f}), Fee=${:.2f}, AC Impact=${:.2f}, Net(AC)=${:.2f}",
                         slip_vwap, slip_mod_pct, slip_mod_usd, fee_usd, ac_cost, net_ac);

            if (!std::isnan(spread) && !std::isnan(depth5)) {
                double takerProb = mt_model.predict(spread, depth5);
                spdlog::info("Maker/Taker ▶ Taker Probability = {:.2f}%", takerProb * 100.0);
            }
        }

        auto t1 = std::chrono::steady_clock::now();
        int elapsed = int(std::chrono::duration_cast<std::chrono::seconds>(t1 - t0).count());
        int toSleep = std::max(0, intervalSec - elapsed);
        for (int i = 0; i < toSleep && keepRunning.load(); ++i) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }

    ws.stop();
    ix::uninitNetSystem();
    spdlog::info("Shutdown complete.");
    return 0;
}
