#include "Svar.h"
#include "Registry.h"
#include "gtest.h"

using namespace sv;

TEST(Svar,PyMath)
{
   const Svar svarpy = Registry::load("svarpy");
   if(svarpy.isUndefined()) return;

   EXPECT_TRUE(svarpy["sqrt"].isFunction());

   const Svar math = svarpy["import"]("math");

   EXPECT_EQ(math["sqrt"](4),2);
}
