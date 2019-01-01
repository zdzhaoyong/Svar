#include "Svar.h"
#include "Jvar.h"
#include "gtest.h"
#include <GSLAM/core/VecParament.h>
#include <GSLAM/core/Timer.h>

using namespace Py;

TEST(Svar,SvarCreate)
{
    Svar var(false);
    std::cerr<<"TypeName:"<<var.typeName();
    EXPECT_TRUE(Svar()==Svar::Undefined());
    EXPECT_TRUE(var.is<bool>());
    EXPECT_FALSE(var.as<bool>());
    EXPECT_TRUE(Svar(1).is<int>());
    EXPECT_TRUE(Svar(1).as<int>()==1);
    EXPECT_TRUE(Svar("").as<std::string>().empty());
    EXPECT_TRUE(Svar(1.).as<double>()==1.);
    EXPECT_TRUE(Svar(std::vector<int>({1,2})).isArray());
    EXPECT_TRUE(Svar(std::map<int,Svar>({{1,2}})).isDict());

    Svar obj(std::map<std::string,int>({{"1",1}}));
    EXPECT_TRUE(obj.isObject());
    EXPECT_TRUE(obj["1"]==1);
}

TEST(Svar,GetSet){
    Svar var;
    int& testInt=var.GetInt("child.testInt",20);
    EXPECT_EQ(testInt,20);
    testInt=30;
    EXPECT_EQ(var.GetInt("child.testInt"),30);
    var.Set("child.testInt",40);
    EXPECT_EQ(testInt,40);
}

void sayHello(){
    std::cout<<"Hello called"<<std::endl;
}

TEST(Svar,SvarFunction)
{
    using namespace std::placeholders;

    SvarFunction isBool([](Svar config){return config.is<bool>();},_1);
    EXPECT_TRUE(isBool.call(false).as<bool>());
    SvarFunction([](std::string name){
        std::cerr<<name<<std::endl;
    }).call("hello");
    SvarFunction(sayHello).call();
    Svar(sayHello).call();

    int intV=0,srcV=0;
    Svar intSvar(0);
    // pointer
    Svar::lambda([](int* ref){*ref=10;})(&intV);
    Svar::lambda([](int* ref){*ref=10;})(intSvar);
    EXPECT_EQ(intV,10);
    EXPECT_EQ(intSvar,10);
    // ref, should pay attention that when using ref it should always input the svar instead of the raw ref
    Svar::lambda([](int& ref){ref=20;})(intSvar);
    EXPECT_EQ(intSvar,20);
    // const ref
    Svar::lambda([](const int& ref,int* dst){*dst=ref;})(30,&intV);
    EXPECT_EQ(intV,30);
    // const pointer
    Svar::lambda([](const int* ref,int* dst){*dst=*ref;})(&srcV,&intV);
    EXPECT_EQ(intV,srcV);
}

TEST(Svar,Cast){
    EXPECT_EQ(Svar(2.).castAs<int>(),2);
    EXPECT_TRUE(Svar(1).castAs<double>()==1.);
}

TEST(Svar,NumOp){
    EXPECT_EQ(-Svar(2.1),-2.1);
    EXPECT_EQ(-Svar(2),-2);
    EXPECT_EQ(Svar(2.1)+Svar(1),3.1);
    EXPECT_EQ(Svar(4.1)-Svar(2),4.1-2);
    EXPECT_EQ(Svar(3)*Svar(3.3),3*3.3);
    EXPECT_EQ(Svar(5.4)/Svar(2),5.4/2);
    EXPECT_EQ(Svar(5)%Svar(2),5%2);
    EXPECT_EQ(Svar(5)^Svar(2),5^2);
    EXPECT_EQ(Svar(5)|Svar(2),5|2);
    EXPECT_EQ(Svar(5)&Svar(2),5&2);
}

TEST(Svar,Dump){
    return;
    Svar obj(std::map<std::string,int>({{"1",1}}));
    std::cout<<obj<<std::endl;
    std::cout<<Svar::Array({1,2,3})<<std::endl;
    std::cout<<Svar::create((std::type_index)typeid(1))<<std::endl;
    std::cout<<Svar::lambda([](std::string sig){})<<std::endl;
    std::cout<<SvarClass::Class<int>();
    std::cout<<Svar::instance();
}

TEST(Svar,Iterator){
    auto vec=Svar::Array({1,2,3});
    Svar arrayiter=vec.classPtr()->__iter__(vec);
    try{
    while(true){
        std::cout<<arrayiter.classPtr()->_methods["next"](arrayiter);;
    }}
    catch (SvarIterEnd){}
}

int main(int argc,char** argv){
    svar.Arg<int>("argInt",100,"this is a sample int argument");
    svar.Arg<double>("argDouble",100.,"this is a sample double argument");
    svar.Arg<std::string>("argString","hello","Sample string argument");
    svar.Arg<bool>("argBool",false,"Sample bool argument");
    svar.Arg<bool>("child.bool",true,"Child bool");
    GSLAM::VecParament<std::string> unParsed=svar.ParseMain(argc,argv);
    if(svar.Get<bool>("help")){
        std::cerr<<svar.help();
        return 0;
    }
    std::cerr<<"Unparsed arguments:"<<unParsed;


    testing::InitGoogleTest(&argc,argv);
    auto ret= RUN_ALL_TESTS();
    return 0;
}
