#include "Svar.h"

using namespace sv;

int add(int a,int b){
    return a+b;
}

class Person{
public:
    Person(int age,std::string name)
        : _age(age),_name(name){
        all_person()[name]=_age;
    }

    virtual std::string intro()const{
        return Svar({{"Person",{_age,_name}}}).dump_json();
    }

    int age()const{return _age;}

    static Svar& all_person(){
        static Svar all;
        return all;
    }

    int _age;
    std::string _name;
};

class Student: public Person{
public:
    Student(int age,std::string name,std::string school)
        : Person(age,name),_school(school){}

    virtual std::string intro()const{
        return Svar({{"Student",{_age,_name,_school}}}).dump_json();
    }

    void setSchool(const std::string& school){
        _school=school;
    }

    std::string _school;
};

Svar call_func(Svar func,Svar a){
    switch (a.size()) {
    case 0:
        return func();
        break;
    case 1:
        return func(a[0]);
        break;
    case 2:
        return func(a[0],a[1]);
        break;
    case 3:
        return func(a[0],a[1],a[2]);
        break;
    case 4:
        return func(a[0],a[1],a[2],a[3]);
        break;
    default:
        return Svar();
        break;
    }
}

Svar use_module(Svar module){
    std::cout<<module<<std::endl;
    Svar uname=module["sayHello"];
    if(uname.isFunction())
        return uname();
    return Svar();
}

REGISTER_SVAR_MODULE(sample)// see, so easy, haha
{
    svar["__name__"]="sample_module";
    svar["__doc__"]="This is a demo to show how to export a module using svar.";
    svar["add"]=add;
    svar["call_func"]=call_func;
    svar["use_module"]=use_module;

    Class<Person>("Person","The base class")
            .construct<int,std::string>()
            .def("intro",&Person::intro)
            .def_static("all",&Person::all_person)
            .def("age",&Person::age)
            .def_readonly("name",&Person::_name,"The name of a person");

    Class<Student>("Student","The derived class")
            .construct<int,std::string,std::string>()
            .inherit<Person>()
            .def("intro",&Student::intro)
            .def("setSchool",&Student::setSchool)
            .def("getSchool",[](Student& self){return self._school;})
            .def_readwrite("school",&Student::_school,"The school of a student");
}

EXPORT_SVAR_INSTANCE // export the symbol of Svar::instance
