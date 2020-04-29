#include "Svar.h"
#include "gtest.h"

using namespace sv;

TEST(Svar,Cast){
    EXPECT_EQ(Svar(2.).castAs<int>(),2);
    EXPECT_TRUE(Svar(1).castAs<double>()==1.);

    struct A{int a,b,c;}; // define a struct

    A a={1,2,3};
    Svar avar=a;      // support directly value copy assign
    Svar aptrvar=&a;  // support pointer value assign
    Svar uptrvar=std::unique_ptr<A>(new A({2,3,4}));
    Svar sptrvar=std::shared_ptr<A>(new A({2,3,4})); // support shared_ptr lvalue

    EXPECT_NO_THROW(avar.as<A>());
    EXPECT_THROW(avar.as<A*>(),SvarExeption);
    EXPECT_NO_THROW(avar.castAs<A*>());

    EXPECT_NO_THROW(aptrvar.as<A>());
    EXPECT_NO_THROW(aptrvar.as<A*>());

    EXPECT_NO_THROW(uptrvar.as<std::unique_ptr<A>>());
    EXPECT_NO_THROW(uptrvar.as<A>());
    EXPECT_THROW(uptrvar.as<A*>(),SvarExeption);
    EXPECT_NO_THROW(uptrvar.castAs<A*>());

    EXPECT_NO_THROW(sptrvar.as<std::shared_ptr<A>>());
    EXPECT_NO_THROW(sptrvar.as<A>());
    EXPECT_THROW(sptrvar.as<A*>(),SvarExeption);
    EXPECT_NO_THROW(sptrvar.castAs<A*>());
}
