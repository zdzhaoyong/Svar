//#include "Svar.h"
#include "Jvar.h"
#include "gtest.h"
#include <GSLAM/core/Timer.h>

using namespace GSLAM;

class NoCopyV{
public:
    NoCopyV(int v):value(v){}

//    NoCopyV(const NoCopyV& v):value(v.value){}
    int value;
    NoCopyV & operator =(const NoCopyV &v) = delete;
    NoCopyV(const NoCopyV &) = delete;
};

TEST(Svar,Variable)
{
    Svar var(false);
    EXPECT_EQ(var.typeName(),"bool");
    EXPECT_TRUE(Svar()==Svar::Undefined());
    EXPECT_TRUE(var.is<bool>());
    EXPECT_FALSE(var.as<bool>());
    EXPECT_TRUE(Svar(1).is<int>());
    EXPECT_TRUE(Svar(1).as<int>()==1);
    EXPECT_TRUE(Svar("").as<std::string>().empty());
    EXPECT_TRUE(Svar(1.).as<double>()==1.);
    EXPECT_TRUE(Svar(std::vector<int>({1,2})).isArray());
    EXPECT_TRUE(Svar(std::map<int,Svar>({{1,2}})).isDict());

    Svar obj(std::map<std::string,int>({{"1",1}}));
    EXPECT_TRUE(obj.isObject());
    EXPECT_TRUE(obj["1"]==1);
//    NoCopyV v(3);
//    Svar NoCopyVar=Svar::create(3);

}

std::string add(std::string left,const std::string& r){
    return left+r;
}

TEST(Svar,Function)
{
    int intV=0,srcV=0;
    Svar intSvar(0);
    // Svar argument is recommended
    SvarFunction isBool([](Svar config){return config.is<bool>();});
    EXPECT_TRUE(isBool.call(false).as<bool>());
    // pointer
    Svar::lambda([](int* ref){*ref=10;})(&intV);
    Svar::lambda([](int* ref){*ref=10;})(intSvar);
    EXPECT_EQ(intV,10);
    EXPECT_EQ(intSvar,10);
    // WARNNING!! ref, should pay attention that when using ref it should always input the svar instead of the raw ref
    Svar::lambda([](int& ref){ref=20;})(intSvar);
    EXPECT_EQ(intSvar,20);
    // const ref
    Svar::lambda([](const int& ref,int* dst){*dst=ref;})(30,&intV);
    EXPECT_EQ(intV,30);
    // const pointer
    Svar::lambda([](const int* ref,int* dst){*dst=*ref;})(&srcV,&intV);
    EXPECT_EQ(intV,srcV);
    // overload is only called when cast is not available!
    Svar funcReturn=Svar::lambda([](int i){return i;});
    funcReturn.as<SvarFunction>().next=Svar::lambda([](char c){return c;});
    EXPECT_EQ(funcReturn(1).cpptype(),typeid(int));
    EXPECT_EQ(funcReturn('c').cpptype(),typeid(char));
    // string argument
    Svar s("hello");
    Svar::lambda([](std::string raw,
                 const std::string c,
                 const std::string& cref,
                 std::string& ref,
                 std::string* ptr,
                 const std::string* cptr){
        EXPECT_EQ(raw,"hello");
        EXPECT_EQ(c,"hello");
        EXPECT_EQ(ref,"hello");
        EXPECT_EQ(cref,"hello");
        EXPECT_EQ((*ptr),"hello");
        EXPECT_EQ((*cptr),"hello");
    })(s,s,s,s,s,s);
    // cpp function binding
    EXPECT_EQ(Svar(add)("a",std::string("b")).as<std::string>(),"ab");
    // static method function binding
    EXPECT_TRUE(Svar::Null().isNull());
    EXPECT_TRUE(Svar(&Svar::Null)().isNull());

}

class BaseClass{
public:
    BaseClass():age_(0){}
    BaseClass(int age):age_(age){}

    int getAge()const{return age_;}
    void setAge(int a){age_=a;}

