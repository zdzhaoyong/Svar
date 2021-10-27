// Svar - A Tiny Modern C++ Header Brings Unified Interface for Different Languages
// Copyright 2018 PILAB Inc. All rights reserved.
// https://github.com/zdzhaoyong/Svar
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
// * Neither the name of Google Inc. nor the names of its contributors may be
//   used to endorse or promote products derived from this software without
//   specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
// Author: zd5945@126.com (Yong Zhao)
//
// By Using Svar, your C++ library can be called easily with different
// languages like C++, Java, Python and Javascript.
//
// Svar brings the following features:
// 1. A dynamic link library that can be used as a module in languages
//    such as C++, Python, and Node-JS;
// 2. Unified C++ library interface, which can be called as a plug-in module.
//    The released library comes with documentation, making *.h header file interface description unnecessary;
// 3. Dynamic features. It wraps variables, functions, and classes into Svar while maintaining efficiency;
// 4. Built-in Json support, parameter configuration parsing, thread-safe reading and writing,
//    data decoupling sharing between modules, etc.

#ifndef SVAR_SVAR_H
#define SVAR_SVAR_H

#include <assert.h>
#include <string.h>
#include <cstdlib>
#include <cstdio> // snprintf
#include <cctype>
#include <cstring> // memcpy
#include <cmath>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <vector>
#include <list>
#include <set>
#include <map>
#include <unordered_map>
#include <algorithm>
#include <memory>
#include <mutex>
#include <thread>
#include <typeindex>
#include <functional>

#if (defined(_WIN32) || defined(_WIN64)) && !defined(__CYGWIN__)// Windows
#include <windows.h>
#include <io.h>
#else // Linux
#include <unistd.h>
#include <dlfcn.h>
#if defined(__CYGWIN__) && !defined(RTLD_LOCAL)
#define RTLD_LOCAL 0
#endif
#endif

#ifdef __GNUC__
#include <cxxabi.h>
#else
#define _GLIBCXX_USE_NOEXCEPT
#endif
#if defined(_MSC_VER)
#include <BaseTsd.h>
typedef SSIZE_T ssize_t;
#endif

#define svar sv::Svar::instance()
#define SVAR_VERSION 0x000302
#define EXPORT_SVAR_INSTANCE extern "C" SVAR_EXPORT sv::Svar* svarInstance(){return &sv::Svar::instance();}
#define REGISTER_SVAR_MODULE(MODULE_NAME) \
    class SVAR_MODULE_##MODULE_NAME{\
    public: SVAR_MODULE_##MODULE_NAME();\
    }SVAR_MODULE_##MODULE_NAME##instance;\
    SVAR_MODULE_##MODULE_NAME::SVAR_MODULE_##MODULE_NAME()

#  if defined(WIN32) || defined(_WIN32)
#    define SVAR_EXPORT __declspec(dllexport)
#  else
#    define SVAR_EXPORT __attribute__ ((visibility("default")))
#  endif

