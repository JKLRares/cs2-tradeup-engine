#include "tradeup/serialization.hpp"
#include "tradeup/historical.hpp"

#include <algorithm>
#include <iomanip>
#include <sstream>

namespace tradeup {
namespace {
void j(std::ostream& out, const std::string_view text) {
    constexpr char hex[] = "0123456789abcdef";
    out << '"';
    for (const unsigned char c : text) {
        if (c == '"' || c == '\\') { out << '\\' << static_cast<char>(c); }
        else if (c < 32) { out << "\\u00" << hex[c/16] << hex[c%16]; }
        else { out << static_cast<char>(c); }
    }
    out << '"';
}
void strings(std::ostream& out, const std::vector<std::string>& values) {
    out << '['; bool first=true;
    for (const auto& s : values) { if (!first) { out << ','; } first=false; j(out,s); } out << ']';
}
void quote(std::ostream& out, const Quote& q) {
    out << "{\"value\":"; j(out,q.value.str()); out << ",\"kind\":"; j(out,quote_kind_name(q.kind));
    out << ",\"source\":"; j(out,q.source); out << '}';
}
void context(std::ostream& out, const LoadedCatalog& c) {
    out << "{\"source\":"; j(out,c.source_path); out << ",\"format\":"; j(out,c.source_format);
    out << ",\"source_sha256\":"; j(out,c.source_sha256); out << ",\"content_sha256\":"; j(out,c.content_sha256);
    out << ",\"snapshot_id\":"; j(out,c.dataset.snapshot_id); out << ",\"identity_version\":\"tradeup-snapshot-v1\",\"mapping\":"; j(out,c.mapping_version);
    out << ",\"source_binding\":"; j(out,c.source_binding);
    out << ",\"scenario\":"; j(out,c.dataset.scenario_id); out << ",\"scenario_label\":"; j(out,c.scenario_label);
    out << ",\"probability_policy\":\"historical-quote-tickets-v1\",\"fee_policy\":\"historical-proportional-13-percent-v1\",\"valuation_policy\":";
    j(out,c.dataset.output_valuation_policy_id); out << ",\"price_unit\":"; j(out,c.dataset.price_unit);
    out << ",\"currency\":null,\"quote_timestamp\":null,\"matches_preserved_matrix_values\":";
    if (c.matrix.empty()) { out << "null"; } else { out << (c.authentic_preserved_values ? "true" : "false"); }
    out << ",\"limitations\":"; strings(out,c.dataset.limitations); out << '}';
}
void metadata(std::ostream& out, const ReportInfo& info) {
    out << "\"schema\":\"tradeup-report-v1\",\"operation\":"; j(out,info.operation); out << ",\"dataset\":"; context(out,info.catalog);
    out << ",\"exact_numbers\":\"normalized numerator/positive-denominator strings\",\"query\":";
    if (!info.query) { out << "null"; } else {
        const auto& q=*info.query; out << "{\"profile\":"; j(out,profile_id(q.profile));
        out << ",\"strict_min_return\":"; j(out,q.threshold.str()); out << ",\"max_cost_inclusive\":";
        if (q.max_cost) { j(out,q.max_cost->str()); } else { out << "null"; }
        out << ",\"max_cost_applied\":\"post-search retained-result filter\",\"ranking\":"; j(out,q.ranking==Ranking::roi ? "roi" : "profit");
        out << ",\"tie_break\":\"canonical contract identity ascending\",\"pruning\":" << (q.options.pruning ? "true" : "false") << '}';
    }
    out << ",\"completion\":{\"scope\":"; j(out,info.search ? "selected search domain" : "single contract");
    out << ",\"status\":"; j(out,info.search ? completion_name(info.search->completion) : "complete");
    out << ",\"complete\":" << (!info.search || info.search->complete() ? "true" : "false") << ",\"detail\":";
    j(out,info.search ? info.search->detail : ""); out << "},\"limits\":";
    if (!info.query) { out << "null"; } else {
        const auto& l=info.query->limits;
        out << "{\"max_evaluations\":" << l.max_evaluations << ",\"max_work\":" << l.max_work << ",\"max_results\":" << l.max_results
            << ",\"max_result_bytes\":" << l.max_result_bytes << ",\"max_time_ms\":" << l.max_time.count() << '}';
    }
    out << ",\"statistics\":";
    if (!info.search) { out << "null"; } else {
        const auto& s=info.search->statistics;
        out << "{\"domain_contracts\":";
        if (s.domain_count_available) { out << s.domain_contracts; } else { out << "null"; }
        out << ",\"candidates_considered\":" << s.candidates_considered << ",\"candidates_evaluated\":" << s.candidates_evaluated
            << ",\"candidates_pruned\":" << s.candidates_pruned << ",\"qualifying_found\":" << s.qualifying_found
            << ",\"work\":" << s.work << ",\"supports\":" << s.supports << ",\"unavailable_bounds\":" << s.unavailable_bounds
            << ",\"integer_decisions\":" << s.integer_decisions << ",\"result_capacity_bytes\":" << s.result_capacity_bytes
            << ",\"elapsed_seconds\":" << std::setprecision(17) << s.elapsed_seconds << '}';
    }
    const auto retained=info.search ? info.search->qualifying.size() : 1;
    out << ",\"selection\":{\"retained_results\":" << retained << ",\"retained_qualifying\":";
    if(info.search){out<<retained;}else{out<<"null";}
    out << ",\"matching_retained\":" << info.matched
        << ",\"filtered_out\":" << retained-info.matched << ",\"human_display_count\":" << info.human_shown
        << ",\"machine_result_count\":" << info.matched << ",\"top_applies_to\":\"human display only; exports contain every matching retained result\"}"
        << ",\"outcomes_included\":" << (info.outcomes ? "true" : "false");
}
const InputBucket& bucket(const LoadedCatalog& c, const BucketId& id) {
    const auto it=std::lower_bound(c.dataset.buckets.begin(),c.dataset.buckets.end(),id,[](const InputBucket& b,const BucketId& value) { return b.id<value; });
    if (it==c.dataset.buckets.end() || it->id!=id) { throw std::logic_error("report references unknown input"); } return *it;
}
void allocations(std::ostream& out, const LoadedCatalog& catalog, const Evaluation& e) {
    out << '['; bool first=true;
    for (const auto& a : e.contract.allocations) {
        if (!first) { out << ','; } first=false;
        const auto& b=bucket(catalog,a.bucket);
        out << "{\"bucket\":"; j(out,a.bucket.value); out << ",\"label\":"; j(out,catalog.bucket_labels.at(a.bucket.value));
        out << ",\"collection\":"; j(out,b.collection.value); out << ",\"wear\":"; j(out,b.wear==WearCategory::lower ? "lower" : "higher");
        out << ",\"quantity\":" << a.quantity << ",\"unit_cost\":"; quote(out,b.unit_cost); out << '}';
    } out << ']';
}
void outcomes(std::ostream& out, const Evaluation& e) {
    out << '['; bool first=true;
    for (const auto& p : e.outcomes) {
        if (!first) { out << ','; } first=false;
        out << "{\"collection\":"; j(out,p.collection.value); out << ",\"outcome\":"; j(out,p.outcome.value);
        out << ",\"probability\":"; j(out,p.probability.str()); out << ",\"gross_price\":"; quote(out,p.gross_price);
        out << ",\"net_receipt\":"; j(out,p.net_receipt.str()); out << '}';
    } out << ']';
}
void evaluation(std::ostream& out, const LoadedCatalog& catalog, const Evaluation& e, const bool details, const std::size_t rank) {
    out << "{\"rank\":" << rank << ",\"contract_id\":"; j(out,e.contract.serialize()); out << ",\"evaluation_id\":"; j(out,e.serialize_identity());
    out << ",\"allocations\":"; allocations(out,catalog,e);
    for (const auto& [name,value] : std::array<std::pair<std::string_view,Rational>,7>{{
        {"cost",e.cost},{"gross_ev",e.gross_ev},{"net_ev",e.net_ev},{"expected_profit",e.expected_profit},
        {"return_percent",e.return_percent},{"roi_percent",e.roi_percent},{"worst_net",e.worst_net}}}) {
        out << ','; j(out,name); out << ':'; j(out,value.str());
    }
    out << ",\"limitations\":"; strings(out,e.limitations);
    // Always expose represented valuation kinds, including composites, without requiring full outcomes.
    std::vector<std::string> kinds;
    for (const auto& p : e.outcomes) { kinds.emplace_back(quote_kind_name(p.gross_price.kind)); }
    std::sort(kinds.begin(),kinds.end()); kinds.erase(std::unique(kinds.begin(),kinds.end()),kinds.end());
    out << ",\"outcome_quote_kinds\":"; strings(out,kinds);
    if (details) { out << ",\"outcomes\":"; outcomes(out,e); } out << '}';
}
void csv_row(std::ostream& out, const std::vector<std::string>& fields) {
    bool first=true;
    for (const auto& s : fields) {
        if (!first) { out << ','; } first=false; out << '"';
        for (const char c : s) { if (c=='"') { out << '"'; } out << c; } out << '"';
    } out << "\r\n";
}
template<class F> std::string formatted(F writer) { std::ostringstream text; writer(text); return text.str(); }
} // namespace
void write_scenarios_json(std::ostream& out) {
    out << '['; bool first=true;
    for (const auto& s : historical_scenarios()) {
        if (!first) { out << ','; } first=false;
        out << "{\"id\":"; j(out,s.id); out << ",\"label\":"; j(out,s.label); out << ",\"lower_wear\":"; j(out,s.lower_wear);
        out << ",\"higher_wear\":"; j(out,s.higher_wear); out << ",\"workbook_cost_row\":" << s.cost_row << ",\"workbook_output_rows\":[";
        for (std::size_t i=0;i<s.output_rows.size();++i) { if(i) { out << ','; } out << s.output_rows[i]; } out << "]}";
    } out << ']';
}
void write_inspect_json(std::ostream& out, const LoadedCatalog& catalog, const bool matrix) {
    out << "{\"schema\":\"tradeup-inspection-v1\",\"dataset\":"; context(out,catalog);
    out << ",\"scenarios\":"; if (catalog.matrix.empty()) { out << "[]"; } else { write_scenarios_json(out); }
    out << ",\"collections\":["; bool first=true;
    for (const auto& c : catalog.dataset.collections) {
        if (!first) { out << ','; } first=false;
        out << "{\"id\":"; j(out,c.id.value); out << ",\"label\":"; j(out,catalog.collection_labels.at(c.id.value)); out << ",\"outcomes\":[";
        bool first_outcome=true;
        for (const auto& o:c.outcomes) { if (!first_outcome) { out << ','; } first_outcome=false;
            out << "{\"id\":"; j(out,o.id.value); out << ",\"price\":"; quote(out,*o.price); out << '}'; }
        out << "]}";
    }
    out << "],\"buckets\":["; first=true;
    for (const auto& b : catalog.dataset.buckets) {
        if (!first) { out << ','; } first=false;
        out << "{\"id\":"; j(out,b.id.value); out << ",\"label\":"; j(out,catalog.bucket_labels.at(b.id.value));
        out << ",\"collection\":"; j(out,b.collection.value); out << ",\"wear\":"; j(out,b.wear==WearCategory::lower ? "lower" : "higher");
        out << ",\"unit_cost\":"; quote(out,b.unit_cost); out << '}';
    } out << ']';
    if (matrix) {
        if (catalog.matrix.empty()) { throw std::invalid_argument("--matrix requires historical-2024 input"); }
        out << ",\"matrix_rows\":55,\"matrix_columns\":58,\"matrix\":[";
        for (std::size_t r=0;r<55;++r) { if(r) {out<<',';} out << '[';
            for (std::size_t c=0;c<58;++c) {if(c) {out<<',';} j(out,catalog.matrix[r*58+c].str());} out << ']'; } out << ']';
    } out << "}\n";
}
void write_report_json(std::ostream& out, const ReportInfo& info, const OptimizedEngine& engine,
                       const std::span<const std::size_t> order, const Evaluation* single) {
    out << '{'; metadata(out,info); out << ",\"results\":[";
    if (single) { evaluation(out,info.catalog,*single,info.outcomes,1); }
    else { for (std::size_t rank=0;rank<order.size();++rank) { if(rank) {out<<',';}
        evaluation(out,info.catalog,engine.evaluate(engine.contract(info.search->qualifying[order[rank]])),info.outcomes,rank+1); } }
    out << "]}\n";
}
void write_report_csv(std::ostream& out, const ReportInfo& info, const OptimizedEngine& engine,
                      const std::span<const std::size_t> order, const Evaluation* single) {
    csv_row(out,{"record_type","metadata_json","rank","contract_id","evaluation_id","allocations_json","cost","gross_ev","net_ev",
        "expected_profit","return_percent","roi_percent","worst_net","outcomes_json","limitations_json"});
    std::vector<std::string> meta(15); meta[0]="metadata"; meta[1]=formatted([&](std::ostream& s){s<<'{';metadata(s,info);s<<'}';}); csv_row(out,meta);
    const auto emit=[&](const Evaluation& e,const std::size_t rank) {
        csv_row(out,{"result","",std::to_string(rank),e.contract.serialize(),e.serialize_identity(),formatted([&](std::ostream& s){allocations(s,info.catalog,e);}),
            e.cost.str(),e.gross_ev.str(),e.net_ev.str(),e.expected_profit.str(),e.return_percent.str(),e.roi_percent.str(),e.worst_net.str(),
            info.outcomes?formatted([&](std::ostream& s){outcomes(s,e);}):"",formatted([&](std::ostream& s){strings(s,e.limitations);})});
    };
    if(single) {emit(*single,1);} else {for(std::size_t i=0;i<order.size();++i) {emit(engine.evaluate(engine.contract(info.search->qualifying[order[i]])),i+1);}}
}
void print_inspect(std::ostream& out, const LoadedCatalog& c) {
    out << "Scenario: " << c.scenario_label << " [" << c.dataset.scenario_id << "]\nSnapshot: " << c.dataset.snapshot_id
        << "\nSource: " << c.source_path << "\nFormat: " << c.source_format << "\nCollections: " << c.dataset.collections.size()
        << "; input buckets: " << c.dataset.buckets.size() << '\n';
    for(const auto& s:c.dataset.limitations) {out<<"Limitation: "<<s<<'\n';}
    out << "Buckets (representative quotes, not listing supply):\n";
    for(const auto& b:c.dataset.buckets) {out<<b.id.value<<"  "<<c.bucket_labels.at(b.id.value)<<"  cost="<<b.unit_cost.value.str()<<"  "<<quote_kind_name(b.unit_cost.kind)<<'\n';}
}
void print_evaluation(std::ostream& out, const LoadedCatalog& c, const Evaluation& e, const bool details) {
    out << "Scenario: " << c.scenario_label << "\nContract: " << e.contract.serialize() << '\n';
    for(const auto& [label,v]:std::array<std::pair<std::string_view,Rational>,7>{{{"Input cost",e.cost},{"Gross EV",e.gross_ev},{"Net EV",e.net_ev},
        {"Expected profit",e.expected_profit},{"Return percent",e.return_percent},{"Expected ROI percent",e.roi_percent},{"Worst modeled net receipt",e.worst_net}}}) {
        out<<label<<": "<<decimal_display(v)<<" (exact "<<v.str()<<")\n";
    }
    for(const auto& a:e.contract.allocations) {const auto& b=bucket(c,a.bucket);out<<a.bucket.value<<" x "<<a.quantity<<": "<<c.bucket_labels.at(a.bucket.value)<<"; "<<quote_kind_name(b.unit_cost.kind)<<"; "<<b.unit_cost.source<<'\n';}
    for(const auto& warning:e.limitations) {out<<"Limitation: "<<warning<<'\n';}
    if(details) { for(const auto& p:e.outcomes) {out<<p.collection.value<<'/'<<p.outcome.value<<" p="<<p.probability.str()<<" gross="<<p.gross_price.value.str()
        <<" net="<<p.net_receipt.str()<<"; "<<quote_kind_name(p.gross_price.kind)<<"; "<<p.gross_price.source<<'\n';} }
}
void print_search(std::ostream& out, const ReportInfo& info, const OptimizedEngine& engine, const std::span<const std::size_t> order) {
    const auto& r=*info.search; const auto& s=r.statistics;
    out << "Search status: " << completion_name(r.completion) << (r.complete()?" (complete for selected profile)":" (INCOMPLETE; retained results are partial)")
        << "\nScenario: " << info.catalog.scenario_label << "\nSnapshot: " << info.catalog.dataset.snapshot_id << "\nProfile: " << profile_id(info.query->profile)
        << "\nStrict return threshold: " << info.query->threshold.str() << "\nDomain: ";
    if(s.domain_count_available){out<<s.domain_contracts;}else{out<<"unknown";}
    out << "; considered: " << s.candidates_considered << "; evaluated: " << s.candidates_evaluated << "; proved pruned: " << s.candidates_pruned
        << "\nQualifiers found: " << s.qualifying_found << "; retained: " << r.qualifying.size() << "; matching post-search filter: " << order.size()
        << "; shown: " << info.human_shown << "\nWork: " << s.work << "; result capacity bytes: " << s.result_capacity_bytes << '\n';
    if(!r.detail.empty()){out<<"Diagnostic: "<<r.detail<<'\n';}
    if(r.complete() && r.qualifying.empty()){out<<"No qualifying contracts in the complete selected search domain.\n";}
    if(info.query->max_cost){out<<"Display/export filter: input cost <= "<<info.query->max_cost->str()<<"; no extra search pruning.\n";}
    out << "Rank  Expected ROI%  Expected profit  Input cost  Bucket quantities\n";
    for(std::size_t i=0;i<info.human_shown;++i) {
        const auto& hit=r.qualifying[order[i]];const auto contract=engine.contract(hit);
        out<<i+1<<"  "<<decimal_display(hit.roi_percent)<<"  "<<decimal_display(hit.expected_profit)<<"  "<<decimal_display(hit.cost)<<"  ";
        bool first=true;for(const auto& a:contract.allocations){if(!first){out<<',';}first=false;out<<a.bucket.value<<':'<<a.quantity;}out<<'\n';
        if(info.outcomes){print_evaluation(out,info.catalog,engine.evaluate(contract),true);}
    }
    out << "Top limits human display only; machine exports contain every matching retained result.\n";
    for(const auto& warning:info.catalog.dataset.limitations){out<<"Limitation: "<<warning<<'\n';}
}
} // namespace tradeup
