#include "Svar.h"
#include "Registry.h"
#include "gtest.h"

using namespace sv;

TEST(Svar,PyMath)
{
   const Svar svarpy = Registry::load("svarpy");
   if(svarpy.isUndefined()) return;

   const Svar math = svarpy["import"]("math");

   EXPECT_TRUE(math["sqrt"].isFunction());

   EXPECT_EQ(math["sqrt"](4),2);
}


TEST(Svar,PyOS)
{
   const Svar svarpy = Registry::load("svarpy");
   if(svarpy.isUndefined()) return;

   const Svar os = svarpy["import"]("os");
//   std::cout<<svarpy["fromPy"](os)<<std::endl;

   EXPECT_TRUE(os["getpid"].isFunction());

   EXPECT_FALSE(os["getpid"]().isUndefined());
}

TEST(Svar,PyOSThread)
{
    std::thread t([]{
        const Svar svarpy = Registry::load("svarpy");
        if(svarpy.isUndefined()) return;

        const Svar os = svarpy["import"]("os");

        EXPECT_TRUE(os["getpid"].isFunction());

        EXPECT_FALSE(os["getpid"]().isUndefined());
    });
    t.join();
}