namespace sv {

#ifndef DOXYGEN_IGNORE_INTERNAL
namespace detail {
template <bool B, typename T = void> using enable_if_t = typename std::enable_if<B, T>::type;
template <bool B, typename T, typename F> using conditional_t = typename std::conditional<B, T, F>::type;
template <typename T> using remove_cv_t = typename std::remove_cv<T>::type;
template <typename T> using remove_reference_t = typename std::remove_reference<T>::type;


/// Strip the class from a method type
template <typename T> struct remove_class { };
template <typename C, typename R, typename... A> struct remove_class<R (C::*)(A...)> { typedef R type(A...); };
template <typename C, typename R, typename... A> struct remove_class<R (C::*)(A...) const> { typedef R type(A...); };

template <typename F> struct strip_function_object {
    using type = typename remove_class<decltype(&F::operator())>::type;
};
// Extracts the function signature from a function, function pointer or lambda.
template <typename Function, typename F = remove_reference_t<Function>>
using function_signature_t = conditional_t<
    std::is_function<F>::value,
    F,
    typename conditional_t<
        std::is_pointer<F>::value || std::is_member_pointer<F>::value,
        std::remove_pointer<F>,
        strip_function_object<F>
    >::type
>;

template<size_t ...> struct index_sequence  { };
template<size_t N, size_t ...S> struct make_index_sequence_impl : make_index_sequence_impl <N - 1, N - 1, S...> { };
template<size_t ...S> struct make_index_sequence_impl <0, S...> { typedef index_sequence<S...> type; };
template<size_t N> using make_index_sequence = typename make_index_sequence_impl<N>::type;

template<class T>
struct sfinae_true : public std::true_type{
  typedef T type;

};

template<class T>
struct sfinae_false : public std::false_type{
  typedef T type;
};

template<class T>
static auto test_call_op(int)
  ->sfinae_true < decltype(&T::operator()) >;
template<class T>
static auto test_call_op(long)->sfinae_false < T >;

template<class T, class T2 =decltype( test_call_op<T>(0) )>
struct has_call_op_ : public T2  {

};

template<class T>
struct has_call_op : public std::is_member_function_pointer < typename has_call_op_<T>::type > {

};

template<typename T>
struct is_callable :public has_call_op<T>
{
};
template<typename TClass, typename TRet, typename... TArgs>
struct is_callable<TRet(TClass::*)(TArgs...)>{
const static bool value = true;
};

template<typename TClass, typename TRet, typename... TArgs>
struct is_callable<TRet(TClass::*)(TArgs...)const>{
const static bool value = true;
};

template<typename TRet, typename... TArgs>
struct is_callable<TRet(*)(TArgs...)>{
const static bool value = true;
};

}
namespace fast_double_parser {

#define FASTFLOAT_SMALLEST_POWER -325
#define FASTFLOAT_LARGEST_POWER 308

#ifdef _MSC_VER
#ifndef really_inline
#define really_inline __forceinline
#endif // really_inline
#ifndef unlikely
#define unlikely(x) x
#endif // unlikely
#else  // _MSC_VER
#ifndef unlikely
#define unlikely(x) __builtin_expect(!!(x), 0)
#endif // unlikely
#ifndef really_inline
#define really_inline __attribute__((always_inline)) inline
#endif // really_inline
#endif // _MSC_VER

struct value128 {
  uint64_t low;
  uint64_t high;
};

// We need a backup on old systems.
// credit: https://stackoverflow.com/questions/28868367/getting-the-high-part-of-64-bit-integer-multiplication
really_inline uint64_t Emulate64x64to128(uint64_t& r_hi, const uint64_t x, const uint64_t y) {
    const uint64_t x0 = (uint32_t)x, x1 = x >> 32;
    const uint64_t y0 = (uint32_t)y, y1 = y >> 32;
    const uint64_t p11 = x1 * y1, p01 = x0 * y1;
    const uint64_t p10 = x1 * y0, p00 = x0 * y0;

    // 64-bit product + two 32-bit values
    const uint64_t middle = p10 + (p00 >> 32) + (uint32_t)p01;

    // 64-bit product + two 32-bit values
    r_hi = p11 + (middle >> 32) + (p01 >> 32);

    // Add LOW PART and lower half of MIDDLE PART
    return (middle << 32) | (uint32_t)p00;
}

really_inline value128 full_multiplication(uint64_t value1, uint64_t value2) {
  value128 answer;
#ifdef __SIZEOF_INT128__ // this is what we have on most 32-bit systems
  __uint128_t r = ((__uint128_t)value1) * value2;
  answer.low = uint64_t(r);
  answer.high = uint64_t(r >> 64);
#else
  // fallback
  answer.low = Emulate64x64to128(answer.high, value1,  value2);
#endif
  return answer;
}

/* result might be undefined when input_num is zero */
inline int leading_zeroes(uint64_t input_num) {
#ifdef _MSC_VER
#if defined(_M_X64) || defined(_M_ARM64) || defined (_M_IA64)
  unsigned long leading_zero = 0;
  // Search the mask data from most significant bit (MSB)
  // to least significant bit (LSB) for a set bit (1).
  if (_BitScanReverse64(&leading_zero, input_num))
    return (int)(63 - leading_zero);
  else
    return 64;
#else
  unsigned long x1 = (unsigned long)(x >> 32), top, bottom;
  _BitScanReverse(&top, x1);
  _BitScanReverse(&bottom, (unsigned long)x);
  return x1 ? top + 32 : bottom;
#endif // defined(_M_X64) || defined(_M_ARM64) || defined (_M_IA64)
#else
  return __builtin_clzll(input_num);
#endif // _MSC_VER
}

static inline bool is_integer(char c) {
  return (c >= '0' && c <= '9');
  // this gets compiled to (uint8_t)(c - '0') <= 9 on all decent compilers
}


really_inline double compute_float_64(int64_t power, uint64_t i, bool negative,
                                      bool *success) {

  // Precomputed powers of ten from 10^0 to 10^22. These
  // can be represented exactly using the double type.
  static const double power_of_ten[] = {
      1e0,  1e1,  1e2,  1e3,  1e4,  1e5,  1e6,  1e7,  1e8,  1e9,  1e10, 1e11,
      1e12, 1e13, 1e14, 1e15, 1e16, 1e17, 1e18, 1e19, 1e20, 1e21, 1e22};

  // The mantissas of powers of ten from -308 to 308, extended out to sixty four
  // bits. The array contains the powers of ten approximated
  // as a 64-bit mantissa. It goes from 10^FASTFLOAT_SMALLEST_POWER to
  // 10^FASTFLOAT_LARGEST_POWER (inclusively).
  // The mantissa is truncated, and
  // never rounded up. Uses about 5KB.
  static const uint64_t mantissa_64[] = {
      0xa5ced43b7e3e9188, 0xcf42894a5dce35ea, 0x818995ce7aa0e1b2, 0xa1ebfb4219491a1f, 0xca66fa129f9b60a6, 0xfd00b897478238d0,
      0x9e20735e8cb16382, 0xc5a890362fddbc62, 0xf712b443bbd52b7b, 0x9a6bb0aa55653b2d, 0xc1069cd4eabe89f8, 0xf148440a256e2c76,
      0x96cd2a865764dbca, 0xbc807527ed3e12bc, 0xeba09271e88d976b, 0x93445b8731587ea3, 0xb8157268fdae9e4c, 0xe61acf033d1a45df,
      0x8fd0c16206306bab, 0xb3c4f1ba87bc8696, 0xe0b62e2929aba83c, 0x8c71dcd9ba0b4925, 0xaf8e5410288e1b6f, 0xdb71e91432b1a24a,
      0x892731ac9faf056e, 0xab70fe17c79ac6ca, 0xd64d3d9db981787d, 0x85f0468293f0eb4e, 0xa76c582338ed2621, 0xd1476e2c07286faa,
      0x82cca4db847945ca, 0xa37fce126597973c, 0xcc5fc196fefd7d0c, 0xff77b1fcbebcdc4f, 0x9faacf3df73609b1, 0xc795830d75038c1d,
      0xf97ae3d0d2446f25, 0x9becce62836ac577, 0xc2e801fb244576d5, 0xf3a20279ed56d48a, 0x9845418c345644d6, 0xbe5691ef416bd60c,
      0xedec366b11c6cb8f, 0x94b3a202eb1c3f39, 0xb9e08a83a5e34f07, 0xe858ad248f5c22c9, 0x91376c36d99995be, 0xb58547448ffffb2d,
      0xe2e69915b3fff9f9, 0x8dd01fad907ffc3b, 0xb1442798f49ffb4a, 0xdd95317f31c7fa1d, 0x8a7d3eef7f1cfc52, 0xad1c8eab5ee43b66,
      0xd863b256369d4a40, 0x873e4f75e2224e68, 0xa90de3535aaae202, 0xd3515c2831559a83, 0x8412d9991ed58091, 0xa5178fff668ae0b6,
      0xce5d73ff402d98e3, 0x80fa687f881c7f8e, 0xa139029f6a239f72, 0xc987434744ac874e, 0xfbe9141915d7a922, 0x9d71ac8fada6c9b5,
      0xc4ce17b399107c22, 0xf6019da07f549b2b, 0x99c102844f94e0fb, 0xc0314325637a1939, 0xf03d93eebc589f88, 0x96267c7535b763b5,
      0xbbb01b9283253ca2, 0xea9c227723ee8bcb, 0x92a1958a7675175f, 0xb749faed14125d36, 0xe51c79a85916f484, 0x8f31cc0937ae58d2,
      0xb2fe3f0b8599ef07, 0xdfbdcece67006ac9, 0x8bd6a141006042bd, 0xaecc49914078536d, 0xda7f5bf590966848, 0x888f99797a5e012d,
      0xaab37fd7d8f58178, 0xd5605fcdcf32e1d6, 0x855c3be0a17fcd26, 0xa6b34ad8c9dfc06f, 0xd0601d8efc57b08b, 0x823c12795db6ce57,
      0xa2cb1717b52481ed, 0xcb7ddcdda26da268, 0xfe5d54150b090b02, 0x9efa548d26e5a6e1, 0xc6b8e9b0709f109a, 0xf867241c8cc6d4c0,
      0x9b407691d7fc44f8, 0xc21094364dfb5636, 0xf294b943e17a2bc4, 0x979cf3ca6cec5b5a, 0xbd8430bd08277231, 0xece53cec4a314ebd,
      0x940f4613ae5ed136, 0xb913179899f68584, 0xe757dd7ec07426e5, 0x9096ea6f3848984f, 0xb4bca50b065abe63, 0xe1ebce4dc7f16dfb,
      0x8d3360f09cf6e4bd, 0xb080392cc4349dec, 0xdca04777f541c567, 0x89e42caaf9491b60, 0xac5d37d5b79b6239, 0xd77485cb25823ac7,
      0x86a8d39ef77164bc, 0xa8530886b54dbdeb, 0xd267caa862a12d66, 0x8380dea93da4bc60, 0xa46116538d0deb78, 0xcd795be870516656,
      0x806bd9714632dff6, 0xa086cfcd97bf97f3, 0xc8a883c0fdaf7df0, 0xfad2a4b13d1b5d6c, 0x9cc3a6eec6311a63, 0xc3f490aa77bd60fc,
      0xf4f1b4d515acb93b, 0x991711052d8bf3c5, 0xbf5cd54678eef0b6, 0xef340a98172aace4, 0x9580869f0e7aac0e, 0xbae0a846d2195712,
      0xe998d258869facd7, 0x91ff83775423cc06, 0xb67f6455292cbf08, 0xe41f3d6a7377eeca, 0x8e938662882af53e, 0xb23867fb2a35b28d,
      0xdec681f9f4c31f31, 0x8b3c113c38f9f37e, 0xae0b158b4738705e, 0xd98ddaee19068c76, 0x87f8a8d4cfa417c9, 0xa9f6d30a038d1dbc,
      0xd47487cc8470652b, 0x84c8d4dfd2c63f3b, 0xa5fb0a17c777cf09, 0xcf79cc9db955c2cc, 0x81ac1fe293d599bf, 0xa21727db38cb002f,
      0xca9cf1d206fdc03b, 0xfd442e4688bd304a, 0x9e4a9cec15763e2e, 0xc5dd44271ad3cdba, 0xf7549530e188c128, 0x9a94dd3e8cf578b9,
      0xc13a148e3032d6e7, 0xf18899b1bc3f8ca1, 0x96f5600f15a7b7e5, 0xbcb2b812db11a5de, 0xebdf661791d60f56, 0x936b9fcebb25c995,
      0xb84687c269ef3bfb, 0xe65829b3046b0afa, 0x8ff71a0fe2c2e6dc, 0xb3f4e093db73a093, 0xe0f218b8d25088b8, 0x8c974f7383725573,
      0xafbd2350644eeacf, 0xdbac6c247d62a583, 0x894bc396ce5da772, 0xab9eb47c81f5114f, 0xd686619ba27255a2, 0x8613fd0145877585,
      0xa798fc4196e952e7, 0xd17f3b51fca3a7a0, 0x82ef85133de648c4, 0xa3ab66580d5fdaf5, 0xcc963fee10b7d1b3, 0xffbbcfe994e5c61f,
      0x9fd561f1fd0f9bd3, 0xc7caba6e7c5382c8, 0xf9bd690a1b68637b, 0x9c1661a651213e2d, 0xc31bfa0fe5698db8, 0xf3e2f893dec3f126,
      0x986ddb5c6b3a76b7, 0xbe89523386091465, 0xee2ba6c0678b597f, 0x94db483840b717ef, 0xba121a4650e4ddeb, 0xe896a0d7e51e1566,
      0x915e2486ef32cd60, 0xb5b5ada8aaff80b8, 0xe3231912d5bf60e6, 0x8df5efabc5979c8f, 0xb1736b96b6fd83b3, 0xddd0467c64bce4a0,
      0x8aa22c0dbef60ee4, 0xad4ab7112eb3929d, 0xd89d64d57a607744, 0x87625f056c7c4a8b, 0xa93af6c6c79b5d2d, 0xd389b47879823479,
      0x843610cb4bf160cb, 0xa54394fe1eedb8fe, 0xce947a3da6a9273e, 0x811ccc668829b887, 0xa163ff802a3426a8, 0xc9bcff6034c13052,
      0xfc2c3f3841f17c67, 0x9d9ba7832936edc0, 0xc5029163f384a931, 0xf64335bcf065d37d, 0x99ea0196163fa42e, 0xc06481fb9bcf8d39,
      0xf07da27a82c37088, 0x964e858c91ba2655, 0xbbe226efb628afea, 0xeadab0aba3b2dbe5, 0x92c8ae6b464fc96f, 0xb77ada0617e3bbcb,
      0xe55990879ddcaabd, 0x8f57fa54c2a9eab6, 0xb32df8e9f3546564, 0xdff9772470297ebd, 0x8bfbea76c619ef36, 0xaefae51477a06b03,
      0xdab99e59958885c4, 0x88b402f7fd75539b, 0xaae103b5fcd2a881, 0xd59944a37c0752a2, 0x857fcae62d8493a5, 0xa6dfbd9fb8e5b88e,
      0xd097ad07a71f26b2, 0x825ecc24c873782f, 0xa2f67f2dfa90563b, 0xcbb41ef979346bca, 0xfea126b7d78186bc, 0x9f24b832e6b0f436,
      0xc6ede63fa05d3143, 0xf8a95fcf88747d94, 0x9b69dbe1b548ce7c, 0xc24452da229b021b, 0xf2d56790ab41c2a2, 0x97c560ba6b0919a5,
      0xbdb6b8e905cb600f, 0xed246723473e3813, 0x9436c0760c86e30b, 0xb94470938fa89bce, 0xe7958cb87392c2c2, 0x90bd77f3483bb9b9,
      0xb4ecd5f01a4aa828, 0xe2280b6c20dd5232, 0x8d590723948a535f, 0xb0af48ec79ace837, 0xdcdb1b2798182244, 0x8a08f0f8bf0f156b,
      0xac8b2d36eed2dac5, 0xd7adf884aa879177, 0x86ccbb52ea94baea, 0xa87fea27a539e9a5, 0xd29fe4b18e88640e, 0x83a3eeeef9153e89,
      0xa48ceaaab75a8e2b, 0xcdb02555653131b6, 0x808e17555f3ebf11, 0xa0b19d2ab70e6ed6, 0xc8de047564d20a8b, 0xfb158592be068d2e,
      0x9ced737bb6c4183d, 0xc428d05aa4751e4c, 0xf53304714d9265df, 0x993fe2c6d07b7fab, 0xbf8fdb78849a5f96, 0xef73d256a5c0f77c,
      0x95a8637627989aad, 0xbb127c53b17ec159, 0xe9d71b689dde71af, 0x9226712162ab070d, 0xb6b00d69bb55c8d1, 0xe45c10c42a2b3b05,
      0x8eb98a7a9a5b04e3, 0xb267ed1940f1c61c, 0xdf01e85f912e37a3, 0x8b61313bbabce2c6, 0xae397d8aa96c1b77, 0xd9c7dced53c72255,
      0x881cea14545c7575, 0xaa242499697392d2, 0xd4ad2dbfc3d07787, 0x84ec3c97da624ab4, 0xa6274bbdd0fadd61, 0xcfb11ead453994ba,
      0x81ceb32c4b43fcf4, 0xa2425ff75e14fc31, 0xcad2f7f5359a3b3e, 0xfd87b5f28300ca0d, 0x9e74d1b791e07e48, 0xc612062576589dda,
      0xf79687aed3eec551, 0x9abe14cd44753b52, 0xc16d9a0095928a27, 0xf1c90080baf72cb1, 0x971da05074da7bee, 0xbce5086492111aea,
      0xec1e4a7db69561a5, 0x9392ee8e921d5d07, 0xb877aa3236a4b449, 0xe69594bec44de15b, 0x901d7cf73ab0acd9, 0xb424dc35095cd80f,
      0xe12e13424bb40e13, 0x8cbccc096f5088cb, 0xafebff0bcb24aafe, 0xdbe6fecebdedd5be, 0x89705f4136b4a597, 0xabcc77118461cefc,
      0xd6bf94d5e57a42bc, 0x8637bd05af6c69b5, 0xa7c5ac471b478423, 0xd1b71758e219652b, 0x83126e978d4fdf3b, 0xa3d70a3d70a3d70a,
      0xcccccccccccccccc, 0x8000000000000000, 0xa000000000000000, 0xc800000000000000, 0xfa00000000000000, 0x9c40000000000000,
      0xc350000000000000, 0xf424000000000000, 0x9896800000000000, 0xbebc200000000000, 0xee6b280000000000, 0x9502f90000000000,
      0xba43b74000000000, 0xe8d4a51000000000, 0x9184e72a00000000, 0xb5e620f480000000, 0xe35fa931a0000000, 0x8e1bc9bf04000000,
      0xb1a2bc2ec5000000, 0xde0b6b3a76400000, 0x8ac7230489e80000, 0xad78ebc5ac620000, 0xd8d726b7177a8000, 0x878678326eac9000,
      0xa968163f0a57b400, 0xd3c21bcecceda100, 0x84595161401484a0, 0xa56fa5b99019a5c8, 0xcecb8f27f4200f3a, 0x813f3978f8940984,
      0xa18f07d736b90be5, 0xc9f2c9cd04674ede, 0xfc6f7c4045812296, 0x9dc5ada82b70b59d, 0xc5371912364ce305, 0xf684df56c3e01bc6,
      0x9a130b963a6c115c, 0xc097ce7bc90715b3, 0xf0bdc21abb48db20, 0x96769950b50d88f4, 0xbc143fa4e250eb31, 0xeb194f8e1ae525fd,
      0x92efd1b8d0cf37be, 0xb7abc627050305ad, 0xe596b7b0c643c719, 0x8f7e32ce7bea5c6f, 0xb35dbf821ae4f38b, 0xe0352f62a19e306e,
      0x8c213d9da502de45, 0xaf298d050e4395d6, 0xdaf3f04651d47b4c, 0x88d8762bf324cd0f, 0xab0e93b6efee0053, 0xd5d238a4abe98068,
      0x85a36366eb71f041, 0xa70c3c40a64e6c51, 0xd0cf4b50cfe20765, 0x82818f1281ed449f, 0xa321f2d7226895c7, 0xcbea6f8ceb02bb39,
      0xfee50b7025c36a08, 0x9f4f2726179a2245, 0xc722f0ef9d80aad6, 0xf8ebad2b84e0d58b, 0x9b934c3b330c8577, 0xc2781f49ffcfa6d5,
      0xf316271c7fc3908a, 0x97edd871cfda3a56, 0xbde94e8e43d0c8ec, 0xed63a231d4c4fb27, 0x945e455f24fb1cf8, 0xb975d6b6ee39e436,
      0xe7d34c64a9c85d44, 0x90e40fbeea1d3a4a, 0xb51d13aea4a488dd, 0xe264589a4dcdab14, 0x8d7eb76070a08aec, 0xb0de65388cc8ada8,
      0xdd15fe86affad912, 0x8a2dbf142dfcc7ab, 0xacb92ed9397bf996, 0xd7e77a8f87daf7fb, 0x86f0ac99b4e8dafd, 0xa8acd7c0222311bc,
      0xd2d80db02aabd62b, 0x83c7088e1aab65db, 0xa4b8cab1a1563f52, 0xcde6fd5e09abcf26, 0x80b05e5ac60b6178, 0xa0dc75f1778e39d6,
      0xc913936dd571c84c, 0xfb5878494ace3a5f, 0x9d174b2dcec0e47b, 0xc45d1df942711d9a, 0xf5746577930d6500, 0x9968bf6abbe85f20,
      0xbfc2ef456ae276e8, 0xefb3ab16c59b14a2, 0x95d04aee3b80ece5, 0xbb445da9ca61281f, 0xea1575143cf97226, 0x924d692ca61be758,
      0xb6e0c377cfa2e12e, 0xe498f455c38b997a, 0x8edf98b59a373fec, 0xb2977ee300c50fe7, 0xdf3d5e9bc0f653e1, 0x8b865b215899f46c,
      0xae67f1e9aec07187, 0xda01ee641a708de9, 0x884134fe908658b2, 0xaa51823e34a7eede, 0xd4e5e2cdc1d1ea96, 0x850fadc09923329e,
      0xa6539930bf6bff45, 0xcfe87f7cef46ff16, 0x81f14fae158c5f6e, 0xa26da3999aef7749, 0xcb090c8001ab551c, 0xfdcb4fa002162a63,
      0x9e9f11c4014dda7e, 0xc646d63501a1511d, 0xf7d88bc24209a565, 0x9ae757596946075f, 0xc1a12d2fc3978937, 0xf209787bb47d6b84,
      0x9745eb4d50ce6332, 0xbd176620a501fbff, 0xec5d3fa8ce427aff, 0x93ba47c980e98cdf, 0xb8a8d9bbe123f017, 0xe6d3102ad96cec1d,
      0x9043ea1ac7e41392, 0xb454e4a179dd1877, 0xe16a1dc9d8545e94, 0x8ce2529e2734bb1d, 0xb01ae745b101e9e4, 0xdc21a1171d42645d,
      0x899504ae72497eba, 0xabfa45da0edbde69, 0xd6f8d7509292d603, 0x865b86925b9bc5c2, 0xa7f26836f282b732, 0xd1ef0244af2364ff,
      0x8335616aed761f1f, 0xa402b9c5a8d3a6e7, 0xcd036837130890a1, 0x802221226be55a64, 0xa02aa96b06deb0fd, 0xc83553c5c8965d3d,
      0xfa42a8b73abbf48c, 0x9c69a97284b578d7, 0xc38413cf25e2d70d, 0xf46518c2ef5b8cd1, 0x98bf2f79d5993802, 0xbeeefb584aff8603,
      0xeeaaba2e5dbf6784, 0x952ab45cfa97a0b2, 0xba756174393d88df, 0xe912b9d1478ceb17, 0x91abb422ccb812ee, 0xb616a12b7fe617aa,
      0xe39c49765fdf9d94, 0x8e41ade9fbebc27d, 0xb1d219647ae6b31c, 0xde469fbd99a05fe3, 0x8aec23d680043bee, 0xada72ccc20054ae9,
      0xd910f7ff28069da4, 0x87aa9aff79042286, 0xa99541bf57452b28, 0xd3fa922f2d1675f2, 0x847c9b5d7c2e09b7, 0xa59bc234db398c25,
      0xcf02b2c21207ef2e, 0x8161afb94b44f57d, 0xa1ba1ba79e1632dc, 0xca28a291859bbf93, 0xfcb2cb35e702af78, 0x9defbf01b061adab,
      0xc56baec21c7a1916, 0xf6c69a72a3989f5b, 0x9a3c2087a63f6399, 0xc0cb28a98fcf3c7f, 0xf0fdf2d3f3c30b9f, 0x969eb7c47859e743,
      0xbc4665b596706114, 0xeb57ff22fc0c7959, 0x9316ff75dd87cbd8, 0xb7dcbf5354e9bece, 0xe5d3ef282a242e81, 0x8fa475791a569d10,
      0xb38d92d760ec4455, 0xe070f78d3927556a, 0x8c469ab843b89562, 0xaf58416654a6babb, 0xdb2e51bfe9d0696a, 0x88fcf317f22241e2,
      0xab3c2fddeeaad25a, 0xd60b3bd56a5586f1, 0x85c7056562757456, 0xa738c6bebb12d16c, 0xd106f86e69d785c7, 0x82a45b450226b39c,
      0xa34d721642b06084, 0xcc20ce9bd35c78a5, 0xff290242c83396ce, 0x9f79a169bd203e41, 0xc75809c42c684dd1, 0xf92e0c3537826145,
      0x9bbcc7a142b17ccb, 0xc2abf989935ddbfe, 0xf356f7ebf83552fe, 0x98165af37b2153de, 0xbe1bf1b059e9a8d6, 0xeda2ee1c7064130c,
      0x9485d4d1c63e8be7, 0xb9a74a0637ce2ee1, 0xe8111c87c5c1ba99, 0x910ab1d4db9914a0, 0xb54d5e4a127f59c8, 0xe2a0b5dc971f303a,
      0x8da471a9de737e24, 0xb10d8e1456105dad, 0xdd50f1996b947518, 0x8a5296ffe33cc92f, 0xace73cbfdc0bfb7b, 0xd8210befd30efa5a,
      0x8714a775e3e95c78, 0xa8d9d1535ce3b396, 0xd31045a8341ca07c, 0x83ea2b892091e44d, 0xa4e4b66b68b65d60, 0xce1de40642e3f4b9,
      0x80d2ae83e9ce78f3, 0xa1075a24e4421730, 0xc94930ae1d529cfc, 0xfb9b7cd9a4a7443c, 0x9d412e0806e88aa5, 0xc491798a08a2ad4e,
      0xf5b5d7ec8acb58a2, 0x9991a6f3d6bf1765, 0xbff610b0cc6edd3f, 0xeff394dcff8a948e, 0x95f83d0a1fb69cd9, 0xbb764c4ca7a4440f,
      0xea53df5fd18d5513, 0x92746b9be2f8552c, 0xb7118682dbb66a77, 0xe4d5e82392a40515, 0x8f05b1163ba6832d, 0xb2c71d5bca9023f8,
      0xdf78e4b2bd342cf6, 0x8bab8eefb6409c1a, 0xae9672aba3d0c320, 0xda3c0f568cc4f3e8, 0x8865899617fb1871, 0xaa7eebfb9df9de8d,
      0xd51ea6fa85785631, 0x8533285c936b35de, 0xa67ff273b8460356, 0xd01fef10a657842c, 0x8213f56a67f6b29b, 0xa298f2c501f45f42,
      0xcb3f2f7642717713, 0xfe0efb53d30dd4d7, 0x9ec95d1463e8a506, 0xc67bb4597ce2ce48, 0xf81aa16fdc1b81da, 0x9b10a4e5e9913128,
      0xc1d4ce1f63f57d72, 0xf24a01a73cf2dccf, 0x976e41088617ca01, 0xbd49d14aa79dbc82, 0xec9c459d51852ba2, 0x93e1ab8252f33b45,
      0xb8da1662e7b00a17, 0xe7109bfba19c0c9d, 0x906a617d450187e2, 0xb484f9dc9641e9da, 0xe1a63853bbd26451, 0x8d07e33455637eb2,
      0xb049dc016abc5e5f, 0xdc5c5301c56b75f7, 0x89b9b3e11b6329ba, 0xac2820d9623bf429, 0xd732290fbacaf133, 0x867f59a9d4bed6c0,
      0xa81f301449ee8c70, 0xd226fc195c6a2f8c, 0x83585d8fd9c25db7, 0xa42e74f3d032f525, 0xcd3a1230c43fb26f, 0x80444b5e7aa7cf85,
      0xa0555e361951c366, 0xc86ab5c39fa63440, 0xfa856334878fc150, 0x9c935e00d4b9d8d2, 0xc3b8358109e84f07, 0xf4a642e14c6262c8,
      0x98e7e9cccfbd7dbd, 0xbf21e44003acdd2c, 0xeeea5d5004981478, 0x95527a5202df0ccb, 0xbaa718e68396cffd, 0xe950df20247c83fd,
      0x91d28b7416cdd27e, 0xb6472e511c81471d, 0xe3d8f9e563a198e5, 0x8e679c2f5e44ff8f};
  // A complement to mantissa_64
  // complete to a 128-bit mantissa.
  // Uses about 5KB but is rarely accessed.
  static const uint64_t mantissa_128[] = {
      0x419ea3bd35385e2d, 0x52064cac828675b9, 0x7343efebd1940993, 0x1014ebe6c5f90bf8, 0xd41a26e077774ef6, 0x8920b098955522b4,
      0x55b46e5f5d5535b0, 0xeb2189f734aa831d, 0xa5e9ec7501d523e4, 0x47b233c92125366e, 0x999ec0bb696e840a, 0xc00670ea43ca250d,
      0x380406926a5e5728, 0xc605083704f5ecf2, 0xf7864a44c633682e, 0x7ab3ee6afbe0211d, 0x5960ea05bad82964, 0x6fb92487298e33bd,
      0xa5d3b6d479f8e056, 0x8f48a4899877186c, 0x331acdabfe94de87, 0x9ff0c08b7f1d0b14, 0x7ecf0ae5ee44dd9, 0xc9e82cd9f69d6150,
      0xbe311c083a225cd2, 0x6dbd630a48aaf406, 0x92cbbccdad5b108, 0x25bbf56008c58ea5, 0xaf2af2b80af6f24e, 0x1af5af660db4aee1,
      0x50d98d9fc890ed4d, 0xe50ff107bab528a0, 0x1e53ed49a96272c8, 0x25e8e89c13bb0f7a, 0x77b191618c54e9ac, 0xd59df5b9ef6a2417,
      0x4b0573286b44ad1d, 0x4ee367f9430aec32, 0x229c41f793cda73f, 0x6b43527578c1110f, 0x830a13896b78aaa9, 0x23cc986bc656d553,
      0x2cbfbe86b7ec8aa8, 0x7bf7d71432f3d6a9, 0xdaf5ccd93fb0cc53, 0xd1b3400f8f9cff68, 0x23100809b9c21fa1, 0xabd40a0c2832a78a,
      0x16c90c8f323f516c, 0xae3da7d97f6792e3, 0x99cd11cfdf41779c, 0x40405643d711d583, 0x482835ea666b2572, 0xda3243650005eecf,
      0x90bed43e40076a82, 0x5a7744a6e804a291, 0x711515d0a205cb36, 0xd5a5b44ca873e03, 0xe858790afe9486c2, 0x626e974dbe39a872,
      0xfb0a3d212dc8128f, 0x7ce66634bc9d0b99, 0x1c1fffc1ebc44e80, 0xa327ffb266b56220, 0x4bf1ff9f0062baa8, 0x6f773fc3603db4a9,
      0xcb550fb4384d21d3, 0x7e2a53a146606a48, 0x2eda7444cbfc426d, 0xfa911155fefb5308, 0x793555ab7eba27ca, 0x4bc1558b2f3458de,
      0x9eb1aaedfb016f16, 0x465e15a979c1cadc, 0xbfacd89ec191ec9, 0xcef980ec671f667b, 0x82b7e12780e7401a, 0xd1b2ecb8b0908810,
      0x861fa7e6dcb4aa15, 0x67a791e093e1d49a, 0xe0c8bb2c5c6d24e0, 0x58fae9f773886e18, 0xaf39a475506a899e, 0x6d8406c952429603,
      0xc8e5087ba6d33b83, 0xfb1e4a9a90880a64, 0x5cf2eea09a55067f, 0xf42faa48c0ea481e, 0xf13b94daf124da26, 0x76c53d08d6b70858,
      0x54768c4b0c64ca6e, 0xa9942f5dcf7dfd09, 0xd3f93b35435d7c4c, 0xc47bc5014a1a6daf, 0x359ab6419ca1091b, 0xc30163d203c94b62,
      0x79e0de63425dcf1d, 0x985915fc12f542e4, 0x3e6f5b7b17b2939d, 0xa705992ceecf9c42, 0x50c6ff782a838353, 0xa4f8bf5635246428,
      0x871b7795e136be99, 0x28e2557b59846e3f, 0x331aeada2fe589cf, 0x3ff0d2c85def7621, 0xfed077a756b53a9, 0xd3e8495912c62894,
      0x64712dd7abbbd95c, 0xbd8d794d96aacfb3, 0xecf0d7a0fc5583a0, 0xf41686c49db57244, 0x311c2875c522ced5, 0x7d633293366b828b,
      0xae5dff9c02033197, 0xd9f57f830283fdfc, 0xd072df63c324fd7b, 0x4247cb9e59f71e6d, 0x52d9be85f074e608, 0x67902e276c921f8b,
      0xba1cd8a3db53b6, 0x80e8a40eccd228a4, 0x6122cd128006b2cd, 0x796b805720085f81, 0xcbe3303674053bb0, 0xbedbfc4411068a9c,
      0xee92fb5515482d44, 0x751bdd152d4d1c4a, 0xd262d45a78a0635d, 0x86fb897116c87c34, 0xd45d35e6ae3d4da0, 0x8974836059cca109,
      0x2bd1a438703fc94b, 0x7b6306a34627ddcf, 0x1a3bc84c17b1d542, 0x20caba5f1d9e4a93, 0x547eb47b7282ee9c, 0xe99e619a4f23aa43,
      0x6405fa00e2ec94d4, 0xde83bc408dd3dd04, 0x9624ab50b148d445, 0x3badd624dd9b0957, 0xe54ca5d70a80e5d6, 0x5e9fcf4ccd211f4c,
      0x7647c3200069671f, 0x29ecd9f40041e073, 0xf468107100525890, 0x7182148d4066eeb4, 0xc6f14cd848405530, 0xb8ada00e5a506a7c,
      0xa6d90811f0e4851c, 0x908f4a166d1da663, 0x9a598e4e043287fe, 0x40eff1e1853f29fd, 0xd12bee59e68ef47c, 0x82bb74f8301958ce,
      0xe36a52363c1faf01, 0xdc44e6c3cb279ac1, 0x29ab103a5ef8c0b9, 0x7415d448f6b6f0e7, 0x111b495b3464ad21, 0xcab10dd900beec34,
      0x3d5d514f40eea742, 0xcb4a5a3112a5112, 0x47f0e785eaba72ab, 0x59ed216765690f56, 0x306869c13ec3532c, 0x1e414218c73a13fb,
      0xe5d1929ef90898fa, 0xdf45f746b74abf39, 0x6b8bba8c328eb783, 0x66ea92f3f326564, 0xc80a537b0efefebd, 0xbd06742ce95f5f36,
      0x2c48113823b73704, 0xf75a15862ca504c5, 0x9a984d73dbe722fb, 0xc13e60d0d2e0ebba, 0x318df905079926a8, 0xfdf17746497f7052,
      0xfeb6ea8bedefa633, 0xfe64a52ee96b8fc0, 0x3dfdce7aa3c673b0, 0x6bea10ca65c084e, 0x486e494fcff30a62, 0x5a89dba3c3efccfa,
      0xf89629465a75e01c, 0xf6bbb397f1135823, 0x746aa07ded582e2c, 0xa8c2a44eb4571cdc, 0x92f34d62616ce413, 0x77b020baf9c81d17,
      0xace1474dc1d122e, 0xd819992132456ba, 0x10e1fff697ed6c69, 0xca8d3ffa1ef463c1, 0xbd308ff8a6b17cb2, 0xac7cb3f6d05ddbde,
      0x6bcdf07a423aa96b, 0x86c16c98d2c953c6, 0xe871c7bf077ba8b7, 0x11471cd764ad4972, 0xd598e40d3dd89bcf, 0x4aff1d108d4ec2c3,
      0xcedf722a585139ba, 0xc2974eb4ee658828, 0x733d226229feea32, 0x806357d5a3f525f, 0xca07c2dcb0cf26f7, 0xfc89b393dd02f0b5,
      0xbbac2078d443ace2, 0xd54b944b84aa4c0d, 0xa9e795e65d4df11, 0x4d4617b5ff4a16d5, 0x504bced1bf8e4e45, 0xe45ec2862f71e1d6,
      0x5d767327bb4e5a4c, 0x3a6a07f8d510f86f, 0x890489f70a55368b, 0x2b45ac74ccea842e, 0x3b0b8bc90012929d, 0x9ce6ebb40173744,
      0xcc420a6a101d0515, 0x9fa946824a12232d, 0x47939822dc96abf9, 0x59787e2b93bc56f7, 0x57eb4edb3c55b65a, 0xede622920b6b23f1,
      0xe95fab368e45eced, 0x11dbcb0218ebb414, 0xd652bdc29f26a119, 0x4be76d3346f0495f, 0x6f70a4400c562ddb, 0xcb4ccd500f6bb952,
      0x7e2000a41346a7a7, 0x8ed400668c0c28c8, 0x728900802f0f32fa, 0x4f2b40a03ad2ffb9, 0xe2f610c84987bfa8, 0xdd9ca7d2df4d7c9,
      0x91503d1c79720dbb, 0x75a44c6397ce912a, 0xc986afbe3ee11aba, 0xfbe85badce996168, 0xfae27299423fb9c3, 0xdccd879fc967d41a,
      0x5400e987bbc1c920, 0x290123e9aab23b68, 0xf9a0b6720aaf6521, 0xf808e40e8d5b3e69, 0xb60b1d1230b20e04, 0xb1c6f22b5e6f48c2,
      0x1e38aeb6360b1af3, 0x25c6da63c38de1b0, 0x579c487e5a38ad0e, 0x2d835a9df0c6d851, 0xf8e431456cf88e65, 0x1b8e9ecb641b58ff,
      0xe272467e3d222f3f, 0x5b0ed81dcc6abb0f, 0x98e947129fc2b4e9, 0x3f2398d747b36224, 0x8eec7f0d19a03aad, 0x1953cf68300424ac,
      0x5fa8c3423c052dd7, 0x3792f412cb06794d, 0xe2bbd88bbee40bd0, 0x5b6aceaeae9d0ec4, 0xf245825a5a445275, 0xeed6e2f0f0d56712,
      0x55464dd69685606b, 0xaa97e14c3c26b886, 0xd53dd99f4b3066a8, 0xe546a8038efe4029, 0xde98520472bdd033, 0x963e66858f6d4440,
      0xdde7001379a44aa8, 0x5560c018580d5d52, 0xaab8f01e6e10b4a6, 0xcab3961304ca70e8, 0x3d607b97c5fd0d22, 0x8cb89a7db77c506a,
      0x77f3608e92adb242, 0x55f038b237591ed3, 0x6b6c46dec52f6688, 0x2323ac4b3b3da015, 0xabec975e0a0d081a, 0x96e7bd358c904a21,
      0x7e50d64177da2e54, 0xdde50bd1d5d0b9e9, 0x955e4ec64b44e864, 0xbd5af13bef0b113e, 0xecb1ad8aeacdd58e, 0x67de18eda5814af2,
      0x80eacf948770ced7, 0xa1258379a94d028d, 0x96ee45813a04330, 0x8bca9d6e188853fc, 0x775ea264cf55347d, 0x95364afe032a819d,
      0x3a83ddbd83f52204, 0xc4926a9672793542, 0x75b7053c0f178293, 0x5324c68b12dd6338, 0xd3f6fc16ebca5e03, 0x88f4bb1ca6bcf584,
      0x2b31e9e3d06c32e5, 0x3aff322e62439fcf, 0x9befeb9fad487c2, 0x4c2ebe687989a9b3, 0xf9d37014bf60a10, 0x538484c19ef38c94,
      0x2865a5f206b06fb9, 0xf93f87b7442e45d3, 0xf78f69a51539d748, 0xb573440e5a884d1b, 0x31680a88f8953030, 0xfdc20d2b36ba7c3d,
      0x3d32907604691b4c, 0xa63f9a49c2c1b10f, 0xfcf80dc33721d53, 0xd3c36113404ea4a8, 0x645a1cac083126e9, 0x3d70a3d70a3d70a3,
      0xcccccccccccccccc, 0x0, 0x0, 0x0, 0x0, 0x0,
      0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
      0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
      0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
      0x0, 0x0, 0x0, 0x0, 0x0, 0x4000000000000000,
      0x5000000000000000, 0xa400000000000000, 0x4d00000000000000, 0xf020000000000000, 0x6c28000000000000, 0xc732000000000000,
      0x3c7f400000000000, 0x4b9f100000000000, 0x1e86d40000000000, 0x1314448000000000, 0x17d955a000000000, 0x5dcfab0800000000,
      0x5aa1cae500000000, 0xf14a3d9e40000000, 0x6d9ccd05d0000000, 0xe4820023a2000000, 0xdda2802c8a800000, 0xd50b2037ad200000,
      0x4526f422cc340000, 0x9670b12b7f410000, 0x3c0cdd765f114000, 0xa5880a69fb6ac800, 0x8eea0d047a457a00, 0x72a4904598d6d880,
      0x47a6da2b7f864750, 0x999090b65f67d924, 0xfff4b4e3f741cf6d, 0xbff8f10e7a8921a4, 0xaff72d52192b6a0d, 0x9bf4f8a69f764490,
      0x2f236d04753d5b4, 0x1d762422c946590, 0x424d3ad2b7b97ef5, 0xd2e0898765a7deb2, 0x63cc55f49f88eb2f, 0x3cbf6b71c76b25fb,
      0x8bef464e3945ef7a, 0x97758bf0e3cbb5ac, 0x3d52eeed1cbea317, 0x4ca7aaa863ee4bdd, 0x8fe8caa93e74ef6a, 0xb3e2fd538e122b44,
      0x60dbbca87196b616, 0xbc8955e946fe31cd, 0x6babab6398bdbe41, 0xc696963c7eed2dd1, 0xfc1e1de5cf543ca2, 0x3b25a55f43294bcb,
      0x49ef0eb713f39ebe, 0x6e3569326c784337, 0x49c2c37f07965404, 0xdc33745ec97be906, 0x69a028bb3ded71a3, 0xc40832ea0d68ce0c,
      0xf50a3fa490c30190, 0x792667c6da79e0fa, 0x577001b891185938, 0xed4c0226b55e6f86, 0x544f8158315b05b4, 0x696361ae3db1c721,
      0x3bc3a19cd1e38e9, 0x4ab48a04065c723, 0x62eb0d64283f9c76, 0x3ba5d0bd324f8394, 0xca8f44ec7ee36479, 0x7e998b13cf4e1ecb,
      0x9e3fedd8c321a67e, 0xc5cfe94ef3ea101e, 0xbba1f1d158724a12, 0x2a8a6e45ae8edc97, 0xf52d09d71a3293bd, 0x593c2626705f9c56,
      0x6f8b2fb00c77836c, 0xb6dfb9c0f956447, 0x4724bd4189bd5eac, 0x58edec91ec2cb657, 0x2f2967b66737e3ed, 0xbd79e0d20082ee74,
      0xecd8590680a3aa11, 0xe80e6f4820cc9495, 0x3109058d147fdcdd, 0xbd4b46f0599fd415, 0x6c9e18ac7007c91a, 0x3e2cf6bc604ddb0,
      0x84db8346b786151c, 0xe612641865679a63, 0x4fcb7e8f3f60c07e, 0xe3be5e330f38f09d, 0x5cadf5bfd3072cc5, 0x73d9732fc7c8f7f6,
      0x2867e7fddcdd9afa, 0xb281e1fd541501b8, 0x1f225a7ca91a4226, 0x3375788de9b06958, 0x52d6b1641c83ae, 0xc0678c5dbd23a49a,
      0xf840b7ba963646e0, 0xb650e5a93bc3d898, 0xa3e51f138ab4cebe, 0xc66f336c36b10137, 0xb80b0047445d4184, 0xa60dc059157491e5,
      0x87c89837ad68db2f, 0x29babe4598c311fb, 0xf4296dd6fef3d67a, 0x1899e4a65f58660c, 0x5ec05dcff72e7f8f, 0x76707543f4fa1f73,
      0x6a06494a791c53a8, 0x487db9d17636892, 0x45a9d2845d3c42b6, 0xb8a2392ba45a9b2, 0x8e6cac7768d7141e, 0x3207d795430cd926,
      0x7f44e6bd49e807b8, 0x5f16206c9c6209a6, 0x36dba887c37a8c0f, 0xc2494954da2c9789, 0xf2db9baa10b7bd6c, 0x6f92829494e5acc7,
      0xcb772339ba1f17f9, 0xff2a760414536efb, 0xfef5138519684aba, 0x7eb258665fc25d69, 0xef2f773ffbd97a61, 0xaafb550ffacfd8fa,
      0x95ba2a53f983cf38, 0xdd945a747bf26183, 0x94f971119aeef9e4, 0x7a37cd5601aab85d, 0xac62e055c10ab33a, 0x577b986b314d6009,
      0xed5a7e85fda0b80b, 0x14588f13be847307, 0x596eb2d8ae258fc8, 0x6fca5f8ed9aef3bb, 0x25de7bb9480d5854, 0xaf561aa79a10ae6a,
      0x1b2ba1518094da04, 0x90fb44d2f05d0842, 0x353a1607ac744a53, 0x42889b8997915ce8, 0x69956135febada11, 0x43fab9837e699095,
      0x94f967e45e03f4bb, 0x1d1be0eebac278f5, 0x6462d92a69731732, 0x7d7b8f7503cfdcfe, 0x5cda735244c3d43e, 0x3a0888136afa64a7,
      0x88aaa1845b8fdd0, 0x8aad549e57273d45, 0x36ac54e2f678864b, 0x84576a1bb416a7dd, 0x656d44a2a11c51d5, 0x9f644ae5a4b1b325,
      0x873d5d9f0dde1fee, 0xa90cb506d155a7ea, 0x9a7f12442d588f2, 0xc11ed6d538aeb2f, 0x8f1668c8a86da5fa, 0xf96e017d694487bc,
      0x37c981dcc395a9ac, 0x85bbe253f47b1417, 0x93956d7478ccec8e, 0x387ac8d1970027b2, 0x6997b05fcc0319e, 0x441fece3bdf81f03,
      0xd527e81cad7626c3, 0x8a71e223d8d3b074, 0xf6872d5667844e49, 0xb428f8ac016561db, 0xe13336d701beba52, 0xecc0024661173473,
      0x27f002d7f95d0190, 0x31ec038df7b441f4, 0x7e67047175a15271, 0xf0062c6e984d386, 0x52c07b78a3e60868, 0xa7709a56ccdf8a82,
      0x88a66076400bb691, 0x6acff893d00ea435, 0x583f6b8c4124d43, 0xc3727a337a8b704a, 0x744f18c0592e4c5c, 0x1162def06f79df73,
      0x8addcb5645ac2ba8, 0x6d953e2bd7173692, 0xc8fa8db6ccdd0437, 0x1d9c9892400a22a2, 0x2503beb6d00cab4b, 0x2e44ae64840fd61d,
      0x5ceaecfed289e5d2, 0x7425a83e872c5f47, 0xd12f124e28f77719, 0x82bd6b70d99aaa6f, 0x636cc64d1001550b, 0x3c47f7e05401aa4e,
      0x65acfaec34810a71, 0x7f1839a741a14d0d, 0x1ede48111209a050, 0x934aed0aab460432, 0xf81da84d5617853f, 0x36251260ab9d668e,
      0xc1d72b7c6b426019, 0xb24cf65b8612f81f, 0xdee033f26797b627, 0x169840ef017da3b1, 0x8e1f289560ee864e, 0xf1a6f2bab92a27e2,
      0xae10af696774b1db, 0xacca6da1e0a8ef29, 0x17fd090a58d32af3, 0xddfc4b4cef07f5b0, 0x4abdaf101564f98e, 0x9d6d1ad41abe37f1,
      0x84c86189216dc5ed, 0x32fd3cf5b4e49bb4, 0x3fbc8c33221dc2a1, 0xfabaf3feaa5334a, 0x29cb4d87f2a7400e, 0x743e20e9ef511012,
      0x914da9246b255416, 0x1ad089b6c2f7548e, 0xa184ac2473b529b1, 0xc9e5d72d90a2741e, 0x7e2fa67c7a658892, 0xddbb901b98feeab7,
      0x552a74227f3ea565, 0xd53a88958f87275f, 0x8a892abaf368f137, 0x2d2b7569b0432d85, 0x9c3b29620e29fc73, 0x8349f3ba91b47b8f,
      0x241c70a936219a73, 0xed238cd383aa0110, 0xf4363804324a40aa, 0xb143c6053edcd0d5, 0xdd94b7868e94050a, 0xca7cf2b4191c8326,
      0xfd1c2f611f63a3f0, 0xbc633b39673c8cec, 0xd5be0503e085d813, 0x4b2d8644d8a74e18, 0xddf8e7d60ed1219e, 0xcabb90e5c942b503,
      0x3d6a751f3b936243, 0xcc512670a783ad4, 0x27fb2b80668b24c5, 0xb1f9f660802dedf6, 0x5e7873f8a0396973, 0xdb0b487b6423e1e8,
      0x91ce1a9a3d2cda62, 0x7641a140cc7810fb, 0xa9e904c87fcb0a9d, 0x546345fa9fbdcd44, 0xa97c177947ad4095, 0x49ed8eabcccc485d,
      0x5c68f256bfff5a74, 0x73832eec6fff3111, 0xc831fd53c5ff7eab, 0xba3e7ca8b77f5e55, 0x28ce1bd2e55f35eb, 0x7980d163cf5b81b3,
      0xd7e105bcc332621f, 0x8dd9472bf3fefaa7, 0xb14f98f6f0feb951, 0x6ed1bf9a569f33d3, 0xa862f80ec4700c8, 0xcd27bb612758c0fa,
      0x8038d51cb897789c, 0xe0470a63e6bd56c3, 0x1858ccfce06cac74, 0xf37801e0c43ebc8, 0xd30560258f54e6ba, 0x47c6b82ef32a2069,
      0x4cdc331d57fa5441, 0xe0133fe4adf8e952, 0x58180fddd97723a6, 0x570f09eaa7ea7648, };

#if (FLT_EVAL_METHOD != 1) && (FLT_EVAL_METHOD != 0)
  // we do not trust the divisor
  if (0 <= power && power <= 22 && i <= 9007199254740991) {
#else
  if (-22 <= power && power <= 22 && i <= 9007199254740991) {
#endif
    // convert the integer into a double. This is lossless since
    // 0 <= i <= 2^53 - 1.
    double d = double(i);
    if (power < 0) {
      d = d / power_of_ten[-power];
    } else {
      d = d * power_of_ten[power];
    }
    if (negative) {
      d = -d;
    }
    *success = true;
    return d;
  }
  if(i == 0) {
    return negative ? -0.0 : 0.0;
  }
  uint64_t factor_mantissa = mantissa_64[power - FASTFLOAT_SMALLEST_POWER];
  int64_t exponent = (((152170 + 65536) * power) >> 16) + 1024 + 63;
  // We want the most significant bit of i to be 1. Shift if needed.
  int lz = leading_zeroes(i);
  i <<= lz;
  // We want the most significant 64 bits of the product. We know
  // this will be non-zero because the most significant bit of i is
  // 1.
  value128 product = full_multiplication(i, factor_mantissa);
  uint64_t lower = product.low;
  uint64_t upper = product.high;
  if (unlikely((upper & 0x1FF) == 0x1FF) && (lower + i < lower)) {
    uint64_t factor_mantissa_low =
        mantissa_128[power - FASTFLOAT_SMALLEST_POWER];
    // next, we compute the 64-bit x 128-bit multiplication, getting a 192-bit
    // result (three 64-bit values)
    product = full_multiplication(i, factor_mantissa_low);
    uint64_t product_low = product.low;
    uint64_t product_middle2 = product.high;
    uint64_t product_middle1 = lower;
    uint64_t product_high = upper;
    uint64_t product_middle = product_middle1 + product_middle2;
    if (product_middle < product_middle1) {
      product_high++; // overflow carry
    }
    // we want to check whether mantissa *i + i would affect our result
    // This does happen, e.g. with 7.3177701707893310e+15
    if (((product_middle + 1 == 0) && ((product_high & 0x1FF) == 0x1FF) &&
         (product_low + i < product_low))) { // let us be prudent and bail out.
      *success = false;
      return 0;
    }
    upper = product_high;
    lower = product_middle;
  }
  // The final mantissa should be 53 bits with a leading 1.
  // We shift it so that it occupies 54 bits with a leading 1.
  ///////
  uint64_t upperbit = upper >> 63;
  uint64_t mantissa = upper >> (upperbit + 9);
  lz += int(1 ^ upperbit);
  if (unlikely((lower == 0) && ((upper & 0x1FF) == 0) &&
               ((mantissa & 3) == 1))) {
      *success = false;
      return 0;
  }
  mantissa += mantissa & 1;
  mantissa >>= 1;
  // Here we have mantissa < (1<<53), unless there was an overflow
  if (mantissa >= (1ULL << 53)) {
    //////////
    // This will happen when parsing values such as 7.2057594037927933e+16
    ////////
    mantissa = (1ULL << 52);
    lz--; // undo previous addition
  }
  mantissa &= ~(1ULL << 52);
  uint64_t real_exponent = exponent - lz;
  // we have to check that real_exponent is in range, otherwise we bail out
  if (unlikely((real_exponent < 1) || (real_exponent > 2046))) {
    *success = false;
    return 0;
  }
  mantissa |= real_exponent << 52;
  mantissa |= (((uint64_t)negative) << 63);
  double d;
  memcpy(&d, &mantissa, sizeof(d));
  *success = true;
  return d;
}
// Return the null pointer on error
static const char * parse_float_strtod(const char *ptr, double *outDouble) {
  char *endptr;
#if defined(FAST_DOUBLE_PARSER_SOLARIS) || defined(FAST_DOUBLE_PARSER_CYGWIN)
  // workround for cygwin, solaris
  *outDouble = cygwin_strtod_l(ptr, &endptr);
#elif defined(_WIN32)
  static _locale_t c_locale = _create_locale(LC_ALL, "C");
  *outDouble = _strtod_l(ptr, &endptr, c_locale);
#else
  static locale_t c_locale = newlocale(LC_ALL_MASK, "C", NULL);
  *outDouble = strtod_l(ptr, &endptr, c_locale);
#endif
  if (!std::isfinite(*outDouble)) {
    return nullptr;
  }
  return endptr;
}

really_inline const char * parse_number(const char *p, double *outDouble) {
  const char *pinit = p;
  bool found_minus = (*p == '-');
  bool negative = false;
  if (found_minus) {
    ++p;
    negative = true;
    if (!is_integer(*p)) { // a negative sign must be followed by an integer
      return nullptr;
    }
  }
  const char *const start_digits = p;

  uint64_t i;      // an unsigned int avoids signed overflows (which are bad)
  if (*p == '0') { // 0 cannot be followed by an integer
    ++p;
    if (is_integer(*p)) {
      return nullptr;
    }
    i = 0;
  } else {
    if (!(is_integer(*p))) { // must start with an integer
      return nullptr;
    }
    unsigned char digit = *p - '0';
    i = digit;
    p++;
    // the is_made_of_eight_digits_fast routine is unlikely to help here because
    // we rarely see large integer parts like 123456789
    while (is_integer(*p)) {
      digit = *p - '0';
      // a multiplication by 10 is cheaper than an arbitrary integer
      // multiplication
      i = 10 * i + digit; // might overflow, we will handle the overflow later
      ++p;
    }
  }
  int64_t exponent = 0;
  const char *first_after_period = NULL;
  if (*p == '.') {
    ++p;
    first_after_period = p;
    if (is_integer(*p)) {
      unsigned char digit = *p - '0';
      ++p;
      i = i * 10 + digit; // might overflow + multiplication by 10 is likely
                          // cheaper than arbitrary mult.
      // we will handle the overflow later
    } else {
      return nullptr;
    }
    while (is_integer(*p)) {
      unsigned char digit = *p - '0';
      ++p;
      i = i * 10 + digit; // in rare cases, this will overflow, but that's ok
                          // because we have parse_highprecision_float later.
    }
    exponent = first_after_period - p;
  }
  int digit_count =
      int(p - start_digits - 1); // used later to guard against overflows
  if (('e' == *p) || ('E' == *p)) {
    ++p;
    bool neg_exp = false;
    if ('-' == *p) {
      neg_exp = true;
      ++p;
    } else if ('+' == *p) {
      ++p;
    }
    if (!is_integer(*p)) {
      return nullptr;
    }
    unsigned char digit = *p - '0';
    int64_t exp_number = digit;
    p++;
    if (is_integer(*p)) {
      digit = *p - '0';
      exp_number = 10 * exp_number + digit;
      ++p;
    }
    if (is_integer(*p)) {
      digit = *p - '0';
      exp_number = 10 * exp_number + digit;
      ++p;
    }
    while (is_integer(*p)) {
      digit = *p - '0';
      if (exp_number < 0x100000000) { // we need to check for overflows
        exp_number = 10 * exp_number + digit;
      }
      ++p;
    }
    exponent += (neg_exp ? -exp_number : exp_number);
  }
  // If we frequently had to deal with long strings of digits,
  // we could extend our code by using a 128-bit integer instead
  // of a 64-bit integer. However, this is uncommon.
  if (unlikely((digit_count >= 19))) { // this is uncommon
    // It is possible that the integer had an overflow.
    // We have to handle the case where we have 0.0000somenumber.
    const char *start = start_digits;
    while (*start == '0' || (*start == '.')) {
      start++;
    }
    // we over-decrement by one when there is a decimal separator
    digit_count -= int(start - start_digits);
    if (digit_count >= 19) {
      // Chances are good that we had an overflow!
      // We start anew.
      // This will happen in the following examples:
      // 10000000000000000000000000000000000000000000e+308
      // 3.1415926535897932384626433832795028841971693993751
      //
      return parse_float_strtod(pinit, outDouble);
    }
  }
  if (unlikely(exponent < FASTFLOAT_SMALLEST_POWER) ||
      (exponent > FASTFLOAT_LARGEST_POWER)) {
    // this is almost never going to get called!!!
    // exponent could be as low as 325
    return parse_float_strtod(pinit, outDouble);
  }
  // from this point forward, exponent >= FASTFLOAT_SMALLEST_POWER and
  // exponent <= FASTFLOAT_LARGEST_POWER
  bool success = true;
  *outDouble = compute_float_64(exponent, i, negative, &success);
  if (!success) {
    // we are almost never going to get here.
    return parse_float_strtod(pinit, outDouble);
  }
  return p;
}

}
namespace dtoa_impl
{
/*!
@brief implements the Grisu2 algorithm for binary to decimal floating-point
conversion.
*/
template <typename Target, typename Source>
Target reinterpret_bits(const Source source)
{
    static_assert(sizeof(Target) == sizeof(Source), "size mismatch");

    Target target;
    std::memcpy(&target, &source, sizeof(Source));
    return target;
}

struct diyfp // f * 2^e
{
    static constexpr int kPrecision = 64; // = q

