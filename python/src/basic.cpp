#define __SVAR_BUILDIN__
#include "../../src/Svar.h"

GSLAM::Svar obtainInfo(GSLAM::Svar pyobj){
    std::cerr<<"Calling obtainInfo"<<std::endl;
    std::cout<<"Argument is "<<pyobj;
    GSLAM::Svar ret=GSLAM::Svar::object({{"1",2.},{"buildin",std::vector<GSLAM::Svar>({1,2,3})}});
    return ret;
}

std::string dtos(double& d){
    return std::to_string(d);
}

int add(int a,int b){
    return a+b;
}

REGISTER_SVAR_MODULE(basic)
{
    svar.def("obtainInfo",obtainInfo);
    svar.def("dtos",dtos);
    svar.def("add",add);
}
