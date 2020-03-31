#include "Svar.h"
#include "Registry.h"

using namespace sv;

int sample_json(Svar config){
    Svar null=nullptr;
    Svar b=false;
    Svar i=1;
    Svar d=2.1;
    Svar s="hello world";
    Svar v={1,2,3};
    Svar m={{"b",false},{"s","hello world"},{"n",nullptr},{"u",Svar()}};

    Svar obj;
    obj["m"]=m;
    obj["pi"]=3.14159;

    std::cout<<obj<<std::endl;

    if(s.is<std::string>()) // use is to check type
        std::cout<<"raw string is "<<s.as<std::string>()<<std::endl;

    double db=i.castAs<double>();// use castAs, this may throw SvarException

    for(auto it:v) std::cout<<it<<std::endl;

    for(std::pair<std::string,Svar> it:m) std::cout<<it.first<<":"<<it.second<<std::endl;

    std::string json=m.dump_json();
    m=Svar::parse_json(json);
    return 0;
}

void c_func_demo(Svar arg){
    std::cout<<"Calling a c_func with arg "<<arg<<std::endl;
}

int sample_func(){
    Svar c_func=c_func_demo;
    c_func("I love Svar");

    Svar lambda=Svar::lambda([](std::string arg){std::cout<<arg<<std::endl;});
    lambda("Using a lambda");

    Svar member_func(&SvarBuffer::md5);
    std::cout<<"Obtain md5 with member function: "
            <<member_func(SvarBuffer(3)).as<std::string>()<<std::endl;

    return 0;
}

int sample_instance()
{
    struct A{int a,b,c;}; // define a struct

    A a={1,2,3};
    Svar avar=a;      // support directly value copy assign
    Svar aptrvar=&a;  // support pointer value assign
    Svar uptrvar=std::unique_ptr<A>(new A({2,3,4}));
    Svar sptrvar=std::shared_ptr<A>(new A({2,3,4})); // support shared_ptr lvalue

    std::cout<<avar.as<A>().a<<std::endl;
    std::cout<<aptrvar.as<A>().a<<std::endl;
    std::cout<<uptrvar.as<A>().a<<std::endl;
    std::cout<<sptrvar.as<A>().a<<std::endl;

    std::cout<<avar.castAs<A*>()->b<<std::endl;
    std::cout<<aptrvar.as<A*>()->b<<std::endl;
    std::cout<<uptrvar.castAs<A*>()->b<<std::endl;
    std::cout<<sptrvar.castAs<A*>()->b<<std::endl;

    std::cout<<uptrvar.as<std::unique_ptr<A>>()->b<<std::endl;
    std::cout<<sptrvar.as<std::shared_ptr<A>>()->b<<std::endl;
    return 0;
}

int sample_cppclass()
{
    // 1. define a class with c++
    struct MyClass{
        MyClass(int b=2):a(b){}
        static MyClass create(){return MyClass();}
        void print(){
            std::cerr<<a<<std::endl;
        }

        int a;
    };

    // 2. define the class members to Svar
    Class<MyClass>("DemoClass")// rename to DemoClass?
            .construct<int>()
            .def("print",&MyClass::print)
            .def_static("create",&MyClass::create)
            .def_readwrite("a",&MyClass::a);

    // 3. use the class with c++ or other languages
    Svar DemoClass=SvarClass::instance<MyClass>();

    Svar inst=DemoClass(3);// construct a instance
    inst.call("print");

    Svar inst2=DemoClass.call("create");
    int  a=inst2.get<int>("a",0); // const & is essential to correctly get property
    std::cout<<a<<std::endl;

    return 0;
}

int sample_svarclass(){// users can also define a class without related c++ class
    // 1. Directly define a class in Svar
    SvarClass helloClass("hello",typeid(SvarObject));
    helloClass.def_static("print",[](Svar rh){
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
    return 0;
}

int sample_module(Svar config){
    Svar sampleModule=Registry::load("sample_module");

    Svar Person=sampleModule["Person"];
    Svar Student=sampleModule["Student"];
    std::cout<<Person.as<SvarClass>();
    std::cout<<Student.as<SvarClass>();

    Svar father=Person(40,"father");
    Svar mother=Person(39,"mother");
    Svar sister=Student(15,"sister","high");
    Svar me    =Student(13,"me","juniar");
    me.call("setSchool","school1");

    std::cout<<"all:"<<Person.call("all")<<std::endl;
    std::cout<<father.call("intro").as<std::string>()<<std::endl;
    std::cout<<sister.call("intro").as<std::string>()<<std::endl;
    std::cout<<mother.call("age")<<std::endl;
    std::cout<<me.call("age")<<std::endl;
    std::cout<<sister.call("getSchool")<<std::endl;
    std::cout<<me.call("getSchool")<<std::endl;

    me.set<std::string>("school","school2");
    std::cout<<me.get<std::string>("school","")<<std::endl;

    return 0;
}

int sample(Svar config){
    bool json=config.arg("json",false,"run json sample");
    bool func=config.arg("func",false,"run function usage sample");
    bool cppclass=config.arg("cppclass",false,"run cpp class sample");
    bool svarclass=config.arg("svarclass",false,"run svar class sample");
    bool module=config.arg("module",false,"show how to use a svar module");
    bool instance=config.arg("instance",false,"use svar to hold values");

    if(config.get("help",false)) return config.help();

    if(json) return sample_json(config);
    if(func) return sample_func();
    if(cppclass) return sample_cppclass();
    if(svarclass) return sample_svarclass();
    if(module) return sample_module(config);
    if(instance) return sample_instance();

    return 0;
}

REGISTER_SVAR_MODULE(sample){
    svar["apps"]["sample"]={sample,"This is a collection of sample to show Svar usages"};
}
