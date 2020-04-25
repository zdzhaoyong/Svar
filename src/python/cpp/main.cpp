#include <Svar/Registry.h>

using namespace sv;

Svar python=Registry::load("svarpy");
const auto os=python["import"]("os");
const auto math=python["import"]("math");

int main(int argc,char** argv){
    std::cerr<<math<<std::endl;
    

    std::thread thread([os,math](){
        std::cerr<<"PID from work thread: "<<os["getpid"]()<<std::endl;
        std::cerr<<"sqrt(100) from work thread: "<<math["sqrt"](100)<<std::endl;
    });

    thread.join();

    return 0;
}