    std::uint64_t f = 0;
    int e = 0;

    constexpr diyfp(std::uint64_t f_, int e_) noexcept : f(f_), e(e_) {}

    /*!
    @brief returns x - y
    @pre x.e == y.e and x.f >= y.f
    */
    static diyfp sub(const diyfp& x, const diyfp& y) noexcept
    {
        assert(x.e == y.e);
        assert(x.f >= y.f);

        return {x.f - y.f, x.e};
    }

    /*!
    @brief returns x * y
    @note The result is rounded. (Only the upper q bits are returned.)
    */
    static diyfp mul(const diyfp& x, const diyfp& y) noexcept
    {
        static_assert(kPrecision == 64, "internal error");

        const std::uint64_t u_lo = x.f & 0xFFFFFFFFu;
        const std::uint64_t u_hi = x.f >> 32u;
        const std::uint64_t v_lo = y.f & 0xFFFFFFFFu;
        const std::uint64_t v_hi = y.f >> 32u;

        const std::uint64_t p0 = u_lo * v_lo;
        const std::uint64_t p1 = u_lo * v_hi;
        const std::uint64_t p2 = u_hi * v_lo;
        const std::uint64_t p3 = u_hi * v_hi;

        const std::uint64_t p0_hi = p0 >> 32u;
        const std::uint64_t p1_lo = p1 & 0xFFFFFFFFu;
        const std::uint64_t p1_hi = p1 >> 32u;
        const std::uint64_t p2_lo = p2 & 0xFFFFFFFFu;
        const std::uint64_t p2_hi = p2 >> 32u;

        std::uint64_t Q = p0_hi + p1_lo + p2_lo;

        Q += std::uint64_t{1} << (64u - 32u - 1u); // round, ties up

        const std::uint64_t h = p3 + p2_hi + p1_hi + (Q >> 32u);

        return {h, x.e + y.e + 64};
    }

    /*!
    @brief normalize x such that the significand is >= 2^(q-1)
    @pre x.f != 0
    */
    static diyfp normalize(diyfp x) noexcept
    {
        assert(x.f != 0);

        while ((x.f >> 63u) == 0)
        {
            x.f <<= 1u;
            x.e--;
        }

        return x;
    }

