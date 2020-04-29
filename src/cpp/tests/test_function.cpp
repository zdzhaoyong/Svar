#include "Svar.h"
#include "gtest.h"

using namespace sv;

std::string add(std::string left,const std::string& r){
    return left+r;
}

TEST(Svar,Function)
{
    int intV=0,srcV=0;
    Svar intSvar(0);
    // Svar argument is recommended
    SvarFunction isBool([](const Svar& config){return config.is<bool>();});
    EXPECT_TRUE(isBool.call(false).as<bool>());

    SvarFunction refArg([](Svar& v){return v.is<int>();});
    EXPECT_TRUE(refArg.call(1).as<bool>());

    // pointer
    Svar::lambda([](int* ref){*ref=10;})(&intV);
    Svar::lambda([](int* ref){*ref=10;})(intSvar);
    EXPECT_EQ(intV,10);
    EXPECT_EQ(intSvar,10);
    // WARNNING!! ref, should pay attention that when using ref it should always input the svar instead of the raw ref
    Svar::lambda([](int& ref){ref=20;})(intSvar);
    EXPECT_EQ(intSvar,20);
    // const ref
    Svar::lambda([](const int& ref,int* dst){*dst=ref;})(30,&intV);
    EXPECT_EQ(intV,30);
    // const pointer
    Svar::lambda([](const int* ref,int* dst){*dst=*ref;})(&srcV,&intV);
    EXPECT_EQ(intV,srcV);
    // overload is only called when cast is not available!
    Svar funcReturn=Svar::lambda([](int i){return i;});
    funcReturn.as<SvarFunction>().next=Svar::lambda([](char c){return c;});
    EXPECT_EQ(funcReturn(1).cpptype(),typeid(int));
    EXPECT_EQ(funcReturn('c').cpptype(),typeid(char));
    // string argument
    Svar s("hello");
    Svar::lambda([](std::string raw,
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
    EXPECT_TRUE(Svar::lambda([](const int& a){return a;})(std::make_shared<int>(1))==1);
    EXPECT_TRUE(Svar::lambda([](std::shared_ptr<int> a){return *a;})(std::make_shared<int>(1))==1);
}

TEST(Svar,Call){
    EXPECT_EQ(Svar(1).call("__str__"),"1");// Call as member methods
    EXPECT_THROW(SvarClass::instance<int>().call("__str__",1),SvarExeption);// Call as static function
}
