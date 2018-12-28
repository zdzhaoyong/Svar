#include "Svar.h"
#include "Jvar.h"
#include "gtest.h"
#include <GSLAM/core/VecParament.h>
#include <GSLAM/core/Timer.h>

using namespace GSLAM;

TEST(Svar,SvarCreate)
{
    Py::Svar var(false);
    std::cerr<<"TypeName:"<<var.typeName();
    EXPECT_TRUE(Py::Svar()==Py::Svar::Null());
    EXPECT_TRUE(var.is<bool>());
    EXPECT_FALSE(var.as<bool>());
    EXPECT_TRUE(Py::Svar(1).is<int>());
    EXPECT_TRUE(Py::Svar(1).as<int>()==1);
    EXPECT_TRUE(Py::Svar("").as<std::string>().empty());
    EXPECT_TRUE(Py::Svar(1.).as<double>()==1.);
    EXPECT_TRUE(Py::Svar(std::vector<int>({1,2})).isArray());
    EXPECT_TRUE(Py::Svar(std::map<int,Py::Svar>({{1,2}})).isDict());

    Py::Svar obj(std::map<std::string,int>({{"1",1}}));
    EXPECT_TRUE(obj.isObject());
    EXPECT_TRUE(obj["1"]==1);

}

int main(int argc,char** argv){
    svar.Arg<int>("argInt",100,"this is a sample int argument");
    svar.Arg<double>("argDouble",100.,"this is a sample double argument");
    svar.Arg<std::string>("argString","hello","Sample string argument");
    svar.Arg<bool>("argBool",false,"Sample bool argument");
    svar.Arg<bool>("child.bool",true,"Child bool");
    VecParament<std::string> unParsed=svar.ParseMain(argc,argv);
    if(svar.Get<bool>("help")){
        std::cerr<<svar.help();
        return 0;
    }
    std::cerr<<"Unparsed arguments:"<<unParsed;


    testing::InitGoogleTest(&argc,argv);
    auto ret= RUN_ALL_TESTS();
    return 0;
}