    /*!
    @brief normalize x such that the result has the exponent E
    @pre e >= x.e and the upper e - x.e bits of x.f must be zero.
    */
    static diyfp normalize_to(const diyfp& x, const int target_exponent) noexcept
    {
        const int delta = x.e - target_exponent;

        assert(delta >= 0);
        assert(((x.f << delta) >> delta) == x.f);

        return {x.f << delta, target_exponent};
    }
};

struct boundaries
{
    diyfp w;
    diyfp minus;
    diyfp plus;
};

/*!
Compute the (normalized) diyfp representing the input number 'value' and its
boundaries.

@pre value must be finite and positive
*/
template <typename FloatType>
boundaries compute_boundaries(FloatType value)
{
    assert(std::isfinite(value));
    assert(value > 0);

    static_assert(std::numeric_limits<FloatType>::is_iec559,
                  "internal error: dtoa_short requires an IEEE-754 floating-point implementation");

    constexpr int      kPrecision = std::numeric_limits<FloatType>::digits; // = p (includes the hidden bit)
    constexpr int      kBias      = std::numeric_limits<FloatType>::max_exponent - 1 + (kPrecision - 1);
    constexpr int      kMinExp    = 1 - kBias;
    constexpr std::uint64_t kHiddenBit = std::uint64_t{1} << (kPrecision - 1); // = 2^(p-1)

    using bits_type = typename std::conditional<kPrecision == 24, std::uint32_t, std::uint64_t >::type;

    const std::uint64_t bits = reinterpret_bits<bits_type>(value);
    const std::uint64_t E = bits >> (kPrecision - 1);
    const std::uint64_t F = bits & (kHiddenBit - 1);

    const bool is_denormal = E == 0;
    const diyfp v = is_denormal
                    ? diyfp(F, kMinExp)
                    : diyfp(F + kHiddenBit, static_cast<int>(E) - kBias);

    const bool lower_boundary_is_closer = F == 0 and E > 1;
    const diyfp m_plus = diyfp(2 * v.f + 1, v.e - 1);
    const diyfp m_minus = lower_boundary_is_closer
                          ? diyfp(4 * v.f - 1, v.e - 2)  // (B)
                          : diyfp(2 * v.f - 1, v.e - 1); // (A)

    // Determine the normalized w+ = m+.
    const diyfp w_plus = diyfp::normalize(m_plus);

    // Determine w- = m- such that e_(w-) = e_(w+).
    const diyfp w_minus = diyfp::normalize_to(m_minus, w_plus.e);

    return {diyfp::normalize(v), w_minus, w_plus};
}

constexpr int kAlpha = -60;
constexpr int kGamma = -32;

struct cached_power // c = f * 2^e ~= 10^k
{
    std::uint64_t f;
    int e;
    int k;
};

/*!
For a normalized diyfp w = f * 2^e, this function returns a (normalized) cached
power-of-ten c = f_c * 2^e_c, such that the exponent of the product w * c
satisfies (Definition 3.2 from [1])

     alpha <= e_c + e + q <= gamma.
*/
inline cached_power get_cached_power_for_binary_exponent(int e)
{
    constexpr int kCachedPowersMinDecExp = -300;
    constexpr int kCachedPowersDecStep = 8;

    static constexpr std::array<cached_power, 79> kCachedPowers =
    {
        {
            { 0xAB70FE17C79AC6CA, -1060, -300 },
            { 0xFF77B1FCBEBCDC4F, -1034, -292 },
            { 0xBE5691EF416BD60C, -1007, -284 },
            { 0x8DD01FAD907FFC3C,  -980, -276 },
            { 0xD3515C2831559A83,  -954, -268 },
            { 0x9D71AC8FADA6C9B5,  -927, -260 },
            { 0xEA9C227723EE8BCB,  -901, -252 },
            { 0xAECC49914078536D,  -874, -244 },
            { 0x823C12795DB6CE57,  -847, -236 },
            { 0xC21094364DFB5637,  -821, -228 },
            { 0x9096EA6F3848984F,  -794, -220 },
            { 0xD77485CB25823AC7,  -768, -212 },
            { 0xA086CFCD97BF97F4,  -741, -204 },
            { 0xEF340A98172AACE5,  -715, -196 },
            { 0xB23867FB2A35B28E,  -688, -188 },
            { 0x84C8D4DFD2C63F3B,  -661, -180 },
            { 0xC5DD44271AD3CDBA,  -635, -172 },
            { 0x936B9FCEBB25C996,  -608, -164 },
            { 0xDBAC6C247D62A584,  -582, -156 },
            { 0xA3AB66580D5FDAF6,  -555, -148 },
            { 0xF3E2F893DEC3F126,  -529, -140 },
            { 0xB5B5ADA8AAFF80B8,  -502, -132 },
            { 0x87625F056C7C4A8B,  -475, -124 },
            { 0xC9BCFF6034C13053,  -449, -116 },
            { 0x964E858C91BA2655,  -422, -108 },
            { 0xDFF9772470297EBD,  -396, -100 },
            { 0xA6DFBD9FB8E5B88F,  -369,  -92 },
            { 0xF8A95FCF88747D94,  -343,  -84 },
            { 0xB94470938FA89BCF,  -316,  -76 },
            { 0x8A08F0F8BF0F156B,  -289,  -68 },
            { 0xCDB02555653131B6,  -263,  -60 },
            { 0x993FE2C6D07B7FAC,  -236,  -52 },
            { 0xE45C10C42A2B3B06,  -210,  -44 },
            { 0xAA242499697392D3,  -183,  -36 },
            { 0xFD87B5F28300CA0E,  -157,  -28 },
            { 0xBCE5086492111AEB,  -130,  -20 },
            { 0x8CBCCC096F5088CC,  -103,  -12 },
            { 0xD1B71758E219652C,   -77,   -4 },
            { 0x9C40000000000000,   -50,    4 },
            { 0xE8D4A51000000000,   -24,   12 },
            { 0xAD78EBC5AC620000,     3,   20 },
            { 0x813F3978F8940984,    30,   28 },
            { 0xC097CE7BC90715B3,    56,   36 },
            { 0x8F7E32CE7BEA5C70,    83,   44 },
            { 0xD5D238A4ABE98068,   109,   52 },
            { 0x9F4F2726179A2245,   136,   60 },
            { 0xED63A231D4C4FB27,   162,   68 },
            { 0xB0DE65388CC8ADA8,   189,   76 },
            { 0x83C7088E1AAB65DB,   216,   84 },
            { 0xC45D1DF942711D9A,   242,   92 },
            { 0x924D692CA61BE758,   269,  100 },
            { 0xDA01EE641A708DEA,   295,  108 },
            { 0xA26DA3999AEF774A,   322,  116 },
            { 0xF209787BB47D6B85,   348,  124 },
            { 0xB454E4A179DD1877,   375,  132 },
            { 0x865B86925B9BC5C2,   402,  140 },
            { 0xC83553C5C8965D3D,   428,  148 },
            { 0x952AB45CFA97A0B3,   455,  156 },
            { 0xDE469FBD99A05FE3,   481,  164 },
            { 0xA59BC234DB398C25,   508,  172 },
            { 0xF6C69A72A3989F5C,   534,  180 },
            { 0xB7DCBF5354E9BECE,   561,  188 },
            { 0x88FCF317F22241E2,   588,  196 },
            { 0xCC20CE9BD35C78A5,   614,  204 },
            { 0x98165AF37B2153DF,   641,  212 },
            { 0xE2A0B5DC971F303A,   667,  220 },
            { 0xA8D9D1535CE3B396,   694,  228 },
            { 0xFB9B7CD9A4A7443C,   720,  236 },
            { 0xBB764C4CA7A44410,   747,  244 },
            { 0x8BAB8EEFB6409C1A,   774,  252 },
            { 0xD01FEF10A657842C,   800,  260 },
            { 0x9B10A4E5E9913129,   827,  268 },
            { 0xE7109BFBA19C0C9D,   853,  276 },
            { 0xAC2820D9623BF429,   880,  284 },
            { 0x80444B5E7AA7CF85,   907,  292 },
            { 0xBF21E44003ACDD2D,   933,  300 },
            { 0x8E679C2F5E44FF8F,   960,  308 },
            { 0xD433179D9C8CB841,   986,  316 },
            { 0x9E19DB92B4E31BA9,  1013,  324 },
        }
    };

    // This computation gives exactly the same results for k as
    //      k = ceil((kAlpha - e - 1) * 0.30102999566398114)
    // for |e| <= 1500, but doesn't require floating-point operations.
    // NB: log_10(2) ~= 78913 / 2^18
    assert(e >= -1500);
    assert(e <=  1500);
    const int f = kAlpha - e - 1;
    const int k = (f * 78913) / (1 << 18) + static_cast<int>(f > 0);

    const int index = (-kCachedPowersMinDecExp + k + (kCachedPowersDecStep - 1)) / kCachedPowersDecStep;
    assert(index >= 0);
    assert(static_cast<std::size_t>(index) < kCachedPowers.size());

    const cached_power cached = kCachedPowers[static_cast<std::size_t>(index)];
    assert(kAlpha <= cached.e + e + 64);
    assert(kGamma >= cached.e + e + 64);

    return cached;
}

/*!
For n != 0, returns k, such that pow10 := 10^(k-1) <= n < 10^k.
For n == 0, returns 1 and sets pow10 := 1.
*/
inline int find_largest_pow10(const std::uint32_t n, std::uint32_t& pow10)
{
    // LCOV_EXCL_START
    if (n >= 1000000000)
    {
        pow10 = 1000000000;
        return 10;
    }
    // LCOV_EXCL_STOP
    else if (n >= 100000000)
    {
        pow10 = 100000000;
        return  9;
    }
    else if (n >= 10000000)
    {
        pow10 = 10000000;
        return  8;
    }
    else if (n >= 1000000)
    {
        pow10 = 1000000;
        return  7;
    }
    else if (n >= 100000)
    {
        pow10 = 100000;
        return  6;
    }
    else if (n >= 10000)
    {
        pow10 = 10000;
        return  5;
    }
    else if (n >= 1000)
    {
        pow10 = 1000;
        return  4;
    }
    else if (n >= 100)
    {
        pow10 = 100;
        return  3;
    }
    else if (n >= 10)
    {
        pow10 = 10;
        return  2;
    }
    else
    {
        pow10 = 1;
        return 1;
    }
}

inline void grisu2_round(char* buf, int len, std::uint64_t dist, std::uint64_t delta,
                         std::uint64_t rest, std::uint64_t ten_k)
{
    assert(len >= 1);
    assert(dist <= delta);
    assert(rest <= delta);
    assert(ten_k > 0);

    while (rest < dist
            and delta - rest >= ten_k
            and (rest + ten_k < dist or dist - rest > rest + ten_k - dist))
    {
        assert(buf[len - 1] != '0');
        buf[len - 1]--;
        rest += ten_k;
    }
}

/*!
Generates V = buffer * 10^decimal_exponent, such that M- <= V <= M+.
M- and M+ must be normalized and share the same exponent -60 <= e <= -32.
*/
inline void grisu2_digit_gen(char* buffer, int& length, int& decimal_exponent,
                             diyfp M_minus, diyfp w, diyfp M_plus)
{
    static_assert(kAlpha >= -60, "internal error");
    static_assert(kGamma <= -32, "internal error");

    assert(M_plus.e >= kAlpha);
    assert(M_plus.e <= kGamma);

    std::uint64_t delta = diyfp::sub(M_plus, M_minus).f; // (significand of (M+ - M-), implicit exponent is e)
    std::uint64_t dist  = diyfp::sub(M_plus, w      ).f; // (significand of (M+ - w ), implicit exponent is e)
    const diyfp one(std::uint64_t{1} << -M_plus.e, M_plus.e);

    auto p1 = static_cast<std::uint32_t>(M_plus.f >> -one.e); // p1 = f div 2^-e (Since -e >= 32, p1 fits into a 32-bit int.)
    std::uint64_t p2 = M_plus.f & (one.f - 1);                    // p2 = f mod 2^-e

    // 1)
    //
    // Generate the digits of the integral part p1 = d[n-1]...d[1]d[0]

    assert(p1 > 0);

    std::uint32_t pow10;
    const int k = find_largest_pow10(p1, pow10);
    int n = k;
    while (n > 0)
    {
        // Invariants:
        //      M+ = buffer * 10^n + (p1 + p2 * 2^e)    (buffer = 0 for n = k)
        //      pow10 = 10^(n-1) <= p1 < 10^n
        //
        const std::uint32_t d = p1 / pow10;  // d = p1 div 10^(n-1)
        const std::uint32_t r = p1 % pow10;  // r = p1 mod 10^(n-1)
        //
        //      M+ = buffer * 10^n + (d * 10^(n-1) + r) + p2 * 2^e
        //         = (buffer * 10 + d) * 10^(n-1) + (r + p2 * 2^e)
        //
        assert(d <= 9);
        buffer[length++] = static_cast<char>('0' + d); // buffer := buffer * 10 + d
        //
        //      M+ = buffer * 10^(n-1) + (r + p2 * 2^e)
        //
        p1 = r;
        n--;
        const std::uint64_t rest = (std::uint64_t{p1} << -one.e) + p2;
        if (rest <= delta)
        {
            // V = buffer * 10^n, with M- <= V <= M+.

            decimal_exponent += n;
            const std::uint64_t ten_n = std::uint64_t{pow10} << -one.e;
            grisu2_round(buffer, length, dist, delta, rest, ten_n);

            return;
        }

        pow10 /= 10;
        //
        //      pow10 = 10^(n-1) <= p1 < 10^n
        // Invariants restored.
    }

    assert(p2 > delta);

    int m = 0;
    for (;;)
    {
        assert(p2 <= (std::numeric_limits<std::uint64_t>::max)() / 10);
        p2 *= 10;
        const std::uint64_t d = p2 >> -one.e;     // d = (10 * p2) div 2^-e
        const std::uint64_t r = p2 & (one.f - 1); // r = (10 * p2) mod 2^-e
        //
        //      M+ = buffer * 10^-m + 10^-m * (1/10 * (d * 2^-e + r) * 2^e
        //         = buffer * 10^-m + 10^-m * (1/10 * (d + r * 2^e))
        //         = (buffer * 10 + d) * 10^(-m-1) + 10^(-m-1) * r * 2^e
        //
        assert(d <= 9);
        buffer[length++] = static_cast<char>('0' + d); // buffer := buffer * 10 + d
        //
        //      M+ = buffer * 10^(-m-1) + 10^(-m-1) * r * 2^e
        //
        p2 = r;
        m++;
        delta *= 10;
        dist  *= 10;
        if (p2 <= delta)
        {
            break;
        }
    }

    // V = buffer * 10^-m, with M- <= V <= M+.

    decimal_exponent -= m;
    const std::uint64_t ten_m = one.f;
    grisu2_round(buffer, length, dist, delta, p2, ten_m);
}

/*!
v = buf * 10^decimal_exponent
len is the length of the buffer (number of decimal digits)
The buffer must be large enough, i.e. >= max_digits10.
*/
inline void grisu2(char* buf, int& len, int& decimal_exponent,
                   diyfp m_minus, diyfp v, diyfp m_plus)
{
    assert(m_plus.e == m_minus.e);
    assert(m_plus.e == v.e);

    const cached_power cached = get_cached_power_for_binary_exponent(m_plus.e);

    const diyfp c_minus_k(cached.f, cached.e); // = c ~= 10^-k

    // The exponent of the products is = v.e + c_minus_k.e + q and is in the range [alpha,gamma]
    const diyfp w       = diyfp::mul(v,       c_minus_k);
    const diyfp w_minus = diyfp::mul(m_minus, c_minus_k);
    const diyfp w_plus  = diyfp::mul(m_plus,  c_minus_k);
    const diyfp M_minus(w_minus.f + 1, w_minus.e);
    const diyfp M_plus (w_plus.f  - 1, w_plus.e );

    decimal_exponent = -cached.k; // = -(-k) = k

    grisu2_digit_gen(buf, len, decimal_exponent, M_minus, w, M_plus);
}

/*!
v = buf * 10^decimal_exponent
len is the length of the buffer (number of decimal digits)
The buffer must be large enough, i.e. >= max_digits10.
*/
template <typename FloatType>
void grisu2(char* buf, int& len, int& decimal_exponent, FloatType value)
{
    static_assert(diyfp::kPrecision >= std::numeric_limits<FloatType>::digits + 3,
                  "internal error: not enough precision");

    assert(std::isfinite(value));
    assert(value > 0);
#if 0
    const boundaries w = compute_boundaries(static_cast<double>(value));
#else
    const boundaries w = compute_boundaries(value);
#endif

    grisu2(buf, len, decimal_exponent, w.minus, w.w, w.plus);
}

/*!
@brief appends a decimal representation of e to buf
@return a pointer to the element following the exponent.
@pre -1000 < e < 1000
*/
inline char* append_exponent(char* buf, int e)
{
    assert(e > -1000);
    assert(e <  1000);

    if (e < 0)
    {
        e = -e;
        *buf++ = '-';
    }
    else
    {
        *buf++ = '+';
    }

    auto k = static_cast<std::uint32_t>(e);
    if (k < 10)
    {
        // Always print at least two digits in the exponent.
        // This is for compatibility with printf("%g").
        *buf++ = '0';
        *buf++ = static_cast<char>('0' + k);
    }
    else if (k < 100)
    {
        *buf++ = static_cast<char>('0' + k / 10);
        k %= 10;
        *buf++ = static_cast<char>('0' + k);
    }
    else
    {
        *buf++ = static_cast<char>('0' + k / 100);
        k %= 100;
        *buf++ = static_cast<char>('0' + k / 10);
        k %= 10;
        *buf++ = static_cast<char>('0' + k);
    }

    return buf;
}

/*!
@brief prettify v = buf * 10^decimal_exponent

If v is in the range [10^min_exp, 10^max_exp) it will be printed in fixed-point
notation. Otherwise it will be printed in exponential notation.

@pre min_exp < 0
@pre max_exp > 0
*/
inline char* format_buffer(char* buf, int len, int decimal_exponent,
                           int min_exp, int max_exp)
{
    assert(min_exp < 0);
    assert(max_exp > 0);

    const int k = len;
    const int n = len + decimal_exponent;

    // v = buf * 10^(n-k)
    // k is the length of the buffer (number of decimal digits)
    // n is the position of the decimal point relative to the start of the buffer.

    if (k <= n and n <= max_exp)
    {
        // digits[000]
        // len <= max_exp + 2

        std::memset(buf + k, '0', static_cast<size_t>(n - k));
        // Make it look like a floating-point number (#362, #378)
        buf[n + 0] = '.';
        buf[n + 1] = '0';
        return buf + (n + 2);
    }

    if (0 < n and n <= max_exp)
    {
        // dig.its
        // len <= max_digits10 + 1

        assert(k > n);

        std::memmove(buf + (n + 1), buf + n, static_cast<size_t>(k - n));
        buf[n] = '.';
        return buf + (k + 1);
    }

    if (min_exp < n and n <= 0)
    {
        // 0.[000]digits
        // len <= 2 + (-min_exp - 1) + max_digits10

        std::memmove(buf + (2 + -n), buf, static_cast<size_t>(k));
        buf[0] = '0';
        buf[1] = '.';
        std::memset(buf + 2, '0', static_cast<size_t>(-n));
        return buf + (2 + (-n) + k);
    }

    if (k == 1)
    {
        // dE+123
        // len <= 1 + 5

        buf += 1;
    }
    else
    {
        // d.igitsE+123
        // len <= max_digits10 + 1 + 5

        std::memmove(buf + 2, buf + 1, static_cast<size_t>(k - 1));
        buf[1] = '.';
        buf += 1 + k;
    }

    *buf++ = 'e';
    return append_exponent(buf, n - 1);
}

} // namespace dtoa_impl
#endif

class Svar;
class SvarValue;
class SvarFunction;
class SvarClass;
class SvarObject;
class SvarArray;
class SvarDict;
class SvarBuffer;
class SvarExeption;
template <typename T>
class SvarValue_;

enum value_t : std::uint8_t
{
    undefined_t,        ///< undefined value
    null_t,             ///< null value
    boolean_t,          ///< boolean value
    integer_t,          ///< number value (signed integer)
    float_t,            ///< number value (floating-point)
    string_t,           ///< string value
    array_t,            ///< array (ordered collection of values)
    object_t,           ///< object (unordered set of name/value pairs)
    dict_t,             ///< dict (unordered set of value/value pairs)
    buffer_t,           ///< memory buffer with type, shape information
    function_t,         ///< function expressions
    svarclass_t,        ///< class expressions
    property_t,         ///< class property
    exception_t,        ///< svar exception
    argument_t,         ///< svar argument
    others_t            ///< user defined types
};

class Svar{
public:
    /// The bare value
    Svar(const std::shared_ptr<SvarValue>& v)
        : _obj(v){}

    /// Undefined singleton
    Svar():Svar(Undefined()){}

    /// Wrap boolean, not singleton
    Svar(bool b);

    /// Wrap a int, uint_8, int_8, short .ext
    Svar(int i);

    /// Wrap a double or float
    Svar(double  d);

    /// Wrap a string or const char*
    Svar(const std::string& str);
    Svar(std::string&& str); // Fast construct with rvalue

    /// Accelerate array with std::move
    Svar(std::vector<Svar>&& rvec);

    /// Wrap anything with template caster for user customize, including STLs
    template <typename T>
    Svar(const T& var);
    template <typename T>
    Svar(detail::enable_if_t<!std::is_same<T,Svar>::value,T>&& var);

    /// Wrap an pointer use unique_ptr, should std::move since uncopyable
    template <typename T>
    Svar(std::unique_ptr<T>&& v);

    /// Construct from initializer to construct array or objects
    Svar(const std::initializer_list<Svar>& init);

    /// Construct a cpp_function from a vanilla function pointer
    template <typename Return, typename... Args, typename... Extra>
    Svar(Return (*f)(Args...), const Extra&... extra);

    /// Construct a cpp_function from a lambda function (possibly with internal state)
    template <typename Func, typename... Extra>
    static Svar lambda(Func &&f, const Extra&... extra);

    /// Construct a cpp_function from a class method (non-const)
    template <typename Return, typename Class, typename... arg, typename... Extra>
    Svar(Return (Class::*f)(arg...), const Extra&... extra);

    /// Construct a cpp_function from a class method (const)
    template <typename Return, typename Class, typename... arg, typename... Extra>
    Svar(Return (Class::*f)(arg...) const, const Extra&... extra);

    /// Create any other c++ type instance, T need to be a copyable type
    template <class T>
    static Svar create(const T& t);

    /// Create with std::move, so T can be uncopyable
    template <class T>
    static Svar create(T&& t);

    /// Create an Object instance
    static Svar object(const std::map<std::string,Svar>& m={}){return Svar(m);}

    /// Create an Array instance
    static Svar array(const std::vector<Svar>& vec={}){return Svar(vec);}

    /// Create a Dict instance
    static Svar dict(const std::map<Svar,Svar>& m={}){return Svar(m);}

    /// Undefined is the default value when Svar is not assigned a valid value
    /// It corrosponding to the c++ void and means no return
    static const Svar& Undefined();

    /// Null is corrosponding to the c++ nullptr
    static const Svar& Null();

    /// The global singleton inside sharedlibrary
    static Svar&       instance();

    /// Return the raw holder
    const std::shared_ptr<SvarValue>& value()const{return _obj;}

    /// Deep copy a object, non-copyable things may become Undefined
    Svar                    clone(int depth=0)const;

    /// Return the value typename
    std::string             typeName()const;

    /// Return the value typeid
    std::type_index         cpptype()const;

    /// Return the json type for faster checking
    value_t                 jsontype()const;

    /// Return the class singleton, should always exists
    SvarClass&              classObject()const;

    /// Return the item numbers when it is an array, object or dict.
    size_t                  length() const;
    size_t                  size()const{return length();}

    /// Is holding a type T value?
    template <typename T>
    bool is()const;
    bool is(const std::type_index& typeId)const;
    bool is(const std::string& typeStr)const;

    bool isUndefined()const{return jsontype()==undefined_t;}
    bool isNull()const{return jsontype()==null_t;}
    bool isFunction() const{return jsontype()==function_t;}
    bool isClass() const{return jsontype()==svarclass_t;}
    bool isException() const{return is<SvarExeption>();}
    bool isProperty() const;
    bool isObject() const;
    bool isArray()const;
    bool isDict()const;

    /// Return the reference as type T with checking.
    template <typename T>
    const T&   as()const;

    template <typename T>
    T& as();

    /// Return the reference as type T, faster but not recommend!
    template <typename T>
    T& unsafe_as();

    template <typename T>
    const T& unsafe_as()const;

    /// No cast is performed, directly return the c++ pointer, throw SvarException when failed
    template <typename T>
    detail::enable_if_t<std::is_pointer<T>::value,T>
    castAs();

    /// No cast is performed, directly return the c++ reference, throw SvarException when failed
    template <typename T>
    detail::enable_if_t<std::is_reference<T>::value,T&>
    castAs();

    /// Cast to c++ type T and return, throw SvarException when failed
    template <typename T>
    detail::enable_if_t<!std::is_reference<T>::value&&!std::is_pointer<T>::value,T>
    castAs()const;

    /// This is the same to castAs<T>()
    template <typename T>
    T get(){return castAs<T>();}

    /// Force to return the children as type T, cast is performed,
    /// otherwise the old value will be droped and set to the value "def"
    template <typename T>
    T get(const std::string& name,T def,bool parse_dot=false);

    /// Set the child "name" to "create<T>(def)"
    template <typename T>
    void set(const std::string& name,const T& def,bool parse_dot=false);

    /// Check if the item is not Undefined without dot compute
    bool exist(const Svar& id)const;

    /// Directly call __delitem__
    void erase(const Svar& id);

    /// Append item when this is an array
    void push_back(const Svar& rh);

    /// The svar iterator now just support array and object.
    /// @code
    /// for(auto v:Svar({1,2,3})) std::cout<<v<<std::endl;
    /// @endcode
    struct svar_interator{
        enum IterType{Object,Array,Others};

        svar_interator(Svar it,IterType tp);
        ~svar_interator(){delete _it;}

        Svar*  _it;
        IterType  _type;

        Svar operator *();

        svar_interator& operator++();

        bool operator==(const svar_interator& other) const;

        bool operator!=(const svar_interator& other) const
        {
            return ! operator==(other);
        }
    };

    svar_interator begin()const;
    svar_interator end()const;
    svar_interator find(const Svar& idx)const;

    /// Used to iter an object
    /// @code
    /// Svar obj={{"a":1,"b":false}};
    /// for(std::pair<std::string,Svar> it:obj)
    ///   std::cout<<it.first<<":"<<it.second<<std::endl;
    /// @endcode
    operator std::pair<std::string,Svar>()const{return castAs<std::pair<const std::string,Svar>>();}

    /// Define a function with extra properties
    template <typename... Extra>
    Svar def(const std::string& name, Svar func, const Extra&... extra);

    /// Define a lambda with extra properties
    template <typename Func, typename... Extra>
    Svar def(const std::string& name, Func func, const Extra&... extra)
    {
        return def(name,sv::Svar::lambda(func),extra...);
    }

    Svar& overload(Svar func);

    /// Call function or class with name and arguments
    template <typename... Args>
    Svar call(const std::string function_t, Args... args)const;

    /// Call this as function or class
    template <typename... Args>
    Svar operator()(Args... args)const;

    /// Import a svar module
    Svar import(std::string module_name){
        return (*this)["__builtin__"]["import"](module_name);
    }

    Svar operator -()const;              //__neg__
    Svar operator +(const Svar& rh)const;//__add__
    Svar operator -(const Svar& rh)const;//__sub__
    Svar operator *(const Svar& rh)const;//__mul__
    Svar operator /(const Svar& rh)const;//__div__
    Svar operator %(const Svar& rh)const;//__mod__
    Svar operator ^(const Svar& rh)const;//__xor__
    Svar operator |(const Svar& rh)const;//__or__
    Svar operator &(const Svar& rh)const;//__and__

    bool operator ==(const Svar& rh)const;//__eq__
    bool operator < (const Svar& rh)const;//__lt__
    bool operator !=(const Svar& rh)const{return !((*this)==rh);}//__ne__
    bool operator > (const Svar& rh)const{return !((*this)==rh||(*this)<rh);}//__gt__
    bool operator <=(const Svar& rh)const{return ((*this)<rh||(*this)==rh);}//__le__
    bool operator >=(const Svar& rh)const{return !((*this)<rh);}//__ge__
    Svar operator [](const Svar& i) const;//__getitem__
    Svar& operator[](const Svar& name);// This is not thread safe!

    template <typename T>
    detail::enable_if_t<std::is_copy_assignable<T>::value,Svar&> operator =(const T& v){
        if(is<T>()) as<T>()=v;
        else (*this)=Svar(v);
        return *this;
    }

    template <typename T>
    detail::enable_if_t<!std::is_copy_assignable<T>::value,Svar&> operator =(const T& v){
        (*this)=Svar(v);
        return *this;
    }

    /// Dump this as Json style outputs
    friend std::ostream& operator <<(std::ostream& ost,const Svar& self);
    friend std::istream& operator >>(std::istream& ist,Svar& self);

    /// Create from Json String
    static Svar parse_json(const std::string& str){
        return instance()["__builtin__"]["Json"].call("load",str);
    }

    /// Dump Json String
    std::string dump_json(const int indent = -1)const;

    template <typename T>
    T& dump(T& ost,const int indent=-1,const int current_indent=0)const;

    static int dtos(double value,char* buf);

    /// Parse the "main(int argc,char** argv)" arguments
    std::vector<std::string> parseMain(int argc, char** argv);

    /// Parse a file, builtin support json
    static Svar        loadFile(const std::string& file_path);

    /// Parse a file to update this, existed value will be overlaped
    bool parseFile(const std::string& file_path);

    /// Register default required arguments
    template <typename T>
    T arg(const std::string& name, T def, const std::string& help);

    /// Format print version, usages and arguments as string
    std::string helpInfo();

    /// Format print version, usages and arguments
    int help(){std::cout<<helpInfo();return 0;}

    /// Format print strings as table
    static std::string printTable(
            std::vector<std::pair<int, std::string>> line);

    template <typename T>
    T Arg(const std::string& name, T def, const std::string& help){return arg<T>(name,def,help);}

    std::vector<std::string> ParseMain(int argc, char** argv){return parseMain(argc,argv);}
    bool ParseFile(const std::string& file_path){return parseFile(file_path);}

    template <typename T>
    T Get(const std::string& name,T def=T()){return get<T>(name,def);}
    Svar Get(const std::string& name,Svar def=Svar()){return get(name,def);}
    int GetInt(const std::string& name,int def=0){return get<int>(name,def);}
    double GetDouble(const std::string& name,double def=0){return get<double>(name,def);}
    std::string GetString(const std::string& name,std::string def=""){return get<std::string>(name,def);}
    void* GetPointer(const std::string& name,void* def=nullptr){return get<void*>(name,def);}
    template <typename T>
    void Set(const std::string& name,const T& def){return set<T>(name,def);}
    template <typename T>
    void Set(const std::string& name,const T& def,bool overwrite){
        if(exist(name)&&!overwrite) return;
        return set<T>(name,def);
    }

    std::shared_ptr<SvarValue> _obj;
};

#ifndef DOXYGEN_IGNORE_INTERNAL
class SvarExeption: public std::exception{
public:
    SvarExeption(const Svar& wt=Svar())
        :_wt(wt){}

    virtual const char* what() const throw(){
        if(_wt.is<std::string>())
            return _wt.as<std::string>().c_str();
        else
            return "SvarException";
    }

    Svar _wt;
};

struct arg{
    arg(std::string name) : name(name) {}
    arg& operator = (sv::Svar val){ value = val; return *this;}
    std::string name;
    sv::Svar    value;
};

inline arg operator"" _a(const char* str,size_t sz){
    return arg(std::string(str,sz));
}

class SvarFunction{
public:
    SvarFunction(){}

    /// Construct a cpp_function from a vanilla function pointer
    template <typename Return, typename... Args, typename... Extra>
    SvarFunction(Return (*f)(Args...), const Extra&... extra) {
        initialize(f, f, extra...);
    }

    /// Construct a cpp_function from a lambda function (possibly with internal state)
    template <typename Func, typename... Extra>
    SvarFunction(Func &&f, const Extra&... extra) {
        initialize(std::forward<Func>(f),
                   (detail::function_signature_t<Func> *) nullptr, extra...);
    }

    /// Construct a cpp_function from a class method (non-const)
    template <typename Return, typename Class, typename... Arg, typename... Extra>
    SvarFunction(Return (Class::*f)(Arg...), const Extra&... extra) {
        initialize([f](Class *c, Arg... args) -> Return { return (c->*f)(args...); },
                   (Return (*) (Class *, Arg...)) nullptr, extra...);
    }

    /// Construct a cpp_function from a class method (const)
    template <typename Return, typename Class, typename... Arg, typename... Extra>
    SvarFunction(Return (Class::*f)(Arg...) const, const Extra&... extra) {
        initialize([f](const Class *c, Arg... args) -> Return { return (c->*f)(args...); },
                   (Return (*)(const Class *, Arg ...)) nullptr, extra...);
    }

    class ScopedStack{
    public:
        ScopedStack(std::list<const SvarFunction*>& stack,const SvarFunction* var)
            :_stack(stack){
            _stack.push_back(var);
        }
        ~ScopedStack(){_stack.pop_back();}
        std::list<const SvarFunction*>& _stack;
    };

