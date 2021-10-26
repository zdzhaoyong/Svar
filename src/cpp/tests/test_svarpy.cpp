#include "Svar.h"
#include "gtest.h"

using namespace sv;

TEST(SvarPy,PyMath)
{
   const Svar svarpy = svar.import("svarpy");
   if(svarpy.isUndefined()) return;

   const Svar math = svarpy["import"]("math");

   EXPECT_TRUE(math["sqrt"].isFunction());

   EXPECT_EQ(math["sqrt"](4),2); // FIXME: lead to segment fault when application shutdown?
}


TEST(SvarPy,PyOS)
{
   const Svar svarpy = svar.import("svarpy");
   if(svarpy.isUndefined()) return;

   const Svar os = svarpy["import"]("os");
#ifdef __linux__
   EXPECT_EQ(os["getpid"](),(int)getpid());
#endif

   EXPECT_TRUE(os["getpid"].isFunction());

   EXPECT_FALSE(os["getpid"]().isUndefined());
}

TEST(SvarPy,PyOSThread)
{
    std::thread t([]{
        const Svar svarpy =svar.import("svarpy");
        if(svarpy.isUndefined()) return;

        const Svar os = svarpy["import"]("os");

        EXPECT_TRUE(os["getpid"].isFunction());

        EXPECT_FALSE(os["getpid"]().isUndefined());
    });
    t.join();
}
