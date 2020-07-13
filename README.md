# Svar, A Tiny Modern C++ Header Brings Unified Interface for Different Languages

---

[![Build Status](https://travis-ci.org/zdzhaoyong/Svar.svg?branch=master)](https://travis-ci.org/zdzhaoyong/Svar)
[![Build Status](https://circleci.com/gh/zdzhaoyong/Svar.svg?style=svg)](https://circleci.com/gh/zdzhaoyong/Svar)
[![Codacy Badge](https://api.codacy.com/project/badge/Grade/9f6efa9443de498fa91a4e99f632dbd2)](https://www.codacy.com/app/zdzhaoyong/Svar)
[![License](https://img.shields.io/badge/license-BSD--2--Clause-blue.svg)](./LICENSE)
[![Version](https://img.shields.io/github/release/zdzhaoyong/Svar.svg)](https://github.com/zdzhaoyong/Svar/releases)

- [Why Svar](#why-svar)
- [Compile and Install](#compile-and-install)
- [Usages](#usages)
  - [Perform tests](#perform-tests)
  - [Use Svar like JSON](#use-svar-like-json)
  - [Use Svar for Argument Parsing](#use-svar-for-argument-parsing)
  - [Svar Holding Everything](#svar-holding-everything)
    - [Hold Class Instances](#hold-class-instances)
    - [Hold Functions](#hold-functions)
    - [Hold and Use a Class](#hold-and-use-a-class)
  - [Create and Use a Svar Module](#create-and-use-a-svar-module)
    - [Create a Svar Module](#create-a-svar-module)
    - [The Svar Module Documentation](#the-svar-module-documentation)
    - [Import and Use a Svar Module](#import-and-use-a-svar-module)
  - [Use Svar Module in Other Languages](#use-svar-module-in-other-languages)
- [Supported Compilers](#supported-compilers)
- [Contact and Donation](#contact-and-donation)
- [License](#license)

---

# Why Svar? 

Svar is a powerful tiny modern c++ header implemented the following functionals:

- JSON with buffer: Svar natually support JSON, but more than JSON, where JSON is only a subset of Svar data structure.
- Argument parsing: Svar manages parameter in tree style, so that configuration in JSON format can be supported.
- More than std::any after c++17: Svar is able to hold everything, including variables, functions, and classes. They can be used directly without declare header, just like writing Python or JavaScript!
- A general plugin form. The released library comes with documentation, making *.h header file interface description unnecessary. 
- Auto multi-languages api. By Using Svar, your C++ library can be called easily with different languages like C++, Java, Python and Javascript.


# Compile and Install

Obtain source code from github

```
git clone https://github.com/zdzhaoyong/Svar
```

Compile the source code with cmake:
```
cd Svar
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make
sudo make install
```

# Usages

## Perform tests

Usage help:
```
svar --help
```

```
svar tests
```

## Use Svar like JSON

Svar natually support JSON and here is some basic usage demo:

```
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
        std::cout<<"raw string is "<<s.as<std::string>()<<std::endl; // use as to cast, never throw

    double db=i.castAs<double>();// use castAs, this may throw SvarException

    for(auto it:v) std::cout<<it<<std::endl;

    for(std::pair<std::string,Svar> it:m) std::cout<<it.first<<":"<<it.second<<std::endl;

    std::string json=m.dump_json();
    m=Svar::parse_json(json);
```

You can run the above code with:
```
svar sample -json
```

## Use Svar for Argument Parsing

Svar provides argument parsing functional with JSON configuration loading.
Here is a small demo shows how to use Svar for argument parsing:

```

int main(int argc,char** argv)
{
    svar.parseMain(argc,argv);

    int  argInt=svar.arg<int>("i",0,"This is a demo int parameter");
    bool bv=svar.arg("b.dump",false,"Svar supports tree item assign");
    Svar m =svar.arg("obj",Svar(),"Svar support json parameter");

    if(svar.get<bool>("help",false)){
        svar.help();
        return 0;
    }
    if(svar["b"]["dump"].as<bool>())
        std::cerr<<svar;
    return 0;
}

```

When you use "--help" or "-help", the terminal should show the below introduction:

```
-> sample_use --help
Usage:
sample_use [--help] [-conf configure_file] [-arg_name arg_value]...

Using Svar supported argument parsing. The following table listed several argume
nt introductions.

Argument        Type(default->setted)           Introduction
--------------------------------------------------------------------------------
-i              int(0)                          This is a demo int parameter
-b.dump         bool(false)                     Svar supports tree item assign
-obj            svar(undefined)                 Svar support json parameter
-conf           str("Default.cfg")              The default configure file going
                                                 to parse.
-help           bool(false->true)               Show the help information.
```

"help" and "conf" is two default parameters and users can use "conf" to load JSON file for configuration loading.


Svar supports the following parsing styles:

- "-arg value": two '-' such as "--arg value" is the same
- "-arg=value": two "--" such as "--arg=value" is the same
- "arg=value"
- "-arg" : this is the same with "arg=true", but the next argument should not be a value

Svar supports to use brief Json instead of strict Json:

```
sample_use -b.dump -obj '{a:1,b:false}'
```

## Svar Holding Everything

### Hold Class Instances

Svar support to hold different values:

```
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
```

Run the above sample with 'svar sample -instance'.

### Hold Functions

```
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
```

Run the above sample with 'svar sample -func'.

### Hold and use a Class

```

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

{
    // define the class to svar
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

    // use the class with svar
    Svar Person=svar["Person"];
    Svar Student=svar["Student"];
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
}
```

Run the above code with "svar sample -module"

## Create and Use a Svar Module
The last page shows that we can use functions and classes with Svar.
What if we expose only Svar instead of headers for a c++ shared library?
Yes we don't need headers any more!
We call that shared library Svar module, which can be imported and call directly.

### Create a Svar module

```
#include "Svar.h"

using namespace GSLAM;

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

REGISTER_SVAR_MODULE(sample)// see, so easy, haha
{
    svar["__name__"]="sample_module";
    svar["__doc__"]="This is a demo to show how to export a module using svar.";
    svar["add"]=add;

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

```

We compile the above code to a shared library "libsample_module.so" or "sample_module.dll".

### The Svar Module Documentation
Show the context of the sample plugin module:

```
# Show the structure of a plugin
svar doc -plugin libsample_module.so
# Show the documentation of a named class
svar doc -plugin libsample_module.so -key Person
# Show the documentation of a named function
svar doc -plugin libsample_module.so -key add
```

### Import and Use a Svar Module

For the Svar module, we can import and use them in different languages!
Here we show how to use it in c++, source code is available in file "src/sample/main.cpp".

```
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
```

Run the above code with "svar sample -module".

## Use Svar Module in Other Languages

The last page shows how to use a Svar module in c++.
Can the module also be imported by other languages?

Yes!Yes!Yes!
But we only implemented the python3 interface now, if you are familar with bindings, please contact me and we can cooperate on this!

### Use the Sample Module with Python

Firstly, please install the svar module with pip3:

```
cd Svar
pip3 install .
```

Then, we can run the following python script with python3:

```
import svar

s=svar.load('sample_module')

father=s.Person(40,'father') # constructor
mother=s.Person(39,'mother')
sister=s.Student(15,'sister','high')
me=s.Student(13,'me','juniar')

me.setSchool('school1')

print("all:",s.Person.all())

print(father.intro())
print(sister.intro())
print(mother.age())
#print(me.age()) # TODO: derivation is not implemented yet by the python binding
print(sister.getSchool())
print(me.getSchool())

print(father.name) # property
print(me.school)
```

### Use the Sample Module with NodeJS (Tested on Ubuntu 16.04)

Firstly, please install the svar module with npm:

```
cd Svar/src/nodejs
npm install .
```

Then, we can run the demo javascript with node:

```
node sample_module.js
```

The file content is listed as below:

```
s    = require('bindings')('svar')('sample_module')

console.log(s)

p = new s.Person(3,'zy')
console.log(p.age())
console.log(s.Person.all())

student = new s.Student(3,"zy","npu")

console.log(student.school,student.intro())

```

# Supported Compilers
Currently, the following compilers are known to work:

GCC 4.8 - 9.2 (and possibly later)

Clang 3.4 - 9.0 (and possibly later)

Intel C++ Compiler 17.0.2 (and possibly later)

Microsoft Visual C++ 2015 / Build Tools 14.0.25123.0 (and possibly later)

Microsoft Visual C++ 2017 / Build Tools 15.5.180.51428 (and possibly later)

Microsoft Visual C++ 2019 / Build Tools 16.3.1+1def00d3d (and possibly later)


# Contact and Donation

Any question or suggestion please contact:

Yong Zhao : *zdzhaoyong@mail.nwpu.edu.cn*



Svar is a personal free project with BSD license, if you like it, donation is accepted with the following approaches to support the author:


|     Wechat    |      Alipay         |  PayPal   |
| :--------: | :-----------: | :-------: |
| <img src="http://zhaoyong.win/resource/weixin.png" width = "40%" /> | <img src="http://zhaoyong.win/resource/alipay.png" width = "60%" /> | [![paypal](https://www.paypalobjects.com/en_US/i/btn/btn_donateCC_LG.gif)](https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=48AGPL9FJX2D6) |


# License

```
Copyright (c) 2018 Northwestern Polytechnical University, Yong Zhao. All rights reserved.

This software was developed by the Yong Zhao at Northwestern Polytechnical University.
Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list
of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this
list of conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.
3. All advertising materials mentioning features or use of this software must
display the following acknowledgement: This product includes software developed
by Northwestern Polytechnical University and its contributors.
4. Neither the name of the University nor the names of its contributors may be
used to endorse or promote products derived from this software without specific
prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
```






