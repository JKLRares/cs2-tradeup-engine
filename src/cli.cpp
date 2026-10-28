#include "tradeup/historical.hpp"
#include "tradeup/serialization.hpp"

#include <algorithm>
#include <charconv>
#include <iostream>
#include <map>
#include <set>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

namespace {
using namespace tradeup;
constexpr std::string_view help=R"help(Historical Trade-Up Search Engine - entirely offline

Usage: tradeup <command> [options]
  scenarios              List the 12 evidence-mapped historical scenarios.
  profiles               List the restricted 5/5 and 6/4 search profiles.
  inspect --input PATH   Inspect a dataset, buckets, provenance and limitations.
  eval --input PATH --contract b002:5,b029:5
                         Evaluate ten inputs; a full canonical ID is also accepted.
  replay --input PATH    Replay the preserved first 2024 result through the importer.
  search --input PATH    Run a bounded optimized search; interrupted runs exit 3.

Input/output options:
  --format historical-2024|catalog-v1   Default historical-2024; no format guessing.
  --scenario ID_OR_LABEL               Historical default st-ft-mw-mov.
  --json                              Machine JSON on stdout, all matching retained results.
  --details                           Include full outcome probabilities and provenance.
  --matrix                            inspect only: include all 3190 original values in JSON.
  --output PATH                       Write a NEW file; existing files are never overwritten.
  --output-format json|csv             Default JSON, or CSV for a .csv suffix.
  --max-export-bytes N                 Default 33554432; maximum 268435456.

Search options:
  --profile 5/5|6/4                    Default 5/5; 2-5 active buckets, ten inputs.
  --min-return EXACT                  Strict return threshold; default 115.
  --min-roi EXACT                     Alternative strict ROI threshold; minimum -100.
  --max-cost EXACT                    Inclusive POST-search retained-result filter.
  --sort roi|profit                   Descending expected ROI (default) or profit.
  --top N                             Human display only, default 20; range 0..1000.
  --max-evaluations N                 Default 100000; maximum 10000000 attempts.
  --max-work N                        Default 500000; maximum 50000000 work units.
  --max-results N                     Default 10000; maximum 100000 retained hits.
  --max-result-bytes N                Default 16777216; maximum 67108864 backing bytes.
  --time-ms N                         Default 2000; maximum 60000, cooperative.
  --no-pruning                        Disable proved profitability bounds.

EXACT accepts integers, decimals or numerator/positive-denominator fractions.
Return includes capital; ROI = return - 100. Qualification is strict, without rounding.
Top affects human display only. Exports include every matching retained result and
explicit completion, filters, budgets and counters; interrupted results are incomplete.
Expected ROI is not guaranteed profit. Historical currency/time, exact float feasibility
and listing quantities are unknown. No network or account connections are used.
Exit codes: 0 success, 2 invalid input/options, 3 incomplete budget, 4 I/O/export failure,
5 unsupported arithmetic, 6 process resource/internal failure. --help shows this text.
)help";
struct Arguments {
    std::string command;
    std::map<std::string,std::string> values;
    std::set<std::string> flags;
    bool has(const std::string& name) const {return values.contains(name)||flags.contains(name);}
    std::string get(const std::string& name,const std::string& fallback="") const {const auto i=values.find(name);return i==values.end()?fallback:i->second;}
};
Arguments parse(const std::vector<std::string>& args) {
    if(args.size()<2){throw std::invalid_argument("a command is required; use --help");}
    Arguments result;result.command=args[1];
    if(result.command=="--help"){if(args.size()!=2){throw std::invalid_argument("--help takes no values");}result.flags.insert("--help");return result;}
    const std::set<std::string> commands{"scenarios","profiles","inspect","eval","replay","search"};
    if(!commands.contains(result.command)){throw std::invalid_argument("unknown command: "+result.command+"; use --help");}
    const std::set<std::string> flags{"--help","--json","--details","--matrix","--no-pruning"};
    const std::set<std::string> values{"--input","--format","--scenario","--output","--output-format","--max-export-bytes","--contract","--profile",
        "--min-return","--min-roi","--max-cost","--sort","--top","--max-evaluations","--max-work","--max-results","--max-result-bytes","--time-ms"};
    for(std::size_t i=2;i<args.size();++i){
        const auto& name=args[i];if(result.has(name)){throw std::invalid_argument("duplicate option: "+name);}
        if(flags.contains(name)){result.flags.insert(name);}
        else if(values.contains(name)){if(i+1==args.size()){throw std::invalid_argument("missing value for "+name);}result.values[name]=args[++i];}
        else{throw std::invalid_argument("unknown option or positional argument: "+name);}
    }
    if(result.has("--help")){return result;}
    std::set<std::string> allowed{"--json"};
    if(result.command!="scenarios"&&result.command!="profiles"){
        allowed.insert({"--input","--format","--scenario","--output","--output-format","--max-export-bytes"});
        if(result.command=="inspect"){allowed.insert("--matrix");}else{allowed.insert("--details");}
        if(result.command=="eval"){allowed.insert("--contract");}
        if(result.command=="search"){allowed.insert({"--profile","--min-return","--min-roi","--max-cost","--sort","--top","--max-evaluations",
            "--max-work","--max-results","--max-result-bytes","--time-ms","--no-pruning"});}
    }
    for(const auto& [key,value]:result.values){(void)value;if(!allowed.contains(key)){throw std::invalid_argument(key+" does not apply to "+result.command);}}
    for(const auto& key:result.flags){if(!allowed.contains(key)){throw std::invalid_argument(key+" does not apply to "+result.command);}}
    return result;
}
std::uint64_t number(const Arguments& args,const std::string& name,const std::uint64_t fallback,const std::uint64_t maximum,const bool zero=false){
    if(!args.has(name)){return fallback;}const auto text=args.get(name);std::uint64_t value=0;
    const auto parsed=std::from_chars(text.data(),text.data()+text.size(),value);
    if(text.empty()||parsed.ec!=std::errc{}||parsed.ptr!=text.data()+text.size()||(!zero&&value==0)||value>maximum){throw std::invalid_argument(name+" requires "+(zero?"0":"1")+".."+std::to_string(maximum));}
    return value;
}
SearchQuery query(const Arguments& a){
    SearchQuery q;const auto profile=a.get("--profile","5/5");
    if(profile=="5/5"||profile==profile_id(SearchProfile::historical_5_5)){q.profile=SearchProfile::historical_5_5;}
    else if(profile=="6/4"||profile==profile_id(SearchProfile::historical_6_4)){q.profile=SearchProfile::historical_6_4;}
    else{throw std::invalid_argument("unsupported profile; use profiles");}
    if(a.has("--min-return")&&a.has("--min-roi")){throw std::invalid_argument("choose --min-return or --min-roi");}
    if(a.has("--min-return")){q.threshold=Rational::parse(a.get("--min-return"));}
    if(a.has("--min-roi")){q.threshold=Rational::parse(a.get("--min-roi"))+Rational(100);}
    if(q.threshold<Rational(0)){throw std::invalid_argument("return threshold must be nonnegative (ROI >= -100)");}
    if(a.has("--max-cost")){q.max_cost=Rational::parse(a.get("--max-cost"));if(*q.max_cost<=Rational(0)){throw std::invalid_argument("--max-cost must be positive");}}
    const auto ranking=a.get("--sort","roi");if(ranking!="roi"&&ranking!="profit"){throw std::invalid_argument("--sort must be roi or profit");}q.ranking=ranking=="roi"?Ranking::roi:Ranking::profit;
    q.top=static_cast<std::size_t>(number(a,"--top",20,1000,true));
    q.limits.max_evaluations=number(a,"--max-evaluations",100000,10000000);
    q.limits.max_work=number(a,"--max-work",500000,50000000);
    q.limits.max_results=static_cast<std::size_t>(number(a,"--max-results",10000,100000));
    q.limits.max_result_bytes=static_cast<std::size_t>(number(a,"--max-result-bytes",16777216,67108864));
    q.limits.max_time=std::chrono::milliseconds(static_cast<std::int64_t>(number(a,"--time-ms",2000,60000)));q.options.pruning=!a.has("--no-pruning");return q;
}
int run(const std::vector<std::string>& arguments){
    const auto a=parse(arguments);
    if(a.has("--help")){std::cout<<help;return 0;}
    if(a.command=="scenarios"){
        if(a.has("--json")){write_scenarios_json(std::cout);std::cout<<'\n';}
        else{for(const auto& s:historical_scenarios()){std::cout<<s.id<<"  "<<s.label<<"  workbook cost="<<s.cost_row<<" outputs=";for(const auto r:s.output_rows){std::cout<<r<<' ';}std::cout<<'\n';}}return 0;
    }
    if(a.command=="profiles"){
        if(a.has("--json")){std::cout<<"[{\"id\":\""<<profile_id(SearchProfile::historical_5_5)<<"\",\"lower\":5,\"higher\":5,\"min_active\":2,\"max_active\":5},{\"id\":\""
            <<profile_id(SearchProfile::historical_6_4)<<"\",\"lower\":6,\"higher\":4,\"min_active\":2,\"max_active\":5}]\n";}
        else{std::cout<<profile_id(SearchProfile::historical_5_5)<<"  5 lower / 5 higher\n"<<profile_id(SearchProfile::historical_6_4)<<"  6 lower / 4 higher\nBoth: ten inputs, 2-5 active buckets; restricted domains, not unrestricted searches.\n";}return 0;
    }
    if(!a.has("--input")){throw std::invalid_argument("--input PATH is required");}
    if(a.has("--output-format")&&!a.has("--output")){throw std::invalid_argument("--output-format requires --output");}
    const auto format=a.get("--format","historical-2024");if(format!="historical-2024"&&format!="catalog-v1"){throw std::invalid_argument("--format must be historical-2024 or catalog-v1");}
    if(a.command=="replay"&&(format!="historical-2024"||historical_scenario(a.get("--scenario","st-ft-mw-mov")).id!="st-ft-mw-mov")){throw std::invalid_argument("replay uses the historical-2024 StatTrak FT MW mov scenario");}
    const auto input=utf8_path(a.get("--input"));
    auto loaded=format=="historical-2024"?load_historical_2024(input,a.get("--scenario","st-ft-mw-mov")):load_catalog_v1(input);
    if(format=="catalog-v1"&&a.has("--scenario")&&a.get("--scenario")!=loaded.dataset.scenario_id&&a.get("--scenario")!=loaded.scenario_label){throw std::invalid_argument("scenario does not match the explicit catalog");}
    if(a.command=="replay"&&!loaded.authentic_preserved_values){throw std::invalid_argument("historical replay requires all preserved matrix values unchanged; use eval for edited datasets");}
    const auto max_bytes=static_cast<std::size_t>(number(a,"--max-export-bytes",33554432,268435456));
    auto output_format=a.get("--output-format",utf8_path(a.get("--output")).extension()==".csv"?"csv":"json");
    if(output_format!="json"&&output_format!="csv"){throw std::invalid_argument("--output-format must be json or csv");}
    const auto export_file=[&](const std::function<void(std::ostream&)>& writer){
        if(a.has("--output")){
            try{write_export(utf8_path(a.get("--output")),max_bytes,writer);}
            catch(const std::exception& e){throw std::runtime_error("export failed: "+std::string(e.what()));}
        }
    };
    if(a.command=="inspect"){
        if(output_format=="csv"){throw std::invalid_argument("inspect exports JSON; CSV is for evaluations/searches");}
        if(a.has("--matrix")&&(loaded.matrix.empty()||(!a.has("--json")&&!a.has("--output")))){throw std::invalid_argument("--matrix requires historical input and --json or --output");}
        const auto writer=[&](std::ostream& out){write_inspect_json(out,loaded,a.has("--matrix"));};export_file(writer);
        if(a.has("--json")){write_bounded(std::cout,max_bytes,writer);}else{write_bounded(std::cout,max_bytes,[&](std::ostream& out){print_inspect(out,loaded);});}return 0;
    }
    const OptimizedEngine engine(loaded.dataset);
    if(a.command=="eval"||a.command=="replay"){
        if(a.command=="eval"&&!a.has("--contract")){throw std::invalid_argument("eval requires --contract bucket:quantity,...");}
        auto contract=a.command=="replay"?std::string("b002:5,b029:5"):a.get("--contract");
        if(contract.starts_with("contract-v1|")){
            const auto parts=split_fields(contract,'|');if(parts.size()!=4||parts[1]!=loaded.dataset.snapshot_id||parts[2]!=loaded.dataset.scenario_id){throw std::invalid_argument("canonical contract snapshot/scenario does not match imported dataset");}contract=parts[3];
        }
        const auto evaluation=engine.evaluate({loaded.dataset.snapshot_id,loaded.dataset.scenario_id,parse_allocations(contract)});
        const ReportInfo info{loaded,a.command,nullptr,nullptr,1,a.has("--json")?0U:1U,a.has("--details")};
        const auto json=[&](std::ostream& out){write_report_json(out,info,engine,{},&evaluation);};
        export_file([&](std::ostream& out){if(output_format=="json"){json(out);}else{write_report_csv(out,info,engine,{},&evaluation);}});
        if(a.has("--json")){write_bounded(std::cout,max_bytes,json);}else{write_bounded(std::cout,max_bytes,[&](std::ostream& out){print_evaluation(out,loaded,evaluation,info.outcomes);});}return 0;
    }
    const auto q=query(a);const auto result=engine.search(q.profile,q.threshold,q.limits,q.options);
    const auto order=select_results(engine,result,q);
    const ReportInfo info{loaded,"search",&q,&result,order.size(),a.has("--json")?0U:std::min(q.top,order.size()),a.has("--details")};
    const auto json=[&](std::ostream& out){write_report_json(out,info,engine,order);};
    export_file([&](std::ostream& out){if(output_format=="json"){json(out);}else{write_report_csv(out,info,engine,order);}});
    if(a.has("--json")){write_bounded(std::cout,max_bytes,json);}else{write_bounded(std::cout,max_bytes,[&](std::ostream& out){print_search(out,info,engine,order);});}
    return result.complete()?0:(result.completion==Completion::arithmetic_limit?5:3);
}
int entry(const std::vector<std::string>& args){
    try{return run(args);}
    catch(const std::overflow_error& e){std::cerr<<"Unsupported arithmetic: "<<e.what()<<'\n';return 5;}
    catch(const std::invalid_argument& e){std::cerr<<"Invalid input/options: "<<e.what()<<'\n';return 2;}
    catch(const std::filesystem::filesystem_error& e){std::cerr<<"Filesystem error: "<<e.what()<<'\n';return 4;}
    catch(const std::bad_alloc&){std::cerr<<"Process memory allocation failed; no complete report produced.\n";return 6;}
    catch(const std::exception& e){std::cerr<<"Application/output failure: "<<e.what()<<'\n';return 4;}
}
} // namespace
#ifdef _WIN32
int wmain(const int argc,wchar_t** argv){
    std::vector<std::string> args;
    for(int i=0;i<argc;++i){
        const auto size=WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,argv[i],-1,nullptr,0,nullptr,nullptr);
        if(size<=0){std::cerr<<"Invalid Unicode command line\n";return 2;}
        std::string value(static_cast<std::size_t>(size),'\0');
        WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,argv[i],-1,value.data(),size,nullptr,nullptr);value.pop_back();args.push_back(std::move(value));
    }return entry(args);
}
#else
int main(const int argc,char** argv){return entry({argv,argv+argc});}
#endif