    Svar Call(std::vector<Svar>& argv)const{
        thread_local static std::list<const SvarFunction*> stack;
        ScopedStack scoped_stack(stack,this);

        const SvarFunction* overload=this;
        std::vector<SvarExeption> catches;
        for(;true;overload=&overload->next.as<SvarFunction>()){
            if(overload->kwargs.size()){// resort args here
                std::vector<Svar> args;
                Svar kwargs;
                for(auto& a:argv){
                    if(!a.is<arg>()){
                        args.push_back(a);
                        continue;
                    }

                    const arg& a_ = a.as<arg>();
                    kwargs[a_.name] = a_.value;
                }
                int fixarg_n = overload->arg_types.size()-overload->kwargs.size()-1;
                if(args.size() < fixarg_n) continue;// not enough

                for(int i=args.size()-fixarg_n;i<overload->kwargs.size();i++)
                {
                    arg  def_a = overload->kwargs[i].as<arg>();
                    Svar val   = kwargs[def_a.name];
                    if( val.isUndefined() )
                        val = def_a.value;
                    if( val.isUndefined())
                        break;// no valid input
                    args.push_back(val);
                }
                argv = args;
            }
            if(do_argcheck&&overload->arg_types.size()!=argv.size()+1)
            {
                if(!overload->next.isFunction()) {
                    overload=nullptr;break;
                }
                continue;
            }
            try{
                return overload->_func(argv);
            }
            catch(SvarExeption e){
                catches.push_back(e);
            }
            if(!overload->next.isFunction()) {
                overload=nullptr;break;
            }
        }

        if(!overload){
            std::stringstream stream;
            stream<<(*this)<<"Failed to call method with input arguments: [";
            for(auto it=argv.begin();it!=argv.end();it++)
            {
                stream<<(it==argv.begin()?"":",")<<it->typeName();
            }
            stream<<"]\n"<<"Overload candidates:\n"<<(*this)<<std::endl;
            for(auto it:catches) stream<<it.what()<<std::endl;
            stream<<"Stack:\n";
            for(auto& l:stack) stream<<*l;
            throw SvarExeption(stream.str());
        }

        return Svar::Undefined();
    }

    template <typename... Args>
    Svar call(Args... args)const{
        std::vector<Svar> argv = {
                (Svar(std::move(args)))...
        };
        return Call(argv);
    }

    /// Special internal constructor for functors, lambda functions, methods etc.
    template <typename Func, typename Return, typename... Args, typename... Extra>
    void initialize(Func &&f, Return (*)(Args...), const Extra&... extra);

    template <typename Func, typename Return, typename... Args,size_t... Is>
    detail::enable_if_t<std::is_void<Return>::value, Svar>
    call_impl(Func&& f,Return (*)(Args...),std::vector<Svar>& args,detail::index_sequence<Is...>){
        f(args[Is].castAs<Args>()...);
        return Svar::Undefined();
    }

    template <typename Func, typename Return, typename... Args,size_t... Is>
    detail::enable_if_t<!std::is_void<Return>::value, Svar>
    call_impl(Func&& f,Return (*)(Args...),std::vector<Svar>& args,detail::index_sequence<Is...>){
        return Svar(f(args[Is].castAs<Args>()...));
    }

    void process_extra(Svar extra);

    Svar& overload(Svar func){
        Svar* dest=&next;
        while(dest->isFunction())
        {
            if(dest->as<SvarFunction>().signature()==func.as<SvarFunction>().signature())
            {
                throw SvarExeption("Overload "+func.as<SvarFunction>().signature()+" already exists.");
            }
            dest=&dest->as<SvarFunction>().next;
        }
        *dest=func;
        return *dest;
    }

    std::string signature()const;

    friend std::ostream& operator<<(std::ostream& sst,const SvarFunction& self){
        if(self.name.size())
            sst<<self.name<<"(...)\n";
        const SvarFunction* overload=&self;
        while(overload){
            sst<<"    "<<overload->signature()<<std::endl;
            if(!overload->next.isFunction()) {
                return sst;
            }
            overload=&overload->next.as<SvarFunction>();
        }
        return sst;
    }

    std::string   name,doc;
    Svar          meta,next;
    std::vector<Svar> arg_types,kwargs;

    std::function<Svar(std::vector<Svar>&)> _func;
    bool          is_method=false,is_constructor=false,do_argcheck=true;
};

class SvarClass{
public:
    class SvarProperty{
    public:
      SvarProperty(Svar fget,Svar fset,std::string name,std::string doc)
        :_fget(fget),_fset(fset),_name(name),_doc(doc){}

      friend std::ostream& operator<<(std::ostream& ost,const SvarProperty& rh)
      {
          ost<<rh._name<<"\n  "<<rh._fget.as<SvarFunction>()<<std::endl;;
          return ost;
      }

      Svar _fget,_fset;
      std::string _name,_doc;
    };

    struct dynamic_class_object
    {
        sv::Svar meta;
    };

    SvarClass(const std::string& name,
              std::type_index cpp_type=typeid(dynamic_class_object),
              std::vector<Svar> parents={});

    std::string name()const{return __name__;}
    void     setName(const std::string& nm){__name__=nm;}

    SvarClass& def(const std::string& name,const Svar& function,
                   bool isMethod=true, std::vector<Svar> extra={})
    {
        assert(function.isFunction());
        Svar* dest=&_attr[name];
        if(dest->isFunction())
            dest=&dest->as<SvarFunction>().overload(function);
        else
            *dest=function;
        dest->as<SvarFunction>().is_method=isMethod;
        dest->as<SvarFunction>().name=__name__+"."+name;
        for(Svar e:extra)
            dest->as<SvarFunction>().process_extra(e);

        if(__init__.is<void>()&&name=="__init__") {
            __init__=function;
            dest->as<SvarFunction>().is_constructor=true;
            if(_cpptype==typeid(dynamic_class_object))
                make_constructor(*dest);
        }
        if(__str__.is<void>()&&name=="__str__") __str__=function;
        if(__getitem__.is<void>()&&name=="__getitem__") __getitem__=function;
        if(__setitem__.is<void>()&&name=="__setitem__") __setitem__=function;
        return *this;
    }

    SvarClass& def_static(const std::string& name,const Svar& function, std::vector<Svar> extra={})
    {
        return def(name,function,false);
    }

    template <typename Func>
    SvarClass& def(const std::string& name,Func &&f, std::vector<Svar> extra={}){
        return def(name,Svar::lambda(f),true);
    }

    /// Construct a cpp_function from a lambda function (possibly with internal state)
    template <typename Func>
    SvarClass& def_static(const std::string& name,Func &&f, std::vector<Svar> extra={}){
        return def(name,Svar::lambda(f),false);
    }

    SvarClass& def_property(const std::string& name,
                            const Svar& fget,const Svar& fset=Svar(),
                            const std::string& doc=""){
        _attr[name]=SvarProperty(fget,fset,name,doc);
        return *this;
    }

    template <typename C, typename D>
    SvarClass& def_readwrite(const std::string& name, D C::*pm, const std::string& doc="") {
        Svar fget=SvarFunction([pm](const C &c) -> const D &{ return c.*pm; });
        Svar fset=SvarFunction([pm](C &c, const D &value) { c.*pm = value;});
        fget.as<SvarFunction>().is_method=true;
        fset.as<SvarFunction>().is_method=true;
        return def_property(name, fget, fset, doc);
    }

    template <typename C, typename D>
    SvarClass& def_readonly(const std::string& name, D C::*pm, const std::string& doc="") {
        Svar fget=SvarFunction([pm](const C &c) -> const D &{ return c.*pm; });
        fget.as<SvarFunction>().is_method=true;
        return def_property(name, fget, Svar(), doc);
    }

    template <typename Base,typename ChildType>
    SvarClass& inherit(){
        Svar base={SvarClass::instance<Base>(),
                   Svar::lambda([](ChildType* v){
                       return dynamic_cast<Base*>(v);
                   })};
        _parents.push_back(base);
        return *this;
    }

    SvarClass& inherit(std::vector<Svar> parents={}){
        _parents=parents;
        return *this;
    }

    Svar operator [](const std::string& name){
        Svar& c=_attr[name];
        if(!c.isUndefined())  return c;
        for(Svar& p:_parents)
        {
            c=p.as<SvarClass>()[name];
            if(!c.isUndefined()) return c;
        }
        return c;
    }

    template <typename... Args>
    Svar call(const Svar& inst,const std::string function, Args... args)const
    {
        std::vector<Svar> argv = {
                (Svar(std::move(args)))...
        };
        return Call(inst,function,argv);
    }

    Svar Call(const Svar& inst,const std::string function, std::vector<Svar> args)const
    {
        Svar f=_attr[function];
        if(f.isFunction())
        {
            SvarFunction& func=f.as<SvarFunction>();
            if(func.is_method){
                if(inst.isUndefined())
                    throw SvarExeption("Method should be called with self.");
                args.insert(args.begin(),inst);
                return func.Call(args);
            }
            else return func.Call(args);
        }

        std::stringstream sst;
        for(const Svar& p:_parents){
            try{
                if(p.isClass()){
                    return p.as<SvarClass>().Call(inst,function,args);
                }
                return p[0].as<SvarClass>().Call(p[1](inst),function,args);
            }
            catch(SvarExeption e){
                sst<<e.what();
                continue;
            }
        }
        throw SvarExeption("Class "+__name__+" has no function "+function+sst.str());
        return Svar::Undefined();
    }

    static std::string decodeName(const char* __mangled_name){
#ifdef __GNUC__
        int status;
        char* realname = abi::__cxa_demangle(__mangled_name, 0, 0, &status);
        std::string result(realname);
        free(realname);
#else
        std::string result(__mangled_name);
#endif
        return result;
    }

    template <typename T>
    static SvarClass* instance()
    {
        static std::shared_ptr<SvarClass> cl;
        if(cl) return cl.get();
        cl=std::make_shared<SvarClass>(decodeName(typeid(T).name()),typeid(T));
        return cl.get();
    }

    template <typename T>
    static SvarClass& Class(){
        return *instance<T>();
    }

    friend std::ostream& operator<<(std::ostream& ost,const SvarClass& rh);

    void make_constructor(sv::Svar fvar);

    std::string  __name__,__doc__;
    std::type_index _cpptype;
    Svar _attr,__init__,__str__,__getitem__,__setitem__;
    std::vector<Svar> _parents;
    value_t _json_type;
};

template <typename C>
class Class
{
public:
    Class(SvarClass& cls):_cls(cls){}

    Class():_cls(SvarClass::Class<C>()){}

    Class(const std::string& name,const std::string& doc="")
        :_cls(SvarClass::Class<C>()){
        _cls.setName(name);
        _cls.__doc__=doc;
        Svar::instance()[name]=SvarClass::instance<C>();
    }

    template <typename... Extra>
    Class& def(const std::string& name,const Svar& function, const Extra&... extra)
    {
        _cls.def(name,function,true,{extra...});
        return *this;
    }

    template <typename... Extra>
    Class& def_static(const std::string& name,const Svar& function, const Extra&... extra)
    {
        _cls.def(name,function,false,{extra...});
        return *this;
    }

    template <typename Func, typename... Extra>
    Class& def(const std::string& name,Func &&f, const Extra&... extra){
        _cls.def(name,Svar::lambda(f),true,{extra...});
        return *this;
    }

    /// Construct a cpp_function from a lambda function (possibly with internal state)
    template <typename Func, typename... Extra>
    Class& def_static(const std::string& name,Func &&f, const Extra&... extra){
        _cls.def(name,Svar::lambda(f),false,{extra...});
        return *this;
    }

    template <typename BaseType>
    Class& inherit(){
        _cls.inherit<BaseType,C>();
        return *this;
    }

    template <typename... Args>
    Class& construct(){
        return def_static("__init__",[](Args... args){
            return std::unique_ptr<C>(new C(args...));
        });
    }

    template <typename... Args>
    Class& unique_construct(){
        return def_static("__init__",[](Args... args){
            return std::unique_ptr<C>(new C(args...));
        });
    }

    template <typename... Args>
    Class& shared_construct(){
        return def_static("__init__",[](Args... args){
            return std::make_shared<C>(args...);
        });
    }

    Class& def_property(const std::string& name,
                        const Svar& fget,const Svar& fset=Svar(),
                        const std::string& doc=""){
      _cls.def_property(name,fget,fset,doc);
      return *this;
    }

    template <typename E, typename D>
    Class& def_readwrite(const std::string& name, D E::*pm, const std::string& doc="") {
        static_assert(std::is_base_of<E, C>::value, "def_readwrite() requires a class member (or base class member)");

        Svar fget=SvarFunction([pm](const C &c) -> const D &{ return c.*pm; });
        Svar fset=SvarFunction([pm](C &c, const D &value) { c.*pm = value;});
        fget.as<SvarFunction>().is_method=true;
        fset.as<SvarFunction>().is_method=true;
        return def_property(name, fget, fset, doc);
    }

    template <typename E, typename D>
    Class& def_readonly(const std::string& name, D E::*pm, const std::string& doc="") {
        static_assert(std::is_base_of<E, C>::value, "def_readonly() requires a class member (or base class member)");
        Svar fget=SvarFunction([pm](const C &c) -> const D &{ return c.*pm; });
        fget.as<SvarFunction>().is_method=true;
        return def_property(name, fget, Svar(), doc);
    }

    SvarClass& _cls;
};

class SvarBuffer
{
public:
  SvarBuffer(void *ptr, ssize_t itemsize, const std::string &format,
             std::vector<ssize_t> shape_in, std::vector<ssize_t> strides_in, Svar holder=Svar())
  : _ptr(ptr), _size(itemsize), _holder(holder), _format(format),
    shape(std::move(shape_in)), strides(std::move(strides_in)) {
      if (shape.size() != strides.size()){
        strides=shape;
        ssize_t stride=itemsize;
        for(int i=shape.size()-1;i>=0;--i)
        {
          strides[i]=stride;
          stride*=shape[i];
        }
      }

      for (size_t i : shape)
          _size *= i;
  }

  template <typename T>
  SvarBuffer(T *ptr, std::vector<ssize_t> shape_in, Svar holder=Svar(), std::vector<ssize_t> strides_in={})
    : SvarBuffer(ptr, sizeof(T), format<T>(), std::move(shape_in), std::move(strides_in), holder) { }

  SvarBuffer(const void *ptr, ssize_t size, Svar holder=Svar())
    : SvarBuffer( (char*)ptr, std::vector<ssize_t>({size}) , holder ) {}

  SvarBuffer(size_t size)
    : SvarBuffer((char*)nullptr,size,Svar::create(std::vector<char>(size))){
    _ptr = _holder.as<std::vector<char>>().data();
  }

    template<typename T=char>
    T*              ptr() const {return (T*)_ptr;}
    size_t          size() const {return _size;}
    size_t          length() const{return _size;}

    static SvarBuffer load(std::string path){
        std::ifstream in(path,std::ios::in|std::ios::binary);
        if(in.is_open()){
            std::string* file = new std::string( (std::istreambuf_iterator<char>(in)) , std::istreambuf_iterator<char>() );
            return SvarBuffer(file->data(),file->size(),std::unique_ptr<std::string>(file));
        }
        std::cout<<"Wrong path!"<<std::endl;
        return SvarBuffer(nullptr,0,Svar::Null());
    }

    bool save(std::string path){
        std::ofstream out(path,std::ios_base::binary);
        if(out.is_open()){
            out.write((const char*)_ptr,_size);
            out.close();
            return true;
        }
        return false;
    }

    /// Transformation between hex and SvarBuffer
    std::string hex()const{
        const std::string h = "0123456789ABCDEF";
        std::string ret;ret.resize(_size*2);
        for(ssize_t i=0;i<_size;i++){
            ret[i<<1]=h[((uint8_t*)_ptr)[i] >> 4];
            ret[(i<<1)+1]=h[((uint8_t*)_ptr)[i] & 0xf];
        }
        return ret;
    }

    static SvarBuffer fromHex(const std::string& h){
        size_t n = h.size()>>1;
        SvarBuffer ret(n);
        for(size_t i=0;i < n;i++){
            ((uint8_t*)(ret._ptr))[i]=strtol(h.substr(i<<1,2).c_str(),nullptr,16);
        }
        return ret;
    }

    /// Transformation between base64 and string
    std::string base64() const {
        const unsigned char * bytes_to_encode=(unsigned  char*)_ptr;
        size_t in_len=_size;
        const std::string chars  = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";;
        std::string ret;
        int i = 0;
        int j = 0;
        unsigned char char_array_3[3];
        unsigned char char_array_4[4];

        while (in_len--) {
            char_array_3[i++] = *(bytes_to_encode++);
            if (i == 3) {
                char_array_4[0] = (unsigned char) ((char_array_3[0] & 0xfc) >> 2);
                char_array_4[1] = (unsigned char) ( ( ( char_array_3[0] & 0x03 ) << 4 ) + ( ( char_array_3[1] & 0xf0 ) >> 4 ) );
                char_array_4[2] = (unsigned char) ( ( ( char_array_3[1] & 0x0f ) << 2 ) + ( ( char_array_3[2] & 0xc0 ) >> 6 ) );
                char_array_4[3] = (unsigned char) ( char_array_3[2] & 0x3f );

                for(i = 0; (i <4) ; i++)
                    ret += chars[char_array_4[i]];
                i = 0;
            }
        }

        if (i)
        {
            for(j = i; j < 3; j++)
                char_array_3[j] = '\0';

            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;

            for (j = 0; (j < i + 1); j++)
                ret += chars[char_array_4[j]];

            while((i++ < 3))
                ret += '=';

        }

        return ret;
    }

    static SvarBuffer fromBase64(const std::string& h){
        const std::string chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";;
        size_t in_len = h.size();
        size_t i = 0;
        size_t j = 0;
        int in_ = 0;
        unsigned char char_array_4[4], char_array_3[3];
        std::string* ret = new std::string;
        auto is_base64=[](unsigned char c) {
            return (isalnum(c) || (c == '+') || (c == '/'));
        };

        while (in_len-- && ( h[in_] != '=') && is_base64(h[in_])) {
          char_array_4[i++] = h[in_]; in_++;
          if (i ==4) {
            for (i = 0; i <4; i++)
              char_array_4[i] = (unsigned char) chars.find( char_array_4[i] );

            char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
            char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
            char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

            for (i = 0; (i < 3); i++)
              *ret += char_array_3[i];
            i = 0;
          }
        }

        if (i) {
          for (j = i; j <4; j++)
            char_array_4[j] = 0;

          for (j = 0; j <4; j++)
            char_array_4[j] = (unsigned char) chars.find( char_array_4[j] );

          char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
          char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
          char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

          for (j = 0; (j < i - 1); j++) *ret += char_array_3[j];
        }

        return SvarBuffer(ret->data(),ret->size(),std::unique_ptr<std::string>(ret));
    }

    /// MD5 value
    std::string md5(){
        const std::string hexs = "0123456789ABCDEF";
        uint32_t atemp=0x67452301,btemp=0xefcdab89,
                 ctemp=0x98badcfe,dtemp=0x10325476;
        const unsigned int k[]={
                0xd76aa478,0xe8c7b756,0x242070db,0xc1bdceee,
                0xf57c0faf,0x4787c62a,0xa8304613,0xfd469501,0x698098d8,
                0x8b44f7af,0xffff5bb1,0x895cd7be,0x6b901122,0xfd987193,
                0xa679438e,0x49b40821,0xf61e2562,0xc040b340,0x265e5a51,
                0xe9b6c7aa,0xd62f105d,0x02441453,0xd8a1e681,0xe7d3fbc8,
                0x21e1cde6,0xc33707d6,0xf4d50d87,0x455a14ed,0xa9e3e905,
                0xfcefa3f8,0x676f02d9,0x8d2a4c8a,0xfffa3942,0x8771f681,
                0x6d9d6122,0xfde5380c,0xa4beea44,0x4bdecfa9,0xf6bb4b60,
                0xbebfbc70,0x289b7ec6,0xeaa127fa,0xd4ef3085,0x04881d05,
                0xd9d4d039,0xe6db99e5,0x1fa27cf8,0xc4ac5665,0xf4292244,
                0x432aff97,0xab9423a7,0xfc93a039,0x655b59c3,0x8f0ccc92,
                0xffeff47d,0x85845dd1,0x6fa87e4f,0xfe2ce6e0,0xa3014314,
                0x4e0811a1,0xf7537e82,0xbd3af235,0x2ad7d2bb,0xeb86d391
        };//64
        const unsigned int s[]={7,12,17,22,7,12,17,22,7,12,17,22,7,
                12,17,22,5,9,14,20,5,9,14,20,5,9,14,20,5,9,14,20,
                4,11,16,23,4,11,16,23,4,11,16,23,4,11,16,23,6,10,
                15,21,6,10,15,21,6,10,15,21,6,10,15,21};

        std::function<uint32_t(uint32_t,uint32_t)> shift = [](uint32_t x,uint32_t n){
            return (x<<n)|(x>>(32-n));
        };
        std::vector<std::function<uint32_t(uint32_t,uint32_t,uint32_t)> > funcs ={
                [](uint32_t x,uint32_t y,uint32_t z){return (x & y)| (~x & z);},
                [](uint32_t x,uint32_t y,uint32_t z){return (x & z)| (y & ~z);},
                [](uint32_t x,uint32_t y,uint32_t z){return (x ^ y ^ z);},
                [](uint32_t x,uint32_t y,uint32_t z){return (y ^ (x | ~z));}};
        std::vector<std::function<uint8_t(uint8_t)> > func_g ={
                [](uint8_t i){return i;},
                [](uint8_t i){return (5*i+1)%16;},
                [](uint8_t i){return (3*i+5)%16;},
                [](uint8_t i){return (7*i)%16;}};

        std::function<void(std::vector<uint32_t>&,int) > mainloop = [&](std::vector<uint32_t>& v,int step) -> void{
            unsigned int f,g;
            unsigned int a=atemp;
            unsigned int b=btemp;
            unsigned int c=ctemp;
            unsigned int d=dtemp;
            for(uint8_t i=0;i<64;i++){
                f=funcs[i>>4](b,c,d);
                g=func_g[i>>4](i);
                unsigned int tmp=d;
                d=c;
                c=b;
                b=b+shift((a+f+k[i]+v[step*16+g]),s[i]);
                a=tmp;
            }
            atemp=a+atemp;
            btemp=b+btemp;
            ctemp=c+ctemp;
            dtemp=d+dtemp;
        };
        std::function<std::string(int)> int2hexstr = [&](int i) -> std::string {
            std::string s;
            s.resize(8);
            for(int j=0;j<4;j++){
                uint8_t b = (i>>(j*8))%(1<<8);
                s[2*j] = hexs[b>>4];
                s[2*j+1] = hexs[b%16];
            }
            return s;
        };


        //fill data
        int total_groups = (_size+8)/64+1;
        int total_ints = total_groups*16;
        std::vector<uint32_t> vec(total_ints,0);
        for(size_t i = 0; i < _size; i++)
            vec[i>>2] |= (((const char*)_ptr)[i]) << ((i%4)*8);
        vec[_size>>2] |= 0x80 << (_size%4*8);
        uint64_t size = _size*8;
        vec[total_ints-2] = size & UINT_LEAST32_MAX;
        vec[total_ints-1] = size>>32;

        //loop calculate
        for(int i = 0; i < total_groups; i++){
            mainloop(vec,i);
        }
        return int2hexstr(atemp)\
        .append(int2hexstr(btemp))\
        .append(int2hexstr(ctemp))\
        .append(int2hexstr(dtemp));
    }

    friend std::ostream& operator<<(std::ostream& ost,const SvarBuffer& b){
        ost<<b.hex();
        return ost;
    }

    SvarBuffer clone(){
        SvarBuffer buf(_size);
        memcpy((void*)buf._ptr,_ptr,_size);
        buf._format   = _format;
        buf.shape     = shape;
        buf.strides   = strides;
        return buf;
    }

    inline static constexpr int log2(size_t n, int k = 0) { return (n <= 1) ? k : log2(n >> 1, k + 1); }

    template <typename T>
    static detail::enable_if_t<std::is_arithmetic<T>::value,std::string> format(){
      int index = std::is_same<T, bool>::value ? 0 : 1 + (
          std::is_integral<T>::value ? log2(sizeof(T))*2 + std::is_unsigned<T>::value : 8 + (
          std::is_same<T, double>::value ? 1 : std::is_same<T, long double>::value ? 2 : 0));
      return std::string(1,"?bBhHiIqQfdg"[index]);
    }

    int itemsize(){
      static int lut[256]={0};
      lut['b']=lut['B']=1;
      lut['h']=lut['H']=2;
      lut['i']=lut['I']=lut['f']=4;
      lut['d']=lut['g']=lut['q']=lut['Q']=8;
      return lut[_format.front()];
    }

    void*   _ptr;
    ssize_t _size;
    Svar    _holder;
    std::string _format;
    std::vector<ssize_t> shape,strides;
};

class SvarValue{
public:
    SvarValue(){}
    virtual ~SvarValue(){}
    typedef std::type_index TypeID;
    virtual const void*     as(const TypeID& tp)const{if(tp==typeid(void)) return this;else return nullptr;}
    virtual SvarClass*     classObject()const;
    virtual size_t          length() const {return 0;}
    virtual Svar            clone(int depth=0)const{return Svar();}
};

template <typename T>
class SvarValue_: public SvarValue{
public:
    explicit SvarValue_(const T& v):_var(v){}
    explicit SvarValue_(T&& v):_var(std::move(v)){}

