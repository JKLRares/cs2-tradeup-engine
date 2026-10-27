#include "tradeup/application.hpp"

#include <functional>
#include <iostream>
#include <limits>
#include <sstream>

using namespace tradeup;
namespace {
void require(const bool condition){if(!condition){throw std::runtime_error("check failed");}}
template<class Exception,class F>void rejects(F f){try{f();}catch(const Exception&){return;}throw std::runtime_error("expected exception");}
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
    std::cout<<passed<<" application tests passed; "<<failed<<" failed\n";return failed?1:0;
}
