#include "Svar.h"
#include "gtest.h"

using namespace sv;

class BBase{
public:
    virtual bool isBBase(){return true;}
};

class BaseClass:public BBase{
public:
    BaseClass():age_(0){}
    BaseClass(int age):age_(age){}

    int getAge()const{return age_;}
    void setAge(int a){age_=a;}

    static BaseClass create(int a){return BaseClass(a);}
    static BaseClass create1(){return BaseClass();}

    virtual std::string intro()const{return "age:"+Svar::toString(age_);}
    virtual bool isBBase(){return false;}

    int age_;
};

class BaseClass1{
public:
    BaseClass1():score(100){}

    double getScore(){return score;}

    double score;
};

class InheritClass: public BaseClass,public BaseClass1
{
public:
    InheritClass(int age,std::string name):BaseClass(age),name_(name){}

    std::string name()const{return name_;}
    void setName(std::string name){name_=name;}

    virtual std::string intro() const{return BaseClass::intro()+", name:"+name_;}
    std::string name_;
};

TEST(Svar,Class){

    SvarClass::Class<BBase>()
            .def("isBBase",&BBase::isBBase);

    SvarClass::Class<BaseClass>()
            .inherit<BBase,BaseClass>()
            .def_static("__init__",[](){return BaseClass();})
            .def_static("__init__",[](int age){return BaseClass(age);})
            .def("getAge",&BaseClass::getAge)
            .def("setAge",&BaseClass::setAge)
            .def_static("create",&BaseClass::create)
            .def_static("create1",&BaseClass::create1)
            .def("intro",&BaseClass::intro);
    Svar a=BaseClass(10);
    Svar baseClass=a.classObject();
    a=baseClass();//
    a=baseClass(10);//
    EXPECT_EQ(a.call("getAge").as<int>(),10);
    EXPECT_EQ(a.call("intro").as<std::string>(),BaseClass(10).intro());

    SvarClass::Class<BaseClass1>()
            .def("getScore",&BaseClass1::getScore);

    SvarClass::Class<InheritClass>()
            .inherit<BaseClass,InheritClass>()
            .inherit<BaseClass1,InheritClass>()// for none first parents
            .def("__init__",[](int age,std::string name){return InheritClass(age,name);})
            .def("name",&InheritClass::name)
            .def("intro",&InheritClass::intro)
            .def_readonly("n",&InheritClass::name_,"the name");

    Svar b=InheritClass(10,"xm");
    EXPECT_EQ(b.call("getAge").as<int>(),10);
    EXPECT_EQ(b.call("name").as<std::string>(),"xm");
    const Svar& c=b;// FIXME: why need explicit const&
    std::string name=c["n"].as<std::string>();
    EXPECT_EQ(name,"xm");
    EXPECT_EQ(b.call("intro").as<std::string>(),InheritClass(10,"xm").intro());
    b.call("setAge",20);
    EXPECT_EQ(b.call("getAge").as<int>(),20);
    EXPECT_EQ(b.call("getScore").as<double>(),100.);
    EXPECT_EQ(b.call("isBBase").as<bool>(),false);
    InheritClass& inst=b.as<InheritClass>();
    EXPECT_EQ(Svar(&inst).call("getAge").as<int>(),20);
    InheritClass instCopy=Svar(&inst).as<InheritClass>();
    EXPECT_EQ(instCopy.name(),inst.name());
}