    virtual const void*     as(const TypeID& tp)const{if(tp==typeid(T)) return &_var;else return nullptr;}
    virtual SvarClass*     classObject()const{return SvarClass::instance<T>();}
    virtual Svar            clone(int depth=0)const{return _var;}
//protected:// FIXME: this should not accessed by outside
    T _var;
};

template <>
class SvarValue_<Svar>: public SvarValue{
private:
    SvarValue_(const Svar& v){}
};

template <typename T>
class SvarValue_<std::shared_ptr<T>>: public SvarValue{
public:
    explicit SvarValue_(const std::shared_ptr<T>& v):_var(v){}
    explicit SvarValue_(std::shared_ptr<T>&& v):_var(std::move(v)){}

    virtual TypeID          cpptype()const{return typeid(T);}
    virtual SvarClass*     classObject()const{return SvarClass::instance<T>();}
    virtual Svar            clone(int depth=0)const{return _var;}
    virtual const void*     as(const TypeID& tp)const{
        if(tp==typeid(T)) return _var.get();
        else if(tp==typeid(std::shared_ptr<T>)) return &_var;
        else return nullptr;
    }

    std::shared_ptr<T> _var;
};

template <typename T>
class SvarValue_<std::unique_ptr<T>>: public SvarValue{
public:
    explicit SvarValue_(std::unique_ptr<T>&& v):_var(std::move(v)){}

    virtual SvarClass*     classObject()const{return SvarClass::instance<T>();}
    virtual const void*     as(const TypeID& tp)const{
        if(tp==typeid(T)) return _var.get();
        else if(tp==typeid(std::unique_ptr<T>)) return &_var;
        else return nullptr;
    }

    std::unique_ptr<T> _var;
};

template <typename T>
class SvarValue_<T*>: public SvarValue{
public:
    explicit SvarValue_(T* v):_var(v){}

    virtual SvarClass*     classObject()const{return SvarClass::instance<T>();}
    virtual const void*     as(const TypeID& tp)const{
        if(tp==typeid(T)) return _var;
        else if(tp==typeid(T*)) return &_var;
        else return nullptr;
    }

    T* _var;
};

class SvarObject : public SvarValue_<std::unordered_map<std::string,Svar> >{
public:
    SvarObject(const std::map<std::string,Svar>& m)
        : SvarValue_<std::unordered_map<std::string,Svar>>({}){
        _var.insert(m.begin(),m.end());
    }

    SvarObject(std::unordered_map<std::string,Svar>&& m)
        : SvarValue_<std::unordered_map<std::string,Svar>>(m){
    }

    SvarObject(const std::unordered_map<std::string,Svar>& m)
        : SvarValue_<std::unordered_map<std::string,Svar>>(m){
    }

    virtual const void*     as(const TypeID& tp)const{
        if(tp==typeid(SvarObject)) return this;
        else if(tp==typeid(std::unordered_map<std::string,Svar>)) return &_var;
        else return nullptr;
    }

    virtual size_t          length() const {return _var.size();}
    virtual SvarClass*     classObject()const{
        if(_class) return _class;
        return SvarClass::instance<SvarObject>();
    }

    virtual Svar            clone(int depth=0)const{
        std::unique_lock<std::mutex> lock(_mutex);
        if(depth<=0)
            return _var;
        auto var=_var;
        for(auto it=var.begin();it!=var.end();it++){
            it->second=it->second.clone(depth-1);
        }
        return var;
    }

    Svar operator[](const std::string &key)const {//get
        std::unique_lock<std::mutex> lock(_mutex);
        auto it=_var.find(key);
        if(it==_var.end()){
            return Svar::Undefined();
        }
        return it->second;
    }

    void set(const std::string &key,const Svar& value){
        std::unique_lock<std::mutex> lock(_mutex);
        auto it=_var.find(key);
        if(it==_var.end()){
            _var.insert(std::make_pair(key,value));
        }
        else it->second=value;
    }

    mutable std::mutex _mutex;
    SvarClass* _class=nullptr;
};

class SvarArray : public SvarValue_<std::vector<Svar> >{
public:
    SvarArray(const std::vector<Svar>& v)
        :SvarValue_<std::vector<Svar>>(v){}

    SvarArray(std::vector<Svar>&& v)
        :SvarValue_<std::vector<Svar>>(std::move(v)){}

    virtual SvarClass*     classObject()const{return SvarClass::instance<SvarArray>();}
    virtual size_t          length() const {return _var.size();}

    virtual const void*     as(const TypeID& tp)const{
        if(tp==typeid(SvarArray)) return this;
        else if(tp==typeid(std::vector<Svar>)) return &_var;
        else return nullptr;
    }

    virtual const Svar& operator[](size_t i) {
        std::unique_lock<std::mutex> lock(_mutex);
        if(i<_var.size()) return _var[i];
        return Svar::Undefined();
    }

    virtual Svar            clone(int depth=0)const{
        std::unique_lock<std::mutex> lock(_mutex);
        if(depth<=0)
            return _var;
        std::vector<Svar> var=_var;
        for(auto& it:var){
            it=it.clone(depth-1);
        }
        return var;
    }

    mutable std::mutex _mutex;
};

class SvarDict : public SvarValue_<std::map<Svar,Svar> >{
public:
    SvarDict(const std::map<Svar,Svar>& dict)
        :SvarValue_<std::map<Svar,Svar> >(dict){}

    virtual const void*     as(const TypeID& tp)const{
        if(tp==typeid(SvarDict)) return this;
        else if(tp==typeid(std::map<Svar,Svar>)) return &_var;
        else return nullptr;
    }

    virtual SvarClass*     classObject()const{return SvarClass::instance<SvarDict>();}

    virtual size_t          length() const {return _var.size();}
    virtual Svar operator[](const Svar& i) {
        std::unique_lock<std::mutex> lock(_mutex);
        auto it=_var.find(i);
        if(it!=_var.end()) return it->second;
        return Svar::Undefined();
    }
    void erase(const Svar& id){
        std::unique_lock<std::mutex> lock(_mutex);
        _var.erase(id);
    }
    mutable std::mutex _mutex;
};

inline SvarClass*     SvarValue::classObject()const{return SvarClass::instance<void>();}

/// Special internal constructor for functors, lambda functions, methods etc.
template <typename Func, typename Return, typename... Args, typename... Extra>
void SvarFunction::initialize(Func &&f, Return (*)(Args...), const Extra&... extra)
{
    // FIXME: this template used over 50% percent of binary size and slow down compile speed
    this->arg_types={SvarClass::instance<Return>(),SvarClass::instance<Args>()...};// about 15%
    std::vector<Svar> extras={extra...};
    for(Svar e:extras) process_extra(e);

    _func=[this,f](std::vector<Svar>& args)->Svar{ // about 19%
        using indices = detail::make_index_sequence<sizeof...(Args)>;
        return call_impl(f,(Return (*) (Args...)) nullptr,args,indices{});// about 19%
    };
}

inline std::string SvarFunction::signature()const{
    std::stringstream sst;
    sst << name << "(";
    int fixarg_n = arg_types.size()-kwargs.size();
    for(size_t i = 1;i < fixarg_n;i++){
        if(is_method && i==1)
            sst<< "self: ";
        else
            sst<< "arg" << i-1 << ": ";
        sst<< arg_types[i].as<SvarClass>().name()
           << (i+1==arg_types.size()?"":", ");
    }

    for(size_t i=fixarg_n;i<arg_types.size();i++){
        arg a = kwargs[i-fixarg_n].as<arg>();
        sst << a.name << ": ";
        if(a.value.isUndefined())
            sst << arg_types[i].as<SvarClass>().name();
        else
            sst << a.value;
        sst << (i+1==arg_types.size()?"":", ");
    }
    sst << ") -> ";
    sst << arg_types[0].as<SvarClass>().name();
    sst << "\n" << doc;
    return sst.str();
}

inline void SvarFunction::process_extra(Svar extra)
{
    if(extra.is<std::string>())
        doc = extra.as<std::string>();
    else if(extra.is<arg>())
        kwargs.push_back(extra);
}

inline SvarClass::SvarClass(const std::string& name,std::type_index cpp_type,
          std::vector<Svar> parents)
    : __name__(name),_cpptype(cpp_type),
      _attr(Svar::object()),_parents(parents),_json_type(value_t::others_t){

    std::map<std::type_index,value_t> lut =
    {{typeid(void),undefined_t},
     {typeid(nullptr),null_t},
     {typeid(bool),boolean_t},
     {typeid(int),value_t::integer_t},
     {typeid(double),value_t::float_t},
     {typeid(std::string),value_t::string_t},
     {typeid(SvarArray),value_t::array_t},
     {typeid(SvarObject),value_t::object_t},
     {typeid(SvarDict),value_t::dict_t},
     {typeid(SvarBuffer),value_t::buffer_t},
     {typeid(SvarFunction),value_t::function_t},
     {typeid(SvarClass),value_t::svarclass_t},
     {typeid(SvarProperty),value_t::property_t},
     {typeid(arg),value_t::argument_t},
     {typeid(SvarExeption),value_t::exception_t}
    };
    auto it=lut.find(cpp_type);
    if(it!=lut.end()) _json_type=it->second;
}

inline void SvarClass::make_constructor(sv::Svar fvar){
    SvarFunction& f=fvar.as<SvarFunction>();
    auto func=f._func;
    f._func=[this,func](std::vector<Svar>& args)->Svar{
        sv::Svar self=sv::Svar::object();
        self.as<SvarObject>()._class=this;
        std::vector<Svar> args1={self};
        args1.insert(args1.end(),args.begin(),args.end());
        func(args1);
        return self;
    };
    f.arg_types.erase(f.arg_types.begin()+1);
}

inline Svar::Svar(const std::initializer_list<Svar>& init)
    :Svar()
{
    bool is_an_object = std::all_of(init.begin(), init.end(),
                                    [](const Svar& ele)
    {
        return ele.isArray() && ele.length() == 2 && ele[0].is<std::string>();
    });

    if(is_an_object){
        std::map<std::string,Svar> obj;
        for(const Svar& p:init) obj.insert(std::make_pair(p[0].as<std::string>(),p[1]));
        *this=Svar(obj);
        return;
    }

    *this=Svar(std::vector<Svar>(init));
}

template <typename Return, typename... Args, typename... Extra>
Svar::Svar(Return (*f)(Args...), const Extra&... extra)
    :Svar(SvarFunction(f,extra...)){}

/// Construct a cpp_function from a lambda function (possibly with internal state)
template <typename Func, typename... Extra>
Svar Svar::lambda(Func &&f, const Extra&... extra)
{
    return SvarFunction(f,extra...);
}

/// Construct a cpp_function from a class method (non-const)
template <typename Return, typename Class, typename... Args, typename... Extra>
Svar::Svar(Return (Class::*f)(Args...), const Extra&... extra)
    :Svar(SvarFunction(f,extra...)){}

/// Construct a cpp_function from a class method (const)
template <typename Return, typename Class, typename... Args, typename... Extra>
Svar::Svar(Return (Class::*f)(Args...) const, const Extra&... extra)
    :Svar(SvarFunction(f,extra...)){}


template <class T>
inline Svar Svar::create(const T & t)
{
    static_assert(!std::is_same<T,Svar>::value,"This should not happen.");
    return (std::shared_ptr<SvarValue>)std::make_shared<SvarValue_<T>>(t);
}

template <class T>
inline Svar Svar::create(T && t)
{
    static_assert(!std::is_same<T,Svar>::value,"This should not happen.");
    return (std::shared_ptr<SvarValue>)std::make_shared<SvarValue_<typename std::remove_reference<T>::type>>(std::move(t));
}

inline Svar::Svar(const std::string& m)
    :_obj(std::make_shared<SvarValue_<std::string>>(m)){}

inline Svar::Svar(std::string&& m)
    :_obj(std::make_shared<SvarValue_<std::string>>(std::move(m))){}

inline Svar::Svar(bool m)
    :_obj(std::make_shared<SvarValue_<bool>>(m)){}

inline Svar::Svar(int m)
    :_obj(std::make_shared<SvarValue_<int>>(m)){}

inline Svar::Svar(double m)
    :_obj(std::make_shared<SvarValue_<double>>(m)){}

inline Svar::Svar(std::vector<Svar>&& rvec)
    :_obj(std::make_shared<SvarArray>(rvec)){}

template <typename T>
Svar::Svar(std::unique_ptr<T>&& v)
    : _obj(new SvarValue_<std::unique_ptr<T>>(std::move(v))){}

template <>
inline Svar::Svar(const std::shared_ptr<SvarValue>& v)
    : _obj(v?v:Null().value()){}

inline Svar operator"" _svar(const char* str,size_t sz){
    return Svar::instance()["__builtin__"]["Json"].call("load",std::string(str,sz));
}

template <typename T>
inline bool Svar::is()const{return _obj->as(typeid(T))!=nullptr;}
inline bool Svar::is(const std::type_index& typeId)const{return _obj->as(typeId)!=nullptr;}
inline bool Svar::is(const std::string& typeStr)const{return classObject().name()==typeStr;}

template <>
inline bool Svar::is<Svar>()const{return true;}

inline bool Svar::isProperty() const{
    return jsontype()==sv::property_t;
}

inline bool Svar::isObject() const{
    return std::dynamic_pointer_cast<SvarObject>(_obj)!=nullptr;
}

inline bool Svar::isArray()const{
    return std::dynamic_pointer_cast<SvarArray>(_obj)!=nullptr;
}

inline bool Svar::isDict()const{
    return std::dynamic_pointer_cast<SvarDict>(_obj)!=nullptr;
}

template <typename T>
inline const T& Svar::as()const{
    T* ptr=(T*)_obj->as(typeid(T));
    if(!ptr)
        throw SvarExeption("Can not treat "+typeName()+" as "+SvarClass::Class<T>().name());
    return *ptr;
}

template <>
inline const Svar& Svar::as<Svar>()const{
    return *this;
}

template <typename T>
T& Svar::as(){
    T* ptr=(T*)_obj->as(typeid(T));
    if(!ptr)
        throw SvarExeption("Can not treat "+typeName()+" as "+SvarClass::Class<T>().name());
    return *ptr;
}

template <typename T>
T& Svar::unsafe_as()
{
    return ((SvarValue_<T>*)_obj.get())->_var;
}

template <typename T>
const T& Svar::unsafe_as()const
{
    return ((SvarValue_<T>*)_obj.get())->_var;
}

template <>
inline Svar& Svar::as<Svar>(){
    return *this;
}

template <typename T>
class caster{
public:
    static Svar from(const Svar& var){
        if(var.is<T>()) return var;

        SvarClass& srcClass=var.classObject();
        Svar cvt=srcClass._attr["__"+SvarClass::Class<T>().name()+"__"];
        if(cvt.isFunction()){
            Svar ret=cvt(var);
            if(ret.is<T>()) return ret;
        }

        SvarClass& destClass=SvarClass::Class<T>();
        if(destClass.__init__.isFunction()){
            Svar ret=destClass.__init__(var);
            if(ret.is<T>()) return ret;
        }

        return Svar::Undefined();
    }

    template <typename T1>
    static detail::enable_if_t<detail::is_callable<T1>::value,Svar> to(const T1& func)
    {
        return SvarFunction(func);
    }

    template <typename T1>
    static detail::enable_if_t<!detail::is_callable<T1>::value,Svar> to(const T1& var){
        return Svar::create(var);
    }

    template <typename T1>
    static detail::enable_if_t<!detail::is_callable<T1>::value,Svar> to(T1&& var){
        return Svar::create(std::move(var));
    }
};

template <typename T>
Svar::Svar(const T& var):Svar(caster<T>::to(var))
{}

template <typename T>
Svar::Svar(detail::enable_if_t<!std::is_same<T,Svar>::value,T>&& var):Svar(caster<T>::to(std::move(var)))
{}

template <typename T>
detail::enable_if_t<!std::is_reference<T>::value&&!std::is_pointer<T>::value,T>
Svar::castAs()const
{
    T* ptr=(T*)_obj->as(typeid(T));
    if(ptr) return *ptr;
    Svar ret=caster<T>::from(*this);
    if(!ret.template is<T>())
        throw SvarExeption("Unable cast "+typeName()+" to "+SvarClass::Class<T>().name());
    return ret.as<T>();
}

template <typename T>
detail::enable_if_t<std::is_reference<T>::value,T&>
Svar::castAs(){
    typedef typename std::remove_const<typename std::remove_reference<T>::type>::type VarType;
    if(!is<VarType>())
        throw SvarExeption("Unable cast "+typeName()+" to "+SvarClass::Class<T>().name());

    return as<VarType>();
}

template <typename T>
detail::enable_if_t<std::is_pointer<T>::value,T>
Svar::castAs(){
    typedef typename std::remove_const<typename std::remove_pointer<T>::type>::type* rcptr;
    typedef typename std::remove_pointer<T>::type rpt;

    // T* -> T*
    const void* ptr=_obj->as(typeid(T));
    if(ptr) return *(T*)ptr;

    // T -> T*
    ptr=_obj->as(typeid(rpt));
    if(ptr) return (rpt*)ptr;

    // T* -> T const*
    ptr=_obj->as(typeid(rcptr));
    if(ptr) return *(rcptr*)ptr;

    // nullptr
    if(isNull()) return (T)nullptr;

    auto ret=caster<T>::from(*this);
    if(!ret.template is<T>())
        throw SvarExeption("Unable cast "+typeName()+" to "+SvarClass::Class<T>().name());
    return ret.template as<T>();// let compiler happy
}

template <>
inline Svar Svar::castAs<Svar>()const{
    return *this;
}

inline Svar Svar::clone(int depth)const{return _obj->clone(depth);}

inline std::string Svar::typeName()const{
    return classObject().name();
}

inline SvarValue::TypeID Svar::cpptype()const{
    return classObject()._cpptype;
}

inline value_t           Svar::jsontype()const
{
    return classObject()._json_type;
}

inline SvarClass&       Svar::classObject()const{
    return *_obj->classObject();
}

inline size_t            Svar::length() const{
    return _obj->length();
}

inline Svar Svar::operator[](const Svar& i) const{
    const SvarClass& cl=classObject();
    if(cl.__getitem__.isFunction()){
        return cl.__getitem__((*this),i);
    }
    if(!i.is<std::string>()) return Undefined();
    Svar property=cl._attr[i.as<std::string>()];
    if(property.isProperty()){
        return property.as<SvarClass::SvarProperty>()._fget(*this);
    }
    else{
        throw SvarExeption(typeName()+": Operator [] called without property "+i.as<std::string>());
    }
    return Undefined();
}

inline Svar& Svar::operator[](const Svar& name){
    if(isUndefined()) {
        *this=object();
    }

    if(isObject()){
        SvarObject& obj=as<SvarObject>();
        std::string nameStr=name.as<std::string>();
        std::unique_lock<std::mutex> lock(obj._mutex);
        auto it=obj._var.find(nameStr);
        if(it!=obj._var.end())
            return it->second;
        auto ret=obj._var.insert(std::make_pair(nameStr,Svar()));
        return ret.first->second;
    }
    else if(isArray())
        return as<SvarArray>()._var[name.castAs<int>()];
    else if(isDict())
        return as<SvarDict>()._var[name];
    throw SvarExeption(typeName()+": Operator [] can't be used as a lvalue.");
    return *this;
}

template <typename T>
T Svar::get(const std::string& name,T def,bool parse_dot){
    if(parse_dot){
        auto idx = name.find_first_of(".");
        if (idx != std::string::npos) {
            return (*this)[name.substr(0, idx)].get(name.substr(idx + 1), def,parse_dot);
        }
    }

    Svar var;
    if(isObject())
    {
        var=as<SvarObject>()[name];
        if(var.is<T>()) return var.as<T>();
    }
    else if(isUndefined())
        *this=object();// force to be an object
    else {
        const SvarClass& cl=classObject();
        if(cl.__getitem__.isFunction()){
            Svar ret=cl.__getitem__((*this),name);
            return ret.as<T>();
        }
        Svar property=cl._attr[name];
        if(property.isProperty()){
            Svar ret=property.as<SvarClass::SvarProperty>()._fget(*this);
            return ret.as<T>();
        }
        else{
            throw SvarExeption(typeName()+": get called without property "+name);
        }
    }

    if(!var.isUndefined()){
        Svar casted=caster<T>::from(var);
        if(casted.is<T>()){
            var=casted;
        }
    }
    else
        var=def;
    set(name,var);// value type changed

    return var.as<T>();
}

template <typename T>
inline void Svar::set(const std::string& name,const T& def,bool parse_dot){
    if(parse_dot){
        auto idx = name.find(".");
        if (idx != std::string::npos) {
            return (*this)[name.substr(0, idx)].set(name.substr(idx + 1), def, parse_dot);
        }
    }
    if(isUndefined()){
        *this=object({{name,def}});
        return;
    }
    if(isObject()){
        Svar var=as<SvarObject>()[name];
        if(!std::is_same<T,Svar>::value&&var.is<T>())
            var.as<T>()=def;
        else
            as<SvarObject>().set(name,def);
        return;
    }
    const SvarClass& cl=classObject();
    if(cl.__setitem__.isFunction()){
        cl.__setitem__((*this),name,def);
        return;
    }
    Svar property=cl._attr[name];
    if(!property.isProperty())
        throw SvarExeption(typeName()+": set called without property "+name);

    Svar fset=property.as<SvarClass::SvarProperty>()._fset;
    if(!fset.isFunction()) throw SvarExeption(typeName()+": property "+name+" is readonly.");
    fset(*this,def);
}

inline bool Svar::exist(const Svar& id)const
{
    return !(*this)[id].isUndefined();
}

inline void Svar::erase(const Svar& id)
{
    call("__delitem__",id);
}

inline void Svar::push_back(const Svar& rh)
{
    SvarArray& self=as<SvarArray>();
    std::unique_lock<std::mutex> lock(self._mutex);
    self._var.push_back(rh);
}

template <typename... Extra>
Svar Svar::def(const std::string& name, Svar func, const Extra&... extra){
    std::vector<Svar> extras={extra...};
    for(Svar e:extras)
        func.as<SvarFunction>().process_extra(e);
    (*this)[name] = func;
    return (*this);
}

inline Svar& Svar::overload(Svar func){
    return as<SvarFunction>().overload(func);
}

template <typename... Args>
Svar Svar::call(const std::string function, Args... args)const
{
    // call as static methods without check
    if(isClass()) return as<SvarClass>().call(Svar(),function,args...);
    return classObject().call(*this,function,args...);
}

template <typename... Args>
Svar Svar::operator()(Args... args)const{
    if(isFunction())
        return as<SvarFunction>().call(args...);
    else if(isClass()){
        const SvarClass& cls=as<SvarClass>();
        if(!cls.__init__.isFunction())
            throw SvarExeption("Class "+cls.__name__+" does not have __init__ function.");
        return cls.__init__(args...);
    }
    throw SvarExeption(typeName()+" can't be called as a function or constructor.");
    return Undefined();
}

struct pagedostream{
    pagedostream():page_size(4096){
        pages.push_back(std::string());
        pages.back().reserve(page_size);
    }

    pagedostream& operator <<(char c){
        if(pages.back().size()>=page_size) {
            pages.push_back(std::string());
            pages.back().reserve(page_size);
        }
        pages.back().push_back(c);
        return *this;
    }

    pagedostream& operator <<(const std::string& c){
        write(c.data(),c.size());
        return *this;
    }

    pagedostream& write(const char* s,size_t length){
        std::string& str=pages.back();
        if(length+str.size()>page_size){
            pages.push_back(std::string(s,length));
            pages.back().reserve(page_size);
            return *this;
        }

        str.append(s,s+length);
        return *this;
    }

    std::string str(){
        int size=0;
        for(auto& it:pages)
            size+=it.size();
        std::string ret(size,0);
        char* c=(char*)ret.data();
        for(auto& it:pages)
        {
            memcpy(c,it.data(),it.size());
            c+=it.size();
        }
        return ret;
    }