    static BaseClass create(int a){return BaseClass(a);}
    static BaseClass create1(){return BaseClass();}

    virtual std::string intro()const{return "age:"+std::to_string(age_);}

    int age_;
};

class InheritClass: public BaseClass
{
public:
    InheritClass(int age,std::string name):BaseClass(age),name_(name){}

    std::string name()const{return name_;}

    virtual std::string intro() const{return BaseClass::intro()+", name:"+name_;}
    std::string name_;
};


TEST(Svar,Class){
    SvarClass::Class<BaseClass>()
            .def_static("__init__",[](){return BaseClass();})
            .def_static("__init__",[](int age){return BaseClass(age);})
            .def("getAge",&BaseClass::getAge)
            .def("setAge",&BaseClass::setAge)
            .def_static("create",&BaseClass::create)
            .def_static("create1",&BaseClass::create1)
            .def("intro",&BaseClass::intro);
    Svar a=Svar::create(BaseClass(10));
    Svar baseClass=a.classObject();
    a=baseClass["__init__"](10);
    a=baseClass();//
    a=baseClass(10);//
    EXPECT_EQ(a.call("getAge").as<int>(),10);
    EXPECT_EQ(a.call("intro").as<std::string>(),BaseClass(10).intro());


    SvarClass::Class<InheritClass>()
            .inherit({SvarClass::instance<BaseClass>()})
            .def("__init__",[](int age,std::string name){return InheritClass(age,name);})
            .def("name",&InheritClass::name)
            .def("intro",&InheritClass::intro);
    Svar b=Svar::create(InheritClass(10,"xm"));
    std::cout<<SvarClass::Class<InheritClass>();
    EXPECT_EQ(b.call("getAge").as<int>(),10);
    EXPECT_EQ(b.call("name").as<std::string>(),"xm");
    EXPECT_EQ(b.call("intro").as<std::string>(),InheritClass(10,"xm").intro());
}

TEST(Svar,GetSet){
    Svar var;
    int& testInt=var.GetInt("child.testInt",20);
    EXPECT_EQ(testInt,20);
    testInt=30;
    EXPECT_EQ(var.GetInt("child.testInt"),30);
    var.set("child.testInt",40);
    EXPECT_EQ(testInt,40);
    EXPECT_EQ(var["child"]["testInt"],40);
}

TEST(Svar,Call){
    EXPECT_EQ(Svar(1).call("__str__"),"1");// Call as member methods
    EXPECT_EQ(SvarClass::instance<int>().call("__str__",1),"1");// Call as static function
}

TEST(Svar,Cast){
    EXPECT_EQ(Svar(2.).castAs<int>(),2);
    EXPECT_TRUE(Svar(1).castAs<double>()==1.);
}

TEST(Svar,NumOp){
    EXPECT_EQ(-Svar(2.1),-2.1);
    EXPECT_EQ(-Svar(2),-2);
    EXPECT_EQ(Svar(2.1)+Svar(1),3.1);
    EXPECT_EQ(Svar(4.1)-Svar(2),4.1-2);
    EXPECT_EQ(Svar(3)*Svar(3.3),3*3.3);
    EXPECT_EQ(Svar(5.4)/Svar(2),5.4/2);
    EXPECT_EQ(Svar(5)%Svar(2),5%2);
    EXPECT_EQ(Svar(5)^Svar(2),5^2);
    EXPECT_EQ(Svar(5)|Svar(2),5|2);
    EXPECT_EQ(Svar(5)&Svar(2),5&2);
}

TEST(Svar,Dump){
    return;
    Svar obj(std::map<std::string,int>({{"1",1}}));
    std::cout<<obj<<std::endl;
    std::cout<<Svar::array({1,2,3})<<std::endl;
    std::cout<<Svar::create((std::type_index)typeid(1))<<std::endl;
    std::cout<<Svar::lambda([](std::string sig){})<<std::endl;
    std::cout<<SvarClass::Class<int>();
    std::cout<<Svar::instance();
}

