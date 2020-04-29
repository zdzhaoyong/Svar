#include "Svar.h"
#include "gtest.h"
#include "Timer.h"

using namespace sv;

TEST(Svar,Thread){
    auto doubleThread=[](){
        while(!svar.get<bool>("shouldstop",false)){
            double d=svar.get<double>("thread.Double",100);
            svar.set<double>("thread.Double",d++);

            // double& d=svar.get<double>("thread.Double",100); d++;
            // The above line is dangerous in Mac system since the value may set after memory free

            Svar v=svar.get("thread.Double",100);
            v=100;// this is much safer to set a copy of svar instead the raw pointer

            GSLAM::Rate::sleep(1e-4);
        }
    };
    auto intThread=[](){
        while(!svar.get<bool>("shouldstop",false)){
            // use int instead of double to violately testing robustness
            int i=svar.get<int>("thread.Double",10);
            svar.set<int>("thread.Double",i);

            GSLAM::Rate::sleep(1e-4);
        }
    };
    std::vector<std::thread> threads;
    for(int i=0;i<svar.GetInt("doubleThreads",4);i++) threads.push_back(std::thread(doubleThread));
    for(int i=0;i<svar.GetInt("intThreads",4);i++) threads.push_back(std::thread(intThread));

    GSLAM::Rate::sleep(1);
    svar.set("shouldstop",true);
    for(auto& it:threads) it.join();
}
