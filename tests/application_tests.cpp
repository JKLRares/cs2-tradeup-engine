#include "tradeup/application.hpp"

#include <functional>
#include <fstream>
#include <iostream>
#include <iterator>
#include <limits>
#include <sstream>
#include <random>

using namespace tradeup;
namespace {
void require(const bool condition){if(!condition){throw std::runtime_error("check failed");}}
template<class Exception,class F>void rejects(F f){try{f();}catch(const Exception&){return;}throw std::runtime_error("expected exception");}
struct TemporaryDirectory {
    std::filesystem::path path;
    TemporaryDirectory() {
        std::random_device random;
        for (int i=0; i<16; ++i) {
            auto candidate=std::filesystem::temp_directory_path() / ("tradeup-publication-test-"+std::to_string(random()));
            if (std::filesystem::create_directory(candidate)) { path=std::move(candidate); return; }
        }
        throw std::runtime_error("cannot create publication test directory");
    }
    ~TemporaryDirectory() { std::error_code ignored; std::filesystem::remove_all(path,ignored); }
};
}
int main(){
    int passed=0,failed=0;
    const auto test=[&](const char* name,const std::function<void()>& f){try{f();++passed;}catch(const std::exception& e){++failed;std::cerr<<"FAIL "<<name<<": "<<e.what()<<'\n';}};
    test("SHA-256 empty",[]{require(sha256("")=="e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");});
    test("SHA-256 abc",[]{require(sha256("abc")=="ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");});
    test("SHA-256 multi-block published vector",[]{require(sha256("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq")=="248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1");});
    test("SHA-256 million a",[]{require(sha256(std::string(1000000,'a'))=="cdc76e5c9914fb9281a1c7e284d73e67f1809a48a497200e046d39ccc7112cd0");});
    test("decimal replay rounded independently of decisions",[]{require(decimal_display(Rational(186383,1455))=="128.098282");require(decimal_display(Rational(40883,1455))=="28.098282");});
    test("decimal negative rounding and carry",[]{require(decimal_display(Rational(-1,3))=="-0.333333");require(decimal_display(Rational(9999999,10000000))=="1.000000");require(decimal_display(Rational(1999,1000),2)=="2.00");});
    test("decimal extreme remainders cannot overflow",[]{constexpr auto m=std::numeric_limits<std::int64_t>::max();require(decimal_display(Rational(m-1,m))=="1.000000");require(decimal_display(Rational(m))=="9223372036854775807.000000");});
    test("allocation parse preserves identities",[]{const auto a=parse_allocations("b002:5,b029:5");require(a.size()==2&&a[0].bucket.value=="b002"&&a[1].quantity==5);});
    test("malformed allocations rejected",[]{for(const auto* s:{"","b:0","b:-1","b:11","b:1x","b:1,","b:1:2"}){rejects<std::invalid_argument>([&]{(void)parse_allocations(s);});}});
    test("bounded serialization exact boundary",[]{std::ostringstream out;write_bounded(out,3,[](std::ostream& s){s<<"abc";});require(out.str()=="abc");rejects<std::exception>([]{std::ostringstream sink;write_bounded(sink,2,[](std::ostream& s){s<<"abc";});});});
    test("single-character writes obey mixed and zero byte budgets",[]{
        std::ostringstream out;
        write_bounded(out,4,[](std::ostream& s){s.put('a');s.write("bc",2);s.put('d');});
        require(out.str()=="abcd");
        rejects<std::exception>([&]{write_bounded(out,0,[](std::ostream& s){s.put('x');});});
        require(out.str()=="abcd");
        std::ostringstream limited;
        rejects<std::exception>([&]{write_bounded(limited,2,[](std::ostream& s){s.put('a');s.put('b');s.put('c');});});
        require(limited.str()=="ab");
    });
    test("single-character sink failure cannot be reported as success",[]{
        struct RejectingBuffer final : std::streambuf {};
        RejectingBuffer rejected; std::ostream sink(&rejected);
        rejects<std::exception>([&]{write_bounded(sink,1,[](std::ostream& s){s.put('x');});});
    });
    test("publication refuses a destination created during serialization",[]{
        TemporaryDirectory work; const auto target=work.path/"report.json";
        rejects<std::filesystem::filesystem_error>([&]{write_export(target,100,[&](std::ostream& out){
            out<<"completed export"; std::ofstream competitor(target,std::ios::binary); competitor<<"keep competitor";
        });});
        require(read_local_file(target)=="keep competitor");
        require(std::distance(std::filesystem::directory_iterator(work.path),std::filesystem::directory_iterator{})==1);
    });
    test("buffered publication writes all bytes before becoming visible",[]{
        TemporaryDirectory work; const auto target=work.path/"complete.json";
        const std::string payload(20000,'x');
        write_export(target,payload.size(),[&](std::ostream& out){out<<payload;require(!std::filesystem::exists(target));});
        require(read_local_file(target)==payload);
        require(std::distance(std::filesystem::directory_iterator(work.path),std::filesystem::directory_iterator{})==1);
    });
    test("single and bulk writes cross export buffer boundaries exactly",[]{
        TemporaryDirectory work;
        for (const std::size_t size : {8191U,8192U,8193U,16384U}) {
            const auto target=work.path/(std::to_string(size)+".json");
            const std::string bulk(size,'b'); const auto expected=bulk+std::string(1,'\0')+"end";
            write_export(target,expected.size(),[&](std::ostream& out){
                out.write(bulk.data(),static_cast<std::streamsize>(bulk.size()));
                out.put('\0');out.put('e');out.write("nd",2);
                require(!std::filesystem::exists(target));
            });
            std::ifstream input(target,std::ios::binary);
            const std::string actual(std::istreambuf_iterator<char>{input},std::istreambuf_iterator<char>{});
            require(actual==expected);
        }
        const auto over_budget=work.path/"over-budget.json";
        rejects<std::exception>([&]{write_export(over_budget,1,[](std::ostream& out){out.put('a');out.put('b');});});
        require(!std::filesystem::exists(over_budget));
        require(std::distance(std::filesystem::directory_iterator(work.path),std::filesystem::directory_iterator{})==4);
    });
    std::cout<<passed<<" application tests passed; "<<failed<<" failed\n";return failed?1:0;
}
