#include "Svar.h"
#include "gtest.h"

using namespace sv;

TEST(Svar,Dump){
    return;
    Svar obj(std::map<std::string,int>({{"1",1}}));
    std::cout<<obj<<std::endl;
    std::cout<<Svar::array({1,2,3})<<std::endl;
    std::cout<<Svar((std::type_index)typeid(1))<<std::endl;
    std::cout<<Svar::lambda([](std::string sig){})<<std::endl;
    std::cout<<SvarClass::Class<int>();
    std::cout<<Svar::instance();
}

TEST(Svar,Json){
    Svar var;
    var.set("i",1);
    var.set("d",2.);
    var.set("s",Svar("str"));
    var.set("b",false);
    var.set("l",Svar::array({1,2,3}));
    var.set("m",Svar::object({{"a",1},{"b",false}}));
    std::string str=var.dump_json();
    Svar varCopy=Svar::parse_json(str);

    Svar v1=Svar::parse_json("{a:1,b:true}");
    EXPECT_EQ(v1["a"],1);
    EXPECT_EQ(v1["b"],true);
}

TEST(Svar,Iterator){
    Svar var={1,2,3,4};
    int i=0;
    for(auto a:var) EXPECT_EQ(a,var[i++]);
    Svar obj={{"1",1},{"2",2}};
    for(std::pair<std::string,Svar> it:obj) EXPECT_EQ(obj[it.first],it.second);
}


TEST(Svar,GetSet){
    Svar var;
    Svar testInt=var.get<Svar>("testInt",20);
    EXPECT_EQ(testInt,20);
    testInt=30;
    EXPECT_EQ(var.GetInt("testInt"),30);
    var["testInt"]=40;
    EXPECT_EQ(testInt,40);
    EXPECT_EQ(var["testInt"],40);
    var.set("int",100);
    var.set("double",100.);
    var.set<std::string>("string","100");
    var.set("bool",true);
    EXPECT_EQ(var["int"],100);
    EXPECT_EQ(var["double"],100.);
    EXPECT_EQ(var["string"],"100");
    // EXPECT_EQ(var["string"],100);
    // FIXME: why this equal, int casted to string? Segment fault in travis-ci
    EXPECT_EQ(var["bool"],true);
}


TEST(Svar,Variable)
{
    EXPECT_TRUE(Svar()==Svar::Undefined());
    EXPECT_TRUE(Svar(nullptr)==Svar::Null());

    Svar var(false);
    EXPECT_EQ(var.typeName(),"bool");
    EXPECT_TRUE(var.is<bool>());
    EXPECT_FALSE(var.as<bool>());

    EXPECT_TRUE(Svar(1).is<int>());
    EXPECT_TRUE(Svar(1).as<int>()==1);

    EXPECT_TRUE(Svar("").as<std::string>().empty());
    EXPECT_TRUE(Svar(1.).as<double>()==1.);
    EXPECT_TRUE(Svar({1,2}).isArray());
    EXPECT_TRUE(Svar(std::map<int,Svar>({{1,2}})).isDict());

    Svar obj({{"1",1}});
    EXPECT_TRUE(obj.isObject());
    EXPECT_TRUE(obj["1"]==1);

    obj["left"]=Svar("hello");
    obj["parent"]["child"]=3;
    EXPECT_EQ(obj["parent"]["child"],3);
    obj["hello"]["world"]=false;
    Svar hello=obj["hello"];
    EXPECT_EQ(obj["hello"]["world"],false);

    std::map<std::string,Svar> cMap=obj.castAs<std::map<std::string,Svar> >();
    EXPECT_EQ(cMap["1"],1);

    Svar vec({0,1,2,3});
    EXPECT_EQ(vec[1],1);
    vec[1]=2;
    EXPECT_EQ(vec[1],2);

    Svar strlit=R"(
                {"a":[true,1,12.3,"hello"]}
                )"_svar;// create from raw string literal

    EXPECT_TRUE(strlit["a"].isArray());

    EXPECT_EQ("[1,2]"_svar .length(),2);

    EXPECT_TRUE(Svar(std::shared_ptr<std::mutex>(new std::mutex())).is<std::mutex>());
    EXPECT_TRUE(Svar(std::unique_ptr<std::mutex>(new std::mutex())).is<std::mutex>());

    std::vector<int> rvec=vec.castAs<std::vector<int>>();
    EXPECT_EQ(rvec[0],0);

    EXPECT_THROW(vec.as<std::vector<int>>(),SvarExeption);


    Svar svarMtx(std::shared_ptr<std::mutex>(new std::mutex()));
    auto mtx=svarMtx.castAs<std::shared_ptr<std::mutex>>();
    mtx->lock();
    mtx->unlock();
}
