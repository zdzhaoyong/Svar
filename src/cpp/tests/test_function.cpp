#include "Svar.h"
#include "gtest.h"

using namespace sv;

std::string add(std::string left,const std::string& r){
    return left+r;
}

TEST(Function,Basic)
{
    int intV=0,srcV=0;
    Svar intSvar(0);
    // Svar argument is recommended
    SvarFunction isBool([](const Svar& config){return config.is<bool>();});
    EXPECT_TRUE(isBool.call(false).as<bool>());

    SvarFunction refArg([](Svar& v){return v.is<int>();});
    EXPECT_TRUE(refArg.call(1).as<bool>());

    // pointer
    Svar([](int* ref){*ref=10;})(&intV);
    Svar([](int* ref){*ref=10;})(intSvar);
    EXPECT_EQ(intV,10);
    EXPECT_EQ(intSvar,10);
    // WARNNING!! ref, should pay attention that when using ref it should always input the svar instead of the raw ref
    Svar([](int& ref){ref=20;})(intSvar);
    EXPECT_EQ(intSvar,20);
    // const ref
    Svar([](const int& ref,int* dst){*dst=ref;})(30,&intV);
    EXPECT_EQ(intV,30);
    // const pointer
    Svar([](const int* ref,int* dst){*dst=*ref;})(&srcV,&intV);
    EXPECT_EQ(intV,srcV);
    // overload is only called when cast is not available!
    Svar funcReturn=Svar([](int i){return i;});
    funcReturn.as<SvarFunction>().next=[](char c){return c;};
    EXPECT_EQ(funcReturn(1).cpptype(),typeid(int));
    EXPECT_EQ(funcReturn('c').cpptype(),typeid(char));
    // string argument
    Svar s("hello");
    Svar([](std::string raw,
                 const std::string c,
                 const std::string& cref,
                 std::string& ref,
                 std::string* ptr,
                 const std::string* cptr){
        EXPECT_EQ(raw,"hello");
        EXPECT_EQ(c,"hello");
        EXPECT_EQ(ref,"hello");
        EXPECT_EQ(cref,"hello");
        EXPECT_EQ((*ptr),"hello");
        EXPECT_EQ((*cptr),"hello");
    })(s,s,s,s,s,s);

    // cpp function binding
    EXPECT_EQ(Svar(add)(Svar("a"),std::string("b")).as<std::string>(),"ab");

    // FIXME: Why the below line won't pass and add returned "b"?
    EXPECT_EQ(Svar(add)(Svar("a"),std::string("b")).as<std::string>(),"ab");

    // static method function binding
    EXPECT_TRUE(Svar::Null().isNull());
    EXPECT_TRUE(Svar(&Svar::Null)().isNull());
    EXPECT_TRUE(Svar([](const int& a){return a;})(std::make_shared<int>(1))==1);
    EXPECT_TRUE(Svar([](std::shared_ptr<int> a){return *a;})(std::make_shared<int>(1))==1);

    Svar var=[](std::string v){};
    var("hello world");
}

int add_int(int a,int b){return a+b;}

TEST(Function,Overload){
    Svar kw_f(add_int,"a"_a,"b"_a=0,"add two int");
    EXPECT_EQ(kw_f(1,2),3); // call with arguments
    EXPECT_EQ(kw_f(3),3); // b default is 0
    EXPECT_EQ(kw_f("b"_a=1,"a"_a=2),3); // kwargs

    kw_f.overload([](int a,int b,int c){
        return a+b+c;
    }); // overload is supported
    EXPECT_EQ(kw_f(1,2,3),6);
}

TEST(Function,KWARGS){
    Svar module;
    module.def("add",[](int a,int b){return a+b;},"a"_a,"b"_a=0,"Add two int");
    EXPECT_EQ(module["add"](1,2),3);
    EXPECT_EQ(module["add"](1),1);
    EXPECT_EQ(module["add"](1,"b"_a=2),3);
    EXPECT_EQ(module["add"]("b"_a=2,"a"_a=3),5);

    EXPECT_THROW(module["add"]("b"_a=1),SvarExeption); // a is not defined
}
