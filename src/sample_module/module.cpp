#define __SVAR_BUILDIN__
#include "Svar.h"

GSLAM::Svar obtainInfo(GSLAM::Svar pyobj){
    return pyobj;
}

std::string dtos(double& d){
    return std::to_string(d);
}

int add(int a,int b){
    return a+b;
}

class ApplicationDemo{
public:
    ApplicationDemo(std::string name):_name(name){
        std::cout<<"Application Created.";
    }
    std::string name()const{return _name;}

    std::string gslam_version()const{return "3.2";}

    std::string introduction()const{
        return "";
    }
    std::string _name;
};

REGISTER_SVAR_MODULE(sample)
{
    svar.def("obtainInfo",obtainInfo);
    svar.def("dtos",dtos);
    svar.def("add",add);
    svar["__name__"]=std::string("sample_module");
    svar.Set<std::string>("__doc__","This is a demo to show how to export a module using svar.");


    using namespace GSLAM;
    Class_<ApplicationDemo>()
            .construct<std::string>()
            .def("name",&ApplicationDemo::name)
            .def("introduction",&ApplicationDemo::introduction)
            .def("gslam_version",&ApplicationDemo::gslam_version);

    svar.set("ApplicationDemo",SvarClass::instance<ApplicationDemo>());
}
