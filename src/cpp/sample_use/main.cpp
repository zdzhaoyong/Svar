#include "Svar.h"
#include "Registry.h"

using namespace GSLAM;

struct MyClass{
    MyClass(int b=2):a(b){}
    static MyClass create(){return MyClass();}
    void print(){
        std::cerr<<a;
    }

    int a;
};

int main(int argc,char** argv){
    svar.parseMain(argc,argv);

    int argInt=svar.arg<int>("i",0,"This is a demo int parameter");
    svar.arg<bool>("b",false,"Here is the bool description");

    if(svar.get<bool>("help",false)){
        svar.help();
        return 0;
    }
    return 0;
    Svar i=1;
    Svar d=2.;
    Svar s="hello world";
    Svar b=false;
    Svar v={1,2,"s",false};
    Svar m={{"i",1},{"d",d}};

    v[2]="hello";
    m["vec"]=v;
    m["myclsInst"]=MyClass();
    m["mycls"]=SvarClass::instance<MyClass>();
    m["lambda"]=Svar::lambda([](){return "hello world";});
    m["static"]=Svar(&MyClass::create);
    m["print"]=Svar(&MyClass::print);

    std::cout<<m;


    SvarClass helloClass("hello",typeid(SvarObject));
    helloClass
            .def_static("print",[](Svar rh){
        std::cerr<<"print:"<<rh<<std::endl;
    })
    .def("printSelf",[](Svar self){
        std::cerr<<"printSelf:"<<self<<std::endl;
    })
    .def("__init__",[helloClass](){
        Svar ret={{"a",100}};
        ret.as<SvarObject>()._class=helloClass;
        return ret;
    });

    Svar cls(helloClass);
    Svar inst=cls();
    std::cerr<<helloClass;
    inst.call("print","hello print.");
    inst.call("printSelf");


    Svar sampleModule=Registry::load("sample_module");

    Svar ApplicationDemo=sampleModule["ApplicationDemo"];

    Svar instance=ApplicationDemo("zhaoyong");
    std::cout<<instance.call("gslam_version");

    std::cout<<ApplicationDemo.as<SvarClass>();
    return 0;

}
