#include "Svar.h"

using namespace GSLAM;

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
    std::string _name;
};

REGISTER_SVAR_MODULE(sample)// see, so easy, haha
{
    svar["__name__"]="sample_module";
    svar["__doc__"]="This is a demo to show how to export a module using svar.";
    svar["add"]=add;

    Class<ApplicationDemo>()
            .construct<std::string>()
            .def("name",&ApplicationDemo::name)
            .def("gslam_version",&ApplicationDemo::gslam_version);

    svar["ApplicationDemo"]=SvarClass::instance<ApplicationDemo>();
}

EXPORT_SVAR_INSTANCE // export the symbol of Svar::instance
