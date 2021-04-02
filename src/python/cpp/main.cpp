#include <Svar/Svar.h>

using namespace sv;

Svar python=svar.import("svarpy");

int main(int argc,char** argv){

    const Svar os=python["import"]("os");
    const Svar math=python["import"]("math");
    std::cerr<<math<<std::endl;
    std::thread thread([os,math](){
        std::cerr<<"PID from work thread: "<<os["getpid"]()<<std::endl;
        std::cerr<<"sqrt(100) from work thread: "<<math["sqrt"](100)<<std::endl;
    });

    thread.join();

    return 0;
}

