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
    EXPECT_TRUE(Svar()==Svar::Null());
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
    }).call(std::string("hello"));
    SvarFunction(sayHello).call();
    Svar(sayHello).call();
}

TEST(Svar,Cast){
    EXPECT_EQ(Svar(2.).castAs<int>(),2);
    EXPECT_TRUE(Svar(1).castAs<double>()==1.);
}

TEST(Svar,NumOp){
    EXPECT_EQ(Svar(2.1)+Svar(1),3.1);
    EXPECT_EQ(Svar(4.1)-Svar(2),4.1-2);
    EXPECT_EQ(Svar(3)*Svar(3.3),3*3.3);
    EXPECT_EQ(Svar(5.4)/Svar(2),5.4/2);
    EXPECT_EQ(Svar(5)%Svar(2),5%2);
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