TEST(Svar,Iterator){
    auto vec=Svar::array({1,2,3});
    Svar arrayiter=vec.call("__iter__");
    Svar next=arrayiter.classObject()["next"];
    int i=0;
    for(Svar it=next(arrayiter);!it.isUndefined();it=next(arrayiter)){
        EXPECT_EQ(vec[i++],it);
    }
    i=0;
    for(Svar it=arrayiter.call("next");!it.isUndefined();it=arrayiter.call("next"))
    {
        EXPECT_EQ(vec[i++],it);
    }
}

TEST(Svar,Json){
    Svar var;
    var.set("i",1);
    var.set("d",2.);
    var.set("s",Svar("str"));
    var.set("b",false);
    var.set("l",Svar::array({1,2,3}));
    var.set("m",Svar::object({{"a",1},{"b",false}}));
    std::string str=Json::dump(var);
    Svar varCopy=Json::load(str);
    EXPECT_EQ(str,Json::dump(varCopy));
}

TEST(Svar,Thread){
    auto readThread=[](){
        while(!svar.get<bool>("shouldstop",false)){
            double& d=svar.get<double>("thread.Double",100);
            usleep(1);
        }
    };
    auto writeThread=[](){
        svar.set<double>("thread.Double",svar.GetDouble("thread.Double")++);
        while(!svar.get<bool>("shouldstop",false)){
            svar.set<int>("thread.Double",10);// use int instead of double to violately testing robustness
            usleep(1);
        }
    };
    std::vector<std::thread> threads;
    for(int i=0;i<svar.GetInt("readThreads",4);i++) threads.push_back(std::thread(readThread));
    for(int i=0;i<svar.GetInt("writeThreads",4);i++) threads.push_back(std::thread(writeThread));
    sleep(1);
    svar.set("shouldstop",true);
    for(auto& it:threads) it.join();
}

class SayHello{
public:
    SayHello(std::string nm):name(nm){}
    void sayHello(){}
    std::string name;// warp a member value
};

TEST(Svar,Inherit){
    svar.set("classSayHello",SayHello("xiaoming"));
    EXPECT_TRUE(svar["classSayHello"].is<SayHello>());
//    auto say_hello=new SvarClass("SayHello",typeid(SayHello));
//    say_hello->def("__init__",&SayHello::SayHello)
//            .def("sayHello",&SayHello::sayHello)
//            .def("name",&SayHello::name)
//            ;

//    Svar zy=Svar(say_hello)("zhaoyong");
//    zy["sayHello"]();
//    zy.call("sayHello");
//    std::string name=zy["name"].as<std::string>();


//    SvarClass* parent=new SvarClass("parent",typeid(SvarObject));
//    parent->def("__init__",[](Svar self){
//        self.set("name","father");
//    })
//    .def("__str__",[](Svar self){
//        return Svar("I am the parent");
//    });
//    Svar p((SvarValue*)parent);
//    SvarClass* child=new SvarClass("child",typeid(SvarObject),{p});
//    child->def("__init__",[](Svar self){
//        self.set("age",10);
//    })
//    .def("__str__",[](Svar self){
//        return Svar("I am the child");
//    });
//    arrayinteratorClass=(SvarValue*)cls;
}
int main(int argc,char** argv){
    Svar var;
    Svar unParsed=var.parseMain(argc,argv);
    var.arg<int>("argInt",100,"this is a sample int argument");
    var.arg<double>("argDouble",100.,"this is a sample double argument");
    var.arg<std::string>("argString","hello","Sample string argument");
    var.arg<bool>("argBool",false,"Sample bool argument");
    var.arg<bool>("child.bool",true,"Child bool");
    if(var.get<bool>("help",false)){
        var.help();
        return 0;
    }
    std::cerr<<"Unparsed arguments:"<<unParsed;

    testing::InitGoogleTest(&argc,argv);
    auto ret= RUN_ALL_TESTS();

    std::cout<<var;
    return 0;
}
