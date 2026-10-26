// Private reproducible synthetic workload, not a historical importer or application CLI.
#include "tradeup/optimized.hpp"

#include <algorithm>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <random>
#include <utility>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <psapi.h>
#endif

using namespace tradeup;
namespace {
using Clock = std::chrono::steady_clock;
double seconds(const Clock::time_point start) { return std::chrono::duration<double>(Clock::now() - start).count(); }
Dataset synthetic(const int n, const std::string& flavor) {
    Dataset data{"benchmark-seed-20261026-n" + std::to_string(n) + "-" + flavor,
        "synthetic", "synthetic-basket-v1", "unit", {}, {}, {"Synthetic benchmark"}};
    std::mt19937 rng(20261026);
    const auto quote = [](const Rational p) { return Quote{p, QuoteKind::synthetic, "seed-20261026"}; };
    for (int c = 0; c < (n + 1) / 2; ++c) {
        Collection collection{CollectionId{"c" + std::to_string(c)}, {}, true};
        for (int o = 0; o < 1 + c % 5; ++o) {
            const Rational price = flavor == "none" ? Rational(0) :
                (flavor == "all" ? Rational(100) : Rational(200 + rng() % 2801U, 100));
            collection.outcomes.push_back({OutcomeId{"o" + std::to_string(o)}, quote(price)});
        }
        data.collections.push_back(std::move(collection));
    }
    for (int i = 0; i < n; ++i) {
        const auto cost = flavor == "all" ? Rational(1) : Rational(20 + rng() % 281U, 100);
        data.buckets.push_back({BucketId{"b" + std::to_string(1000 + i)}, CollectionId{"c" + std::to_string(i / 2)},
            i % 2 == 0 ? WearCategory::lower : WearCategory::higher, quote(cost)});
    }
    return data;
}
std::string row(const Contract& c, const Rational cost, const Rational gross, const Rational net,
    const Rational profit, const Rational percent, const Rational roi, const Rational worst) {
    return c.serialize() + '|' + cost.str() + '|' + gross.str() + '|' + net.str() + '|' + profit.str()
        + '|' + percent.str() + '|' + roi.str() + '|' + worst.str();
}
std::vector<std::string> rows(const SearchResult& result) {
    std::vector<std::string> out;
    for (const auto& e : result.qualifying) { out.push_back(row(e.contract, e.cost, e.gross_ev, e.net_ev,
        e.expected_profit, e.return_percent, e.roi_percent, e.worst_net)); }
    std::sort(out.begin(), out.end()); return out;
}
std::vector<std::string> rows(const OptimizedEngine& engine, const OptimizedResult& result) {
    std::vector<std::string> out;
    for (const auto& e : result.qualifying) { out.push_back(row(engine.contract(e), e.cost, e.gross_ev, e.net_ev,
        e.expected_profit, e.return_percent, e.roi_percent, e.worst_net)); }
    std::sort(out.begin(), out.end()); return out;
}
std::size_t peak_memory() {
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS info{}; info.cb = sizeof(info);
    if (GetProcessMemoryInfo(GetCurrentProcess(), &info, sizeof(info))) {
        return static_cast<std::size_t>(info.PeakWorkingSetSize);
    }
#endif
    return 0;
}
} // namespace

int main(int argc, char** argv) {
    try {
#if !defined(NDEBUG) || (defined(__GNUC__) && !defined(__OPTIMIZE__))
        throw std::runtime_error("benchmark requires an optimized Release build");
#endif
        if (argc != 8) { throw std::invalid_argument("expected engine buckets profile flavor repeats pruning scaled"); }
        const std::string mode = argv[1], flavor = argv[4];
        const auto n = std::stoi(argv[2]); const auto repetitions = std::stoi(argv[5]);
        if (n < 2 || n > 58 || repetitions < 1 || repetitions > 20 ||
            (flavor != "mixed" && flavor != "all" && flavor != "none")) { throw std::invalid_argument("bounded benchmark parameters"); }
        const auto profile = std::string(argv[3]) == "55" ? SearchProfile::historical_5_5 : SearchProfile::historical_6_4;
        const OptimizedOptions options{std::string(argv[6]) == "1", std::string(argv[7]) == "1"};
        OptimizedLimits limits{100'000, 2'000'000, 100'000, 64 * 1024 * 1024, std::chrono::milliseconds(10'000)};
        const SearchLimits reference_limits{20'000'000, 100'000};
        if (mode == "verify") {
            const auto data = synthetic(n, flavor); const ReferenceEngine ref(data); const OptimizedEngine opt(data);
            const auto r = ref.search(profile, Rational(115), reference_limits);
            const auto o = opt.search(profile, Rational(115), limits, options);
            if (!o.complete() || r.candidates_evaluated != o.statistics.domain_contracts || rows(r) != rows(opt, o)) {
                throw std::runtime_error("benchmark full result sets differ or workload incomplete");
            }
            std::cout << "verified," << r.candidates_evaluated << ',' << r.qualifying.size() << '\n'; return 0;
        }
        std::cout << "iteration,preparation_s,search_s,domain,evaluated,pruned,qualifying,retained,work,status,result_capacity_bytes,peak_working_set_bytes,integer_decisions\n";
        std::cout << std::setprecision(10);
        for (int iteration = 0; iteration < repetitions; ++iteration) {
            const auto data = synthetic(n, flavor); // generation excluded from preparation/search
            const auto start = Clock::now();
            if (mode == "reference") {
                const ReferenceEngine engine(data); const auto preparation = seconds(start);
                const auto search_start = Clock::now(); const auto r = engine.search(profile, Rational(115), reference_limits);
                const auto elapsed = seconds(search_start);
                std::cout << iteration << ',' << preparation << ',' << elapsed << ',' << r.candidates_evaluated << ','
                    << r.candidates_evaluated << ",0," << r.qualifying.size() << ',' << r.qualifying.size() << ','
                    << r.nodes_visited << ",complete,0," << peak_memory() << ",0\n";
            } else if (mode == "optimized" || mode == "expanded") {
                const OptimizedEngine engine(data); const auto preparation = seconds(start);
                const auto search_start = Clock::now(); const auto r = engine.search(profile, Rational(115), limits, options);
                std::vector<Evaluation> expanded;
                if (mode == "expanded") {
                    expanded.reserve(r.qualifying.size());
                    for (const auto& hit : r.qualifying) { expanded.push_back(engine.evaluate(engine.contract(hit))); }
                }
                const auto elapsed = seconds(search_start); const auto& s = r.statistics;
                std::cout << iteration << ',' << preparation << ',' << elapsed << ',' << s.domain_contracts << ','
                    << s.candidates_evaluated << ',' << s.candidates_pruned << ',' << s.qualifying_found << ',' << r.qualifying.size()
                    << ',' << s.work << ',' << completion_name(r.completion) << ',' << s.result_capacity_bytes << ','
                    << peak_memory() << ',' << s.integer_decisions << '\n';
            } else { throw std::invalid_argument("invalid benchmark mode"); }
        }
        return 0;
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
