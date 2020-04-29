#include "Svar.h"
#include "gtest.h"

using namespace sv;

TEST(Svar,Alias){
    Svar i=1;
    Svar alias=i;
    EXPECT_EQ(i,alias);
    i=2;
    EXPECT_EQ(i,alias);
    Svar copy=i.clone();
    EXPECT_EQ(i,copy);
    i=4;
    EXPECT_NE(i,copy);

    Svar var;
    var.set("i",i);
    var.set("alias",i);
    EXPECT_EQ(var["i"],var["alias"]);
    i=3;
    EXPECT_EQ(var["i"],3);
    EXPECT_EQ(var["i"],var["alias"]);

    Svar varCopy=var.clone();
    varCopy["i"]=5;
    EXPECT_EQ(var["i"],5);
    EXPECT_EQ(var["alias"],5);

    Svar varDeepCopy=var.clone(100);
    varDeepCopy["i"]=6;
    varDeepCopy["alias"]=7;
    EXPECT_NE(var["i"],6);
    EXPECT_NE(var["alias"],7);

    Svar vec={i,i};
    EXPECT_EQ(vec[0],i);
    i=8;
    EXPECT_EQ(vec[0],8);
    EXPECT_EQ(vec[1],8);

    Svar vecClone=vec.clone(1);
    EXPECT_EQ(vecClone[0],i);
    i=9;
    EXPECT_NE(vecClone[0],i);
}