    int                    page_size;
    bool         reverse;
    std::list<std::string> pages;
};

template <typename T>
T& Svar::dump(T& o,const int indent_step,const int current_indent)const
{
    static auto dump_str=[](const std::string& value)->std::string{
        std::string out;
        out.reserve(value.size()*2);
        out+='"';
        for (size_t i = 0; i < value.length(); i++) {
            const char ch = value[i];
            uint8_t uch=ch;
            switch (uch) {
            case '\\': out+= "\\\\";break;
            case '"':  out+= "\\\"";break;
            case '\b': out+= "\\b";break;
            case '\f': out+= "\\f";break;
            case '\n': out+= "\\n";break;
            case '\r': out+= "\\r";break;
            case '\t': out+= "\\t";break;
            case 0xe2:
                if (static_cast<uint8_t>(value[i+1]) == 0x80
                        && static_cast<uint8_t>(value[i+2]) == 0xa8) {
                    out+= "\\u2028";
                    i += 2;
                } else if (static_cast<uint8_t>(value[i+1]) == 0x80
                           && static_cast<uint8_t>(value[i+2]) == 0xa9) {
                    out+= "\\u2029";
                    i += 2;
                }else {
                    out.push_back(ch);
                }
                break;
            default:
                if (uch <= 0x1f) {
                    char buf[8];
                    snprintf(buf, sizeof buf, "\\u%04x", ch);
                    out.append(buf,buf+8);
                } else
                    out.push_back(ch);
                break;
            }
        }
        out+='"';
        return out;
    };
    switch(classObject()._json_type)
    {
    case value_t::null_t:
        return o<<"null";
    case value_t::boolean_t:
        return o<<(unsafe_as<bool>()? "true":"false");
    case value_t::float_t:{
        char buf[64];
        return o.write(buf,dtos(unsafe_as<double>(),buf));
    }
    case value_t::integer_t:
        return o<<std::to_string(unsafe_as<int>());
    case value_t::string_t:
        return o<<dump_str(unsafe_as<std::string>());
    case value_t::array_t:
    {
        const std::vector<Svar>& vec=unsafe_as<std::vector<Svar>>();
        const auto N = vec.size();
        if(N==0)
            return o<<"[]";
        o<<'[';
        std::string indent_front,ident_back("]");
        int new_indent=current_indent+indent_step;
        if(indent_step>=0){
            indent_front = std::string(new_indent+1,' ');
            indent_front[0]='\n';
            ident_back = std::string(current_indent+2,' ');
            ident_back[0]='\n'; ident_back.back()=']';
        }
        for (auto it=vec.begin();it != vec.end();++it)
        {
            if(indent_step>=0)
                o << indent_front;
            it->dump(o,indent_step,new_indent);
            if(it==vec.end()-1)
                o<<ident_back;
            else
                o<<',';
        }
        return o;
    }
    case value_t::object_t:
    {
        const auto& obj=unsafe_as<std::unordered_map<std::string,Svar>>();
        const auto N = obj.size();
        if(N==0) return o<<"{}";
        o<<'{';
        auto it=obj.begin();
        int new_indent=current_indent+indent_step;
        std::string indent_front,ident_back("}");
        if(indent_step>=0){
            indent_front = std::string(new_indent+1,' ');
            indent_front[0]='\n';
            ident_back = std::string(current_indent+2,' ');
            ident_back[0]='\n';ident_back.back()='}';
        }
        for(size_t i=1;i<=N;++i,++it)
        {
            if(indent_step>=0)
                o<<indent_front;
            o<<dump_str(it->first);
            o<<": ";
            it->second.dump(o, indent_step, new_indent);
            if(i==N)
                o << ident_back;
            else
                o << ',';
        }
        return o;
    }
    default:
    {
        auto& cls=classObject();
        if(cls.__str__.isFunction()){
            Svar a = cls.__str__(*this);
            return o<<a.as<std::string>();
        }
        std::stringstream sst;
        sst<<value().get();
        return o<<"<"<<typeName()<<" at "<<sst.str()<<">";
    }
    }
}

inline std::string Svar::dump_json(const int indent)const
{
    pagedostream sst;
    dump(sst,indent);
    return sst.str();
}

inline std::vector<std::string> Svar::parseMain(int argc, char** argv) {
  using namespace std;
    auto getFileName=[](const std::string& path) ->std::string {
        auto idx = std::string::npos;
        if ((idx = path.find_last_of('/')) == std::string::npos)
            idx = path.find_last_of('\\');
        if (idx != std::string::npos)
            return path.substr(idx + 1);
        else
            return path;
    };
  // save main cmd things
  set("argc",argc);
  set("argv",argv);
  set("__name__",getFileName(argv[0]));
  auto setjson=[this](std::string name,std::string json){
      try{
          Svar v=parse_json(json);
          if(!v.isUndefined()) set(name,v,true);
          else set(name,json,true);
      }
      catch (SvarExeption& e){
          set(name,json,true);
      }
  };
  auto setvar=[this,setjson](std::string s)->bool{
      // Execution failed. Maybe its an assignment.
      std::string::size_type n = s.find("=");

      if (n != std::string::npos) {
        std::string var = s.substr(0, n);
        std::string val = s.substr(n + 1);

        // Strip whitespace from around var;
        std::string::size_type s = 0, e = var.length() - 1;
        if ('?' == var[e]) {
          e--;
        }
        for (; std::isspace(var[s]) && s < var.length(); s++) {
        }
        if (s == var.length())  // All whitespace before the `='?
          return false;
        for (; std::isspace(var[e]); e--) {
        }
        if (e >= s) {
          var = var.substr(s, e - s + 1);

          // Strip whitespace from around val;
          s = 0, e = val.length() - 1;
          for (; std::isspace(val[s]) && s < val.length(); s++) {
          }
          if (s < val.length()) {
            for (; std::isspace(val[e]); e--) {
            }
            val = val.substr(s, e - s + 1);
          } else
            val = "";

          setjson(var, val);
          return true;
        }
      }
      return false;
  };

  // parse main cmd
  std::vector<std::string> unParsed;
  int beginIdx = 1;
  for (int i = beginIdx; i < argc; i++) {
    string str = argv[i];
    bool foundPrefix = false;
    size_t j = 0;
    for (; j < 2 && j < str.size() && str.at(j) == '-'; j++) foundPrefix = true;

    if (!foundPrefix) {
      if (!setvar(str)) unParsed.push_back(str);
      continue;
    }

    str = str.substr(j);
    if (str.find('=') != string::npos) {
      setvar(str);
      continue;
    }

    if (i + 1 >= argc) {
      set(str, true,true);
      continue;
    }
    string str2 = argv[i + 1];
    if (str2.front() == '-') {
      set(str, true,true);
      continue;
    }

    i++;
    setjson(str, argv[i]);
    continue;
  }

  // parse default config file
  string argv0 = argv[0];
  std::vector<std::string> tries={argv0+".json",
                                  argv0+".yaml",
                                  argv0+".xml",
                                 "Default.json",
                                 "Default.yaml",
                                 "Default.xml",
                                  argv0+".cfg",
                                 "Default.cfg"};
  auto fileExists=[](const std::string& filename)
  {
      std::ifstream f(filename.c_str());
      return f.good();
  };
  if(exist("conf")){
      parseFile(get<std::string>("conf",""));
  }
  else
  for(auto& it:tries)
      if(fileExists(it)){
          parseFile(it);
          break;
      }
  return unParsed;
}


inline bool Svar::parseFile(const std::string& file_path)
{
    Svar var=loadFile(file_path);
    if(var.isUndefined()) return false;
    if(isUndefined()) {*this=var;return true;}
    *this=var+(*this);
    return true;
}

template <typename T>
T Svar::arg(const std::string& name, T def, const std::string& help) {
    Svar argInfo=array({name,def,help});
    Svar& args=(*this)["__builtin__"]["args"];
    if(!args.isArray()) args=array();
    args.push_back(argInfo);
    return get(name,def,true);
}

inline std::string Svar::helpInfo()
{
    arg<std::string>("conf", "Default.cfg",
                     "The default configure file going to parse.");
    arg<bool>("help", false, "Show the help information.");
    Svar args=(*this)["__builtin__"]["args"];
    if(get("complete_function_request",false))
    {
        std::string str;
        for (int i=0;i<(int)args.length();i++) {
          str+=" -"+args[i][0].castAs<std::string>();
        }
        return str;
    }
    std::stringstream sst;
    int width = get("help_colums", 80, true);
    int namePartWidth = width / 5 - 1;
    int statusPartWidth = width * 2 / 5 - 1;
    int introPartWidth = width * 2 / 5;
    std::string usage = get<std::string>("__usage__", "");
    if (usage.empty()) {
      usage = "Usage:\n" + get<std::string>("__name__", "exe") +
              " [--help] [-conf configure_file]"
              " [-arg_name arg_value]...\n";
    }
    sst << usage << std::endl;

    std::string desc;
    if (exist("__version__"))
      desc += "Version: " + get<std::string>("__version__","1.0") + ", ";
    desc +=
        "Using Svar supported argument parsing. The following table listed "
        "several argument introductions.\n";
    sst << printTable({{width, desc}});
    if(!args.isArray()) return "";

    sst << printTable({{namePartWidth, "Argument"},
                       {statusPartWidth, "Type(default->setted)"},
                       {introPartWidth, "Introduction"}});
    for (int i = 0; i < width; i++) sst << "-";
    sst << std::endl;

    for (int i=0;i<(int)args.length();i++) {
      Svar info=args[i];
      std::stringstream defset;
      Svar def    = info[1];
      std::string name=info[0].castAs<std::string>();
      Svar setted = get(name,Svar::Undefined(),true);
      if(setted.isUndefined()||def==setted) defset<<def;
      else defset<<def<<"->"<<setted;
      std::string status = def.typeName() + "(" + defset.str() + ")";
      std::string intro = info[2].castAs<std::string>();
      sst << printTable({{namePartWidth, "-" +name},
                         {statusPartWidth, status},
                         {introPartWidth, intro}});
    }
    return sst.str();
}

inline std::string Svar::printTable(
    std::vector<std::pair<int, std::string>> line) {
  std::stringstream sst;
  while (true) {
    size_t emptyCount = 0;
    for (auto& it : line) {
      size_t width = it.first;
      std::string& str = it.second;
      if (str.size() <= width) {
        sst << std::setw(width) << std::setiosflags(std::ios::left) << str
            << " ";
        str.clear();
        emptyCount++;
      } else {
        sst << str.substr(0, width) << " ";
        str = str.substr(width);
      }
    }
    sst << std::endl;
    if (emptyCount == line.size()) break;
  }
  return sst.str();
}

inline Svar Svar::operator -()const
{
    return classObject().call(*this,"__neg__");
}

#define DEF_SVAR_OPERATOR_IMPL(SNAME)\
{\
    auto& cls=classObject();\
    Svar ret=cls.call(*this,#SNAME,rh);\
    if(ret.isUndefined()) throw SvarExeption(cls.__name__+" operator "#SNAME" with rh: "+rh.typeName()+"returned Undefined.");\
    return ret;\
}

inline Svar Svar::operator +(const Svar& rh)const
DEF_SVAR_OPERATOR_IMPL(__add__)

inline Svar Svar::operator -(const Svar& rh)const
DEF_SVAR_OPERATOR_IMPL(__sub__)

inline Svar Svar::operator *(const Svar& rh)const
DEF_SVAR_OPERATOR_IMPL(__mul__)

inline Svar Svar::operator /(const Svar& rh)const
DEF_SVAR_OPERATOR_IMPL(__div__)

inline Svar Svar::operator %(const Svar& rh)const
DEF_SVAR_OPERATOR_IMPL(__mod__)

inline Svar Svar::operator ^(const Svar& rh)const
DEF_SVAR_OPERATOR_IMPL(__xor__)

inline Svar Svar::operator |(const Svar& rh)const
DEF_SVAR_OPERATOR_IMPL(__or__)

inline Svar Svar::operator &(const Svar& rh)const
DEF_SVAR_OPERATOR_IMPL(__and__)

inline bool Svar::operator ==(const Svar& rh)const{
    Svar eq_func=classObject()["__eq__"];
    if(!eq_func.isFunction()) return _obj==rh.value();
    Svar ret=eq_func(*this,rh);
    assert(ret.is<bool>());
    return ret.as<bool>();
}

inline bool Svar::operator <(const Svar& rh)const{
    Svar lt_func=classObject()["__lt__"];
    if(!lt_func.isFunction()) return _obj==rh.value();
    Svar ret=lt_func(*this,rh);
    assert(ret.is<bool>());
    return ret.as<bool>();
}

inline std::ostream& operator <<(std::ostream& ost,const Svar& self)
{
    return self.dump(ost,2);
}

inline const Svar& Svar::Undefined(){
    static Svar v(std::make_shared<SvarValue>());
    return v;
}

inline const Svar& Svar::Null()
{
    static Svar v=create<std::nullptr_t>(nullptr);
    return v;
}

inline Svar& Svar::instance(){
    static Svar v=Svar::object();
    return v;
}

inline Svar  Svar::loadFile(const std::string& file_path)
{
    std::ifstream ifs(file_path);
    if(!ifs.is_open()) return Svar();

    auto getExtension=[](const std::string& filename)->std::string{
        auto idx = filename.find_last_of('.');
        if (idx == std::string::npos)
          return "";
        else
          return filename.substr(idx+1);
    };
    std::string ext=getExtension(file_path);
    Svar ext2cls=Svar::object({{"json","Json"},
                               {"xml","Xml"},
                               {"yaml","Yaml"},
                               {"yml","Yaml"},
                               {"cfg","Cfg"}});
    Svar cls=Svar::instance()["__builtin__"][ext2cls[ext]];
    if(!cls.isClass()){
        std::cerr<<"Svar::loadFile don't support format "<<ext<<".\n";
        return Svar();
    }

    try{
        if(cls.exist("loadFile"))
            return cls.call("loadFile",file_path);
        std::stringstream sst;
        std::string line;
        while(std::getline(ifs,line)) sst<<line<<std::endl;
        return cls.call("load",sst.str());
    }
    catch(SvarExeption e){
        std::cerr<<e.what();
    }
    return Svar::Undefined();
}

inline Svar::svar_interator::svar_interator(Svar it,Svar::svar_interator::IterType tp)
    :_it(new Svar(it)),_type(tp)
{}

inline Svar Svar::svar_interator::operator*(){
    typedef std::vector<Svar>::const_iterator array_iter;
    typedef std::unordered_map<std::string,Svar>::const_iterator object_iter;
    switch (_type) {
    case Object: return *_it->as<object_iter>();break;
    case Array: return *_it->as<array_iter>();break;
    default:  return *_it;
    }
}

inline Svar::svar_interator& Svar::svar_interator::operator++()
{
    typedef std::vector<Svar>::const_iterator array_iter;
    typedef std::unordered_map<std::string,Svar>::const_iterator object_iter;
    switch (_type)
    {
    case Object: _it->as<object_iter>()++;break;
    case Array: _it->as<array_iter>()++;break;
    default: break;
    }
    return *this;
}

inline bool Svar::svar_interator::operator==(const svar_interator& other) const
{
    typedef std::vector<Svar>::const_iterator array_iter;
    typedef std::unordered_map<std::string,Svar>::const_iterator object_iter;
    switch (_type)
    {
    case Object: return _it->as<object_iter>()==other._it->as<object_iter>();
    case Array: return _it->as<array_iter>()==other._it->as<array_iter>();
    default: return _it==other._it;break;
    }
}

inline Svar::svar_interator Svar::begin()const
{
    if(isObject()) return svar_interator(as<SvarObject>()._var.begin(),svar_interator::Object);
    else if(isArray()) return svar_interator(as<SvarArray>()._var.begin(),svar_interator::Array);
    return svar_interator(*this,svar_interator::Others);
}

inline Svar::svar_interator Svar::end()const
{
    if(isObject()) return svar_interator(as<SvarObject>()._var.end(),svar_interator::Object);
    else if(isArray()) return svar_interator(as<SvarArray>()._var.end(),svar_interator::Array);
    return svar_interator(*this,svar_interator::Others);
}

inline Svar::svar_interator Svar::find(const Svar& idx)const
{
    if(isObject()) return svar_interator(as<SvarObject>()._var.find(idx.castAs<std::string>()),svar_interator::Object);
    return end();
}

inline std::ostream& operator<<(std::ostream& ost,const SvarClass& rh){
    ost<<"class "<<rh.__name__<<"():\n";
    std::stringstream  content;
    if(!rh.__doc__.empty()) content<<rh.__doc__<<std::endl;
    if(rh._attr.isObject()&&rh._attr.length()){
        content<<"Methods defined here:\n";
        for(std::pair<std::string,Svar> it:rh._attr)
        {
            if(!it.second.isFunction()) continue;
            content<<it.second.as<SvarFunction>();
        }
        content<<"Property defined here:\n\n";
        for(std::pair<std::string,Svar> it:rh._attr)
        {
            if(!it.second.isProperty()) continue;
            content<<it.second.as<SvarClass::SvarProperty>()<<std::endl;
        }
    }
    std::string line;
    while(std::getline(content,line)){ost<<"|  "<<line<<std::endl;}
    return ost;
}

template <typename T>
class caster<std::vector<T> >{
public:
    static Svar from(const Svar& var){
        if(var.is<std::vector<T>>()) return var;

        if(!var.isArray()) return Svar::Undefined();

        std::vector<T> ret;
        ret.reserve(var.length());
        for(const Svar& v:var.as<SvarArray>()._var)
        {
            ret.push_back(v.castAs<T>());
        }

        return Svar::create(ret);
    }

    static Svar to(const std::vector<T>& var){
        return std::make_shared<SvarArray>(std::vector<Svar>(var.begin(),var.end()));
    }
};

template <>
class caster<std::vector<Svar> >{
public:
    static Svar from(const Svar& var){
        if(var.is<std::vector<Svar>>()) return var;
        return Svar();
    }

    static Svar to(const std::vector<Svar>& var){
        return (std::shared_ptr<SvarValue>)std::make_shared<SvarArray>(var);
    }

    static Svar to(std::vector<Svar>&& var){
        return (std::shared_ptr<SvarValue>)std::make_shared<SvarArray>(std::move(var));
    }
};

template <typename T>
class caster<std::list<T> >{
public:
    static Svar from(const Svar& var){
        if(var.is<std::list<T>>()) return var;

        if(!var.isArray()) return Svar::Undefined();

        std::list<T> ret;
        for(const Svar& v:var.as<SvarArray>()._var)
        {
            ret.push_back(v.castAs<T>());
        }

        return Svar::create(ret);
    }

    static Svar to(const std::list<T>& var){
        return std::make_shared<SvarArray>(std::vector<Svar>(var.begin(),var.end()));
    }
};

template <typename T>
class caster<std::map<std::string,T> >{
public:
    static Svar from(const Svar& var){
        if(var.is<std::map<std::string,T> >()) return var;

        if(!var.isObject()) return  Svar::Undefined();

        std::map<std::string,T> ret;
        for(const std::pair<std::string,Svar>& v:var.as<SvarObject>()._var)
        {
            ret.insert(std::make_pair(v.first,v.second.castAs<T>()));
        }

        return Svar::create(ret);
    }

    static Svar to(const std::map<std::string,T>& var){
        return (std::shared_ptr<SvarValue>)std::make_shared<SvarObject>(std::unordered_map<std::string,Svar>(var.begin(),var.end()));
    }
};

template <typename T>
class caster<std::unordered_map<std::string,T> >{
public:
    static Svar from(const Svar& var){
        if(var.is<std::unordered_map<std::string,T> >()) return var;

        if(!var.isObject()) return  Svar::Undefined();

        std::unordered_map<std::string,T> ret;
        for(const std::pair<std::string,Svar>& v:var.as<SvarObject>()._var)
        {
            ret.insert(std::make_pair(v.first,v.second.castAs<T>()));
        }

        return Svar::create(ret);
    }

    static Svar to(const std::unordered_map<std::string,T>& var){
        return (std::shared_ptr<SvarValue>)std::make_shared<SvarObject>(std::unordered_map<std::string,Svar>(var.begin(),var.end()));
    }
};

template <typename K,typename T>
class caster<std::map<K,T> >{
public:
    static Svar from(const Svar& var){
        if(var.is<std::map<K,T> >()) return var;

        if(!var.isDict()) return  Svar::Undefined();

        std::map<K,T> ret;
        for(const std::pair<Svar,Svar>& v:var.as<SvarObject>()._var)
        {
            ret.insert(std::make_pair(v.first.castAs<K>(),v.second.castAs<T>()));
        }

        return Svar::create(ret);
    }

    static Svar to(const std::map<K,T>& var){
        return (std::shared_ptr<SvarValue>)std::make_shared<SvarDict>(std::map<Svar,Svar>(var.begin(),var.end()));
    }
};

template <typename T>
class caster<std::shared_ptr<T> >{
    using H=std::shared_ptr<T>;
public:
    static Svar from(const Svar& var){
        if(var.is<H>()) return var;
        if(var.isNull()) return Svar::create(H());

        return  Svar::Undefined();
    }

    static Svar to(const std::shared_ptr<T>& var){
        if(!var) return Svar::Null();
        return Svar::create(var);
    }
};

template <>
class caster<std::nullptr_t >{
public:
    static Svar from(const Svar& var){
        return var;
    }

    static Svar to(std::nullptr_t v){
        return Svar::Null();
    }
};

template <int sz>
class caster<char[sz]>{
public:
    static Svar to(const char* v){
        return Svar(std::string(v));
    }
};

template <>
class caster<const char*>{
public:
    static Svar from(const Svar& var){
        if(var.is<std::string>())
            return Svar::create(var.as<std::string>().c_str());
        return Svar();
    }

    static Svar to(const char* v){
        return std::string(v);
    }
};

inline std::istream& operator >>(std::istream& ist,Svar& self)
{
    Svar json=Svar::instance()["__builtin__"]["Json"];
    self=json.call("load",std::string( (std::istreambuf_iterator<char>(ist)) , std::istreambuf_iterator<char>() ));
    return ist;
}

#if !defined(__SVAR_NOBUILTIN__)

inline int Svar::dtos(double value,char* buf)
{
    auto first=buf;
    assert(std::isfinite(value));

    if (value == 0) {
        *first++ = '0';
        *first++ = '.';
        *first++ = '0';
        return first-buf;
    };
    if (std::signbit(value))
    {
        value = -value;
        *first++ = '-';
    }
    int len = 0;
    int decimal_exponent = 0;
    dtoa_impl::grisu2(first, len, decimal_exponent, value);

    assert(len <= std::numeric_limits<double>::max_digits10);

    // Format the buffer like printf("%.*g", prec, value)
    constexpr int kMinExp = -4;
    // Use digits10 here to increase compatibility with version 2.
    constexpr int kMaxExp = std::numeric_limits<double>::digits10;

    return dtoa_impl::format_buffer(first, len, decimal_exponent, kMinExp, kMaxExp)-buf;
}

/// Json save and load class
class Json final {
public:
    enum JsonParse {
        STANDARD, COMMENTS
    };

    static Svar load(const std::string& in){
        Json parser(in,STANDARD);
        Svar result = parser.parse_json(0);
        // Check for any trailing garbage
        parser.consume_garbage();
        if (parser.failed){
            return Svar();
        }
        if (parser.i != in.size())
            return parser.fail("unexpected trailing " + esc(in[parser.i]));

        return result;
    }

private:
    Json(const std::string& str_input, JsonParse parse_strategy=STANDARD)
        :str(str_input),i(0),failed(false),strategy(parse_strategy){
        for(int i=0;i<128;i++) {invalidmask[i]=0;namemask[i]=0;}
        for(char c='0';c<='9';c++) namemask[c]=1;
        for(char c='A';c<='Z';c++) namemask[c]=2;
        for(char c='a';c<'z';c++) namemask[c]=2;
        namemask['_']=2;
    }

    std::string str;
    size_t i;
    std::string err;
    bool failed;
    const JsonParse strategy;
    const int max_depth = 200;
    int   invalidmask[128],namemask[128];

    /// fail(msg, err_ret = Json())
    Svar fail(std::string &&msg) {
        if (!failed)
            err = std::move(msg);
        failed = true;
        throw SvarExeption(msg);
        return Svar();
    }

    /// Advance until the current character is non-whitespace.
    void consume_whitespace() {
        while (str[i] == ' ' || str[i] == '\r' || str[i] == '\n' || str[i] == '\t')
            i++;
    }

    /// Advance comments (c-style inline and multiline).
    bool consume_comment() {
      bool comment_found = false;
      if (str[i] == '/') {
        i++;
        if (i == str.size())
          fail("unexpected end of input after start of comment");
        if (str[i] == '/') { // inline comment
          i++;
          // advance until next line, or end of input
          while (i < str.size() && str[i] != '\n') {
            i++;
          }
          comment_found = true;
        }
        else if (str[i] == '*') { // multiline comment
          i++;
          if (i > str.size()-2)
            fail("unexpected end of input inside multi-line comment");
          // advance until closing tokens
          while (!(str[i] == '*' && str[i+1] == '/')) {
            i++;
            if (i > str.size()-2)
              fail("unexpected end of input inside multi-line comment");
          }
          i += 2;
          comment_found = true;
        }
        else
          fail("malformed comment");
      }
      return comment_found;
    }

    /// Advance until the current character is non-whitespace and non-comment.
    void consume_garbage() {
      consume_whitespace();
      if(strategy == JsonParse::COMMENTS) {
        bool comment_found = false;
        do {
          comment_found = consume_comment();
          if (failed) return;
          consume_whitespace();
        }
        while(comment_found);
      }
    }

    /// Return the next non-whitespace character. If the end of the input is reached,
    /// flag an error and return 0.
    char get_next_token() {
        consume_garbage();
        if (failed) return (char)0;
        if (i == str.size())
            fail("unexpected end of input");

        return str[i++];
    }

    /// Encode pt as UTF-8 and add it to out.
    void encode_utf8(long pt, std::string & out) {
        if (pt < 0)
            return;

        if (pt < 0x80) {
            out += static_cast<char>(pt);
        } else if (pt < 0x800) {
            out += static_cast<char>((pt >> 6) | 0xC0);
            out += static_cast<char>((pt & 0x3F) | 0x80);
        } else if (pt < 0x10000) {
            out += static_cast<char>((pt >> 12) | 0xE0);
            out += static_cast<char>(((pt >> 6) & 0x3F) | 0x80);
            out += static_cast<char>((pt & 0x3F) | 0x80);
        } else {
            out += static_cast<char>((pt >> 18) | 0xF0);
            out += static_cast<char>(((pt >> 12) & 0x3F) | 0x80);
            out += static_cast<char>(((pt >> 6) & 0x3F) | 0x80);
            out += static_cast<char>((pt & 0x3F) | 0x80);
        }
    }

    /// Format char c suitable for printing in an error message.
    static inline std::string esc(char c) {
        char buf[12];
        if (static_cast<uint8_t>(c) >= 0x20 && static_cast<uint8_t>(c) <= 0x7f) {
            snprintf(buf, sizeof buf, "'%c' (%d)", c, c);
        } else {
            snprintf(buf, sizeof buf, "(%d)", c);
        }
        return std::string(buf);
    }

    static inline bool in_range(long x, long lower, long upper) {
        return (x >= lower && x <= upper);
    }

    inline void handle_escape(long& last_escaped_codepoint,std::string& out){
        assert(i != str.size());

        char ch = str[i++];

        switch (ch) {
        case 'b':
            out += '\b';return;
        case 'f':
            out += '\f';return;
        case 'r':
            out += '\r';return;
        case 'n':
            out += '\n';return;
        case 't':
            out += '\t';return;
        case '"':
        case '\\':
        case '/':
            out += ch;return;
        case 'u':
        {
            std::string esc = str.substr(i, 4);
            if (esc.length() < 4) {
                fail("bad \\u escape: " + esc);
            }

            long codepoint = strtol(esc.data(), nullptr, 16);

            if (in_range(last_escaped_codepoint, 0xD800, 0xDBFF)
                    && in_range(codepoint, 0xDC00, 0xDFFF)) {
                // Reassemble the two surrogate pairs into one astral-plane character, per
                // the UTF-16 algorithm.
                encode_utf8((((last_escaped_codepoint - 0xD800) << 10)
                             | (codepoint - 0xDC00)) + 0x10000, out);
                last_escaped_codepoint = -1;
            } else {
                encode_utf8(last_escaped_codepoint, out);
                last_escaped_codepoint = codepoint;
            }

            i += 4;
        }
            return;
        default:
            fail("invalid escape character " + esc(ch));
            break;
        }
    }

    /// Parse a string, starting at the current position.
    std::string parse_string() {
        std::string out;
        long last_escaped_codepoint = -1;
        while (true) {
            if (i == str.size())
                fail("unexpected end of input in string");

            char ch = str[i++];

            switch (ch) {
            case '"':
                encode_utf8(last_escaped_codepoint, out);
                return out;
            case '\\':
                handle_escape(last_escaped_codepoint,out);
                break;
            default:
                if(last_escaped_codepoint>=0){
                    encode_utf8(last_escaped_codepoint, out);
                    last_escaped_codepoint = -1;
                }
                out += ch;
                break;
            }
        }
    }

    /// Parse a double.
    Svar parse_number() {
        size_t start_pos = i;

        if (str[i] == '-')
            i++;

        // Integer part
        if (str[i] == '0') {
            i++;
            if (in_range(str[i], '0', '9'))
                return fail("leading 0s not permitted in numbers");
        } else if (in_range(str[i], '1', '9')) {
            i++;
            while (in_range(str[i], '0', '9'))
                i++;
        } else {
            return fail("invalid " + esc(str[i]) + " in number");
        }

        if (str[i] != '.' && str[i] != 'e' && str[i] != 'E'
                && (i - start_pos) <= static_cast<size_t>(std::numeric_limits<int>::digits10)) {
            return std::atoi(str.c_str() + start_pos);
        }

//        // Decimal part
//        if (str[i] == '.') {
//            i++;
//            if (!in_range(str[i], '0', '9'))
//                return fail("at least one digit required in fractional part");

//            while (in_range(str[i], '0', '9'))
//                i++;
//        }

//        // Exponent part
//        if (str[i] == 'e' || str[i] == 'E') {
//            i++;

//            if (str[i] == '+' || str[i] == '-')
//                i++;

//            if (!in_range(str[i], '0', '9'))
//                return fail("at least one digit required in exponent");

//            while (in_range(str[i], '0', '9'))
//                i++;
//        }

        double d;
        i=fast_double_parser::parse_number(str.c_str()+start_pos,&d)-str.c_str();
        return d;
//        return std::strtod(str.c_str() + start_pos, nullptr);
    }

    std::string parse_name(){
        size_t start_i=i-1;
        while(i<str.size()&&namemask[str[i]]>=1)++i;
        std::string ret=str.substr(start_i,i-start_i);
        return ret;
    }

    /// Expect that 'str' starts at the character that was just read. If it does, advance
    /// the input and return res. If not, flag an error.
    Svar expect(const std::string &expected, const Svar& res) {
        assert(i != 0);
        i--;
        if (str.compare(i, expected.length(), expected) == 0) {
            i += expected.length();
            return res;
        } else if (namemask[str[i++]]>=2){
            return parse_name();
        }
        else{
            return fail("parse error: expected " + expected + ", got " + str.substr(i, expected.length()));
        }
    }

    /// Parse a JSON object.
    Svar parse_json(int depth) {
        if (depth > max_depth) {
            return fail("exceeded maximum nesting depth");
        }

        char ch = get_next_token();
        if (failed)
            return Svar();

        switch (ch) {
        case 't':
            return expect("true", true);
        case 'f':
            return expect("false", false);
        case 'n':
            return expect("null", Svar::Null());
        case '"':
            return parse_string();
        case '{':{
            std::unordered_map<std::string, Svar> data;
            ch = get_next_token();
            if (ch == '}')
                return data;

            while (1) {
                std::string key;
                if (ch == '"')
                    key = parse_string();
                else if (namemask[ch]>=2){
                    key = parse_name();
                }
                else
                    return fail("expected '\"' in object, got " + esc(ch));

                if (failed)
                    return Svar();

                ch = get_next_token();
                if (ch != ':')
                    return fail("expected ':' in object, got " + esc(ch));

                data.insert(std::make_pair(std::move(key),parse_json(depth + 1)));
                if (failed)
                    return Svar();

                ch = get_next_token();
                if (ch == '}')
                    break;
                if (ch != ',')
                    return fail("expected ',' in object, got " + esc(ch));

                ch = get_next_token();
            }
            return data;
        }
        case '[':{
            std::vector<Svar> data;
            ch = get_next_token();
            if (ch == ']')
                return data;

            while (1) {
                i--;
                data.push_back(parse_json(depth + 1));
                if (failed)
                    return Svar();

                ch = get_next_token();
                if (ch == ']')
                    break;
                if (ch != ',')
                    return fail("expected ',' in list, got " + esc(ch));

                ch = get_next_token();
                (void)ch;
            }
            return data;
        }
        case '<':{
            while (1) {
                ch = get_next_token();
                if (ch == '>')
                    break;
            }
            return Svar();
        }
        default:{
            if (ch == '-' || (ch >= '0' && ch <= '9')) {
                i--;
                return parse_number();
            }
            if (namemask[ch]>=2){
                return parse_name();
            }
            break;
        }
        }

        return fail("expected value, got " + esc(ch));
    }
};

/// SharedLibrary is used to load shared libraries
class SharedLibrary
{
    enum Flags
    {
        SHLIB_GLOBAL_IMPL = 1,
        SHLIB_LOCAL_IMPL  = 2
    };
public:
    typedef std::mutex MutexRW;
    typedef std::unique_lock<std::mutex> WriteMutex;

    SharedLibrary():_handle(NULL){}

    SharedLibrary(const std::string& path):_handle(NULL)
    {
        load(path);
    }

    ~SharedLibrary(){unload();}

    bool load(const std::string& path,int flags=0)
    {
        WriteMutex lock(_mutex);

        if (_handle)
            return false;

#if (defined(_WIN32) || defined(_WIN64)) && !defined(__CYGWIN__)// Windows

        flags |= LOAD_WITH_ALTERED_SEARCH_PATH;
        _handle = LoadLibraryExA(path.c_str(), 0, flags);
        if (!_handle) return false;
#else
        int realFlags = RTLD_LAZY;
        if (flags & SHLIB_LOCAL_IMPL)
            realFlags |= RTLD_LOCAL;
        else
            realFlags |= RTLD_GLOBAL;
        _handle = dlopen(path.c_str(), realFlags);
        if (!_handle)
        {
            const char* err = dlerror();
            std::cerr<<"Can't open file "<<path<<" since "<<err<<std::endl;
            return false;
        }

#endif
        _path = path;
        return true;
    }

    void unload()
    {
        WriteMutex lock(_mutex);

        if (_handle)
        {
#if (defined(_WIN32) || defined(_WIN64)) && !defined(__CYGWIN__)// Windows
            FreeLibrary((HMODULE) _handle);
#else
            dlclose(_handle);
#endif
            _handle = 0;
            _path.clear();
        }
    }

    bool isLoaded() const
    {
        return _handle!=0;
    }

    bool hasSymbol(const std::string& name)
    {
        return getSymbol(name)!=0;
    }

    void* getSymbol(const std::string& name)
    {
        WriteMutex lock(_mutex);

        void* result = 0;
        if (_handle)
        {
#if (defined(_WIN32) || defined(_WIN64)) && !defined(__CYGWIN__)// Windows
            return (void*) GetProcAddress((HMODULE) _handle, name.c_str());
#else
            result = dlsym(_handle, name.c_str());
#endif
        }
        return result;
    }

    const std::string& getPath() const
    {
        return _path;
    }
        /// Returns the path of the library, as
        /// specified in a call to load() or the
        /// constructor.

    static std::string suffix()
    {
#if defined(__APPLE__)
        return ".dylib";
#elif defined(hpux) || defined(_hpux)
        return ".sl";
#elif defined(WIN32) || defined(WIN64)
        return ".dll";
#else
        return ".so";
#endif
    }

private:
    SharedLibrary(const SharedLibrary&);
    SharedLibrary& operator = (const SharedLibrary&);
    MutexRW     _mutex;
    std::string _path;
    void*       _handle;
};
typedef std::shared_ptr<SharedLibrary> SharedLibraryPtr;

/// Registry is used to cache shared libraries
class Registry
{
public:
    typedef std::set<std::string> FilePathList;
    Registry(){
        updatePaths();
    }

    static Svar load(std::string pluginName){
        SharedLibraryPtr plugin=get(pluginName);
        if(!plugin)
        {
            std::cerr<<"Unable to load plugin "<<pluginName<<std::endl;
            std::cerr<<"PATH=";
            for(std::string p:instance()._libraryFilePath)
                std::cerr<<p<<":";
            std::cerr<<std::endl;
            return Svar();
        }
        if(!plugin->hasSymbol("svarInstance"))
        {
            std::cerr<<"Unable to find symbol svarInstance."<<std::endl;
            return Svar();
        }

        sv::Svar* (*getInst)()=(sv::Svar* (*)())plugin->getSymbol("svarInstance");
        if(!getInst){
            std::cerr<<"No svarInstance found in "<<pluginName<<std::endl;
            return Svar();
        }
        sv::Svar* inst=getInst();
        if(!inst){
            std::cerr<<"svarInstance returned null.\n";
            return Svar();
        }

        return *inst;
    }

    static Registry& instance()
    {
        static std::shared_ptr<Registry> reg(new Registry);
        return *reg;
    }

    static SharedLibraryPtr get(std::string pluginName)
    {
        if(pluginName.empty()) return SharedLibraryPtr();
        Registry& inst=instance();
        pluginName=inst.getPluginName(pluginName);

        if(inst._registedLibs[pluginName].is<SharedLibraryPtr>())
            return inst._registedLibs.Get<SharedLibraryPtr>(pluginName);

        // find out and load the SharedLibrary
        for(std::string dir:inst._libraryFilePath)
        {
            std::string pluginPath=dir+"/"+pluginName;
            if(!fileExists(pluginPath)) continue;
            SharedLibraryPtr lib(new SharedLibrary(pluginPath));
            if(lib->isLoaded())
            {
                inst._registedLibs.set(pluginName,lib);
                return lib;
            }
        }

        // find out and load the SharedLibrary
        for(std::string dir:inst._libraryFilePath)
        {
            std::string pluginPath=dir+"/lib"+pluginName;
            if(!fileExists(pluginPath)) continue;
            SharedLibraryPtr lib(new SharedLibrary(pluginPath));
            if(lib->isLoaded())
            {
                inst._registedLibs.set(pluginName,lib);
                return lib;
            }
        }
        // failed to find the library
        return SharedLibraryPtr();
    }

    static bool erase(std::string pluginName)
    {
        if(pluginName.empty()) return false;
        Registry& inst=instance();
        pluginName=inst.getPluginName(pluginName);
        inst._registedLibs.set(pluginName,Svar());
        return true;
    }
protected:
    static bool fileExists(const std::string& filename)
    {
        return access( filename.c_str(), 0 ) == 0;
    }

    static void convertStringPathIntoFilePathList(const std::string& paths,FilePathList& filepath)
    {
    #if defined(WIN32) && !defined(__CYGWIN__)
        char delimitor = ';';
        if(paths.find(delimitor)==std::string::npos) delimitor=':';
    #else
        char delimitor = ':';
        if(paths.find(delimitor)==std::string::npos) delimitor=';';
    #endif

        if (!paths.empty())
        {
            std::string::size_type start = 0;
            std::string::size_type end;
            while ((end = paths.find_first_of(delimitor,start))!=std::string::npos)
            {
                filepath.insert(std::string(paths,start,end-start));
                start = end+1;
            }

            std::string lastPath(paths,start,std::string::npos);
            if (!lastPath.empty())
                filepath.insert(lastPath);
        }

    }

    std::string getPluginName(std::string pluginName)
    {
        std::string suffix;
        size_t idx=pluginName.find_last_of('.');
        if(idx!=std::string::npos)
        suffix=pluginName.substr(idx);
        if(suffix!=SharedLibrary::suffix())
        {
            pluginName+=SharedLibrary::suffix();
        }

        std::string folder=getFolderPath(pluginName);
        pluginName=getFileName(pluginName);
        if(folder.size()){
            _libraryFilePath.insert(folder);
        }
        return pluginName;
    }

    void updatePaths()
    {
        _libraryFilePath.clear();

        char** argv=svar.get<char**>("argv",nullptr);
        if(argv)
        {
            _libraryFilePath.insert(getFolderPath(argv[0]));//application folder
        }
        _libraryFilePath.insert(".");

        FilePathList envs={"GSLAM_LIBRARY_PATH","GSLAM_LD_LIBRARY_PATH"};
        FilePathList paths;
#ifdef __linux

#if defined(__ia64__) || defined(__x86_64__)
        paths.insert("/usr/lib/:/usr/lib64/:/usr/local/lib/:/usr/local/lib64/");
#else
        paths.insert("/usr/lib/:/usr/local/lib/");
#endif
        envs.insert("LD_LIBRARY_PATH");
#elif defined(__CYGWIN__)
        envs.insert("PATH");
        paths.insert("/usr/bin/:/usr/local/bin/");
#elif defined(WIN32)
        envs.insert("PATH");
#endif
        for(std::string env:envs)
        {
            char *ptr = std::getenv(env.c_str());
            if (ptr)
                convertStringPathIntoFilePathList(std::string(ptr),_libraryFilePath);
        }
        for(std::string ptr:paths)
            convertStringPathIntoFilePathList(ptr,_libraryFilePath);
    }

    inline std::string getFolderPath(const std::string& path) {
      auto idx = std::string::npos;
      if ((idx = path.find_last_of('/')) == std::string::npos)
        idx = path.find_last_of('\\');
      if (idx != std::string::npos)
        return path.substr(0, idx);
      else
        return "";
    }

    inline std::string getBaseName(const std::string& path) {
      std::string filename = getFileName(path);
      auto idx = filename.find_last_of('.');
      if (idx == std::string::npos)
        return filename;
      else
        return filename.substr(0, idx);
    }

    inline std::string getFileName(const std::string& path) {
      auto idx = std::string::npos;
      if ((idx = path.find_last_of('/')) == std::string::npos)
        idx = path.find_last_of('\\');
      if (idx != std::string::npos)
        return path.substr(idx + 1);
      else
        return path;
    }

    std::set<std::string>               _libraryFilePath;// where to search?
    Svar                                _registedLibs;   // already loaded
};

class SvarBuiltin{
public:
    SvarBuiltin(){
        SvarClass::Class<int32_t>().setName("int");
        SvarClass::Class<int64_t>().setName("int64_t");
        SvarClass::Class<uint32_t>().setName("uint32_t");
        SvarClass::Class<uint64_t>().setName("uint64_t");
        SvarClass::Class<unsigned char>().setName("uchar");
        SvarClass::Class<char>().setName("char");
        SvarClass::Class<float>().setName("float");
        SvarClass::Class<double>().setName("double");
        SvarClass::Class<std::string>().setName("str");
        SvarClass::Class<bool>().setName("bool");
        SvarClass::Class<SvarDict>().setName("dict");
        SvarClass::Class<SvarObject>().setName("object");
        SvarClass::Class<SvarArray>().setName("array");
        SvarClass::Class<SvarFunction>().setName("function");
        SvarClass::Class<SvarClass>().setName("class");
        SvarClass::Class<Svar>().setName("svar");
        SvarClass::Class<SvarBuffer>().setName("buffer");
        SvarClass::Class<SvarClass::SvarProperty>().setName("property");

        SvarClass::Class<void>()
                .def("__str__",[](const Svar&){return "undefined";})
        .setName("void");
        Svar::Null().classObject()
                .def("__str__",[](const Svar&){return "null";});

        SvarClass::Class<int>()
                .def("__init__",&SvarBuiltin::int_create)
                .def("__double__",[](int& i){return (double)i;})
                .def("__bool__",[](int& i){return (bool)i;})
                .def("__str__",[](int& i){return std::to_string(i);})
                .def("__eq__",[](int& self,int rh){return self==rh;})
                .def("__lt__",[](int& self,int rh){return self<rh;})
                .def("__add__",[](int& self,Svar& rh)->Svar{
            if(rh.is<int>()) return Svar(self+rh.as<int>());
            if(rh.is<double>()) return Svar(self+rh.as<double>());
            return Svar::Undefined();
        })
                .def("__sub__",[](int self,Svar& rh)->Svar{
            if(rh.is<int>()) return Svar(self-rh.as<int>());
            if(rh.is<double>()) return Svar(self-rh.as<double>());
            return Svar::Undefined();
        })
                .def("__mul__",[](int& self,Svar rh)->Svar{
            if(rh.is<int>()) return Svar(self*rh.as<int>());
            if(rh.is<double>()) return Svar(self*rh.as<double>());
            return Svar::Undefined();
        })
                .def("__div__",[](int& self,Svar rh){
            if(rh.is<int>()) return Svar(self/rh.as<int>());
            if(rh.is<double>()) return Svar(self/rh.as<double>());
            return Svar::Undefined();
        })
                .def("__mod__",[](int& self,int& rh){
            return self%rh;
        })
                .def("__neg__",[](int& self){return -self;})
                .def("__xor__",[](int& self,int& rh){return self^rh;})
                .def("__or__",[](int& self,int& rh){return self|rh;})
                .def("__and__",[](int& self,int& rh){return self&rh;});

        SvarClass::Class<bool>()
                .def("__int__",[](bool& b){return (int)b;})
                .def("__double__",[](bool& b){return (double)b;})
                .def("__str__",[](bool& b){return std::to_string(b);})
                .def("__eq__",[](bool& self,bool& rh){return self==rh;});

        SvarClass::Class<double>()
                .def("__int__",[](double& d){return (int)d;})
                .def("__bool__",[](double& d){return (bool)d;})
                .def("__str__",[](double& d){return std::to_string(d);})
                .def("__eq__",[](double& self,double rh){return self==rh;})
                .def("__lt__",[](double& self,double rh){return self<rh;})
                .def("__neg__",[](double& self){return -self;})
                .def("__add__",[](double& self,double rh){return self+rh;})
                .def("__sub__",[](double& self,double rh){return self-rh;})
                .def("__mul__",[](double& self,double rh){return self*rh;})
                .def("__div__",[](double& self,double rh){return self/rh;})
                .def("__invert__",[](double& self){return 1./self;});

        SvarClass::Class<std::string>()
                .def("__len__",[](const std::string& self){return self.size();})
                .def("__add__",[](const std::string& self,const std::string& rh){
            return self+rh;
        })
                .def("__eq__",[](const std::string& self,std::string& rh){return self==rh;})
                .def("__lt__",[](const std::string& self,std::string& rh){return self<rh;});

        SvarClass::Class<char const*>()
                .def("__str__",[](char const* str){
            return std::string(str);
        });

        SvarClass::Class<SvarArray>()
                .def("__getitem__",[](SvarArray& self,int& i){
            return self[i];
        })
                .def("__delitem__",[](SvarArray& self,const int& idx){
            std::unique_lock<std::mutex> lock(self._mutex);
            self._var.erase(self._var.begin()+idx);
        })
        .def("__add__",[](const SvarArray& self,const SvarArray& other){
            std::unique_lock<std::mutex> lock1(self._mutex);
            std::unique_lock<std::mutex> lock2(other._mutex);
            std::vector<Svar> ret(self._var);
            ret.insert(ret.end(),other._var.begin(),other._var.end());
            return ret;
        })
            .def("__mul__",[](const SvarArray& self,const int& num){
            std::vector<Svar> ret;
            for(int i=0;i<num;++i)
                ret.insert(ret.end(),self._var.begin(),self._var.end());
            return ret;
        });

        SvarClass::Class<SvarDict>()
                .def("__getitem__",&SvarDict::operator [])
                .def("__delitem__",&SvarDict::erase);


        SvarClass::Class<SvarObject>()
                .def("__getitem__",&SvarObject::operator [])
                .def("__delitem__",[](SvarObject& self,const std::string& id){
            std::unique_lock<std::mutex> lock(self._mutex);
            self._var.erase(id);
        })
                .def("__add__",[](const SvarObject& self,const SvarObject& rh){
            if(&self==&rh)
            {
                std::unique_lock<std::mutex> lock1(self._mutex);
                return self._var;
            }
            else
            {
                auto ret=self._var;
                std::unique_lock<std::mutex> lock1(self._mutex);
                std::unique_lock<std::mutex> lock2(rh._mutex);
                for(auto it:rh._var){
                    if(ret.find(it.first)==ret.end())
                        ret.insert(it);
                }
                return ret;
            }
        });

        SvarClass::Class<SvarFunction>()
                .def("__doc__",[](SvarFunction& self){
            std::stringstream sst;
            sst<<self;
            return sst.str();
        });

        SvarClass::Class<SvarClass>()
                .def("__getitem__",[](SvarClass& self,const std::string& i)->Svar{return self[i];});

        SvarClass::Class<Json>()
                .def_static("load",&Json::load);

        SvarClass::Class<SvarBuffer>()
                .def("size",&SvarBuffer::size)
            .def("__buffer__",[](Svar& self){return self;})
        .def("__str__",[](SvarBuffer& self){
          std::stringstream sst;
          sst<<"<buffer ";
          for(auto s:self.shape)
            sst<<s<<"X";
          sst<<self._format<<">";
          return sst.str();
        });

        Svar& builtin=Svar::instance()["__builtin__"];
        if(builtin.isUndefined())
            builtin=Svar::object();
        Svar  addon  ={
            {"version",SVAR_VERSION},
            {"author","Yong Zhao"},
            {"email","zdzhaoyong@mail.nwpu.edu.cn"},
            {"license","BSD"},
            {"date",std::string(__DATE__)},
            {"time",std::string(__TIME__)},
            {"Json",SvarClass::instance<Json>()}};
        builtin = addon + builtin;
#ifdef BUILD_VERSION
        builtin["tag"]=std::string(BUILD_VERSION);
#endif
        builtin["import"]=&Registry::load;
    }

    static Svar int_create(const Svar& rh){
        if(rh.is<int>()) return rh;
        if(rh.is<std::string>()) return (Svar)std::atoi(rh.as<std::string>().c_str());
        if(rh.is<double>()) return (Svar)(int)rh.as<double>();
        if(rh.is<bool>()) return (Svar)(int)rh.as<bool>();

        throw SvarExeption("Can't construct int from "+rh.typeName()+".");
        return Svar::Undefined();
    }
};

static SvarBuiltin SvarBuiltinInitializerinstance;
#endif
#endif

}

#endif
