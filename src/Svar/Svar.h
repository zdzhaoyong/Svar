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
#define SVAR_VERSION 0x000301
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

/**
@brief The Svar class, A Tiny Modern C++ Header Brings Unified Interface for Different Languages

By Using Svar, your C++ library can be called easily with different languages like C++, Java, Python and Javascript.

Svar brings the following features:

- A dynamic link library that can be used as a module in languages such as C++, Python, and Node-JS;
- Unified C++ library interface, which can be called as a plug-in module. The released library comes with documentation, making *.h header file interface description unnecessary;
- Dynamic features. It wraps variables, functions, and classes into Svar while maintaining efficiency;
- Built-in Json support, parameter configuration parsing, thread-safe reading and writing, data decoupling sharing between modules, etc.

\section s_svar_json Use Svar like JSON

Svar natually support JSON and here is some basic usage demo:
\code
    Svar null=nullptr;
    Svar b=false;
    Svar i=1;
    Svar d=2.1;
    Svar s="hello world";
    Svar v={1,2,3};
    Svar m={{"b",false},{"s","hello world"}};

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
    std::cout<<json<<std::endl;

    m=Svar::parse_json(json);
\endcode

\section s_svar_arguments Use Svar for Argument Parsing

Svar provides argument parsing functional with JSON configuration loading.
Here is a small demo shows how to use Svar for argument parsing:

@code

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

@endcode

When you use "--help" or "-help", the terminal should show the below introduction:
@code
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
@endcode
"help" and "conf" is two default parameters and users can use "conf" to load JSON file for configuration loading.


Svar supports the following parsing styles:
- "-arg value": two '-' such as "--arg value" is the same
- "-arg=value": two "--" such as "--arg=value" is the same
- "arg=value"
- "-arg" : this is the same with "arg=true", but the next argument should not be a value

Svar supports to use brief Json instead of strict Json:
@code
sample_use -b.dump -obj '{a:1,b:false}'
@endcode

\section s_svar_cppelements Use Svar to Hold Other C++ Elements

Svar can not only hold JSON types, but also any other c++ elements including functions, classes and objects.

\subsection s_svar_cppelements_sample A tiny sample of Svar holding functions and classes.

@code
#include <Svar/Svar.h>

int c_func(int a,int b){return a+b;}

class DemoClass{
public:
    DemoClass(const std::string& name):_name(name){}
    std::string getName()const{return _name;}
    void setName(std::string name){_name=name}

    static DemoClass create(std::string name){return DemoClass(name);}

    std::string _name;
};

int main(int argc,char** argv)
{
    Svar add=c_func;// hold a function
    int  c=add(a,b).castAs<int>();// use the function

    Svar create(&DemoClass::create);// hold a static member function
    DemoClass inst=create("hello svar").as<DemoClass>();// call a static member function

    Svar(&DemoClass::setName)(&inst,"I changed name");// call a member function

    Svar cls=SvarClass::instance<DemoClass>();
    Class<DemoClass>()
        .construct<const std::string&>()
        .def("getName",&DemoClass::getName)
        .def("setName",&DemoClass::setName)
        .def_static("create",&DemoClass::create);

    Svar inst2=cls("I am another instance");// construct a instance
    std::string name=inst2.call("getName").as<std::string>();
    inst2.call("setName","changed name with svar");
}
@endcode

\subsection s_svar_class Define Class Members with Svar
Svar does not do magic, so users need to bind functions mannually like boost or pybind11 did.
Fortunately, Svar provided utils to bind those functions very conveniencely.

As used in last section, there are three types of functions: constructor,
member function and static member function.
And Svar also support lambda expression of functions so that users able to extend functions more easily.
@code
    Class<DemoClass>("Demo")             // change the name to Demo
        .construct<const std::string&>() // constructor
        .def("getName",&DemoClass::getName) // const member function
        .def("setName",&DemoClass::setName) // member function
        .def_static("create",&DemoClass::create)// static function
        .def("print",[](DemoClass& self){std::cerr<<self._name;})// lambda function
        .def_static("none",[](){});// static lambda function
@endcode

@todo Svar does not support field now, maybe it's a future work.

\subsection s_svar_operators Operators of Svar

Svar supports to call functions with operators, such as for int,
we got some math operators and we want to use it directly with Svar:
@code
    Svar a=4;
    Svar b=5;
    auto c=a+b;
    EXPECT_TRUE(c.is<int>())
    EXPECT_EQ(c,20)
@endcode

The above operation is available because Svar builtin defined as following:
@code
    SvarClass::Class<int>()
    .def("__init__",&SvarBuiltin::int_create)
    .def("__double__",[](int i){return (double)i;})
    .def("__bool__",[](int i){return (bool)i;})
    .def("__eq__",[](int self,int rh){return self==rh;})
    .def("__lt__",[](int self,int rh){return self<rh;})
    .def("__add__",[](int self,Svar rh)->Svar{
        if(rh.is<int>()) return Svar(self+rh.as<int>());
        if(rh.is<double>()) return Svar(self+rh.as<double>());
        return Svar::Undefined();
    })
    .def("__sub__",[](int self,Svar rh)->Svar{
        if(rh.is<int>()) return Svar(self-rh.as<int>());
        if(rh.is<double>()) return Svar(self-rh.as<double>());
        return Svar::Undefined();
    })
    .def("__mul__",[](int self,Svar rh)->Svar{
        if(rh.is<int>()) return Svar(self*rh.as<int>());
        if(rh.is<double>()) return Svar(self*rh.as<double>());
        return Svar::Undefined();
    })
    .def("__div__",[](int self,Svar rh){
        if(rh.is<int>()) return Svar(self/rh.as<int>());
        if(rh.is<double>()) return Svar(self/rh.as<double>());
        return Svar::Undefined();
    })
    .def("__mod__",[](int self,int rh){
        return self%rh;
    })
    .def("__neg__",[](int self){return -self;})
    .def("__xor__",[](int self,int rh){return self^rh;})
    .def("__or__",[](int self,int rh){return self|rh;})
    .def("__and__",[](int self,int rh){return self&rh;});
@endcode

If you want your own class support these operators, you need to define you own implementations.
The following table listed some operators that often used.

| Operator |  Function Name |
|    --    |  --            |
|    +     |  "__add__"     |
|    -     |  "__sub__"     |
|    *     |  "__mul__"     |
|    /     |  "__div__"     |
|    %     |  "__mod__"     |
|    &     |  "__and__"     |
|    \|    |  "__or__"      |
|    ^     |  "__xor__"     |
|    =     |  "__eq__"      |
|    !=    |  "__nq__"      |
|    <     |  "__lt__"      |
|    >     |  "__gt__"      |
|    <=    |  "__le__"      |
|    >=    |  "__ge__"      |
|    []    |  "__getitem__" |

\subsection s_svar_inherit Inherit of Classes

@code
#include <Svar/Svar.h>

using namespace sv;

class BBase{
public:
    virtual bool isBBase(){return true;}
};

class BaseClass: public BBase{
public:
    BaseClass(int age):age_(age){}

    int getAge()const{return age_;}
    void setAge(int a){age_=a;}

    virtual bool isBBase(){return false;}

    int age_;
};

int main(int argc,char** argv)
{
    Class<BBase>()
        .def("isBBase",&BBase::isBBase);

    Class<BaseClass>()
        .inherit<BBase,BaseClass>() // use this to inherit
        .def_static("__init__",[](int age){return BaseClass(age);})// replace construct<int>()
        .def("getAge",&BaseClass::getAge)
        .def("setAge",&BaseClass::setAge);

    Svar cls=SvarClass::instance<BaseClass>();
    Svar obj=cls(10);// ten years old?
    assert(!obj.call("isBBase).as<bool>());
    return 0;
}

@endcode

\section s_svar_plugin Export & Import Svar Module with C++ Shared Library

One of the most important design target of Svar is to export a shared library that
can be used by different languages.

\subsection s_svar_export Export Svar Module

The only thing needs to do is to put what you want to the singleton of Svar: Svar::instance(),
which is defined as 'svar', and expose the symbol with macro EXPORT_SVAR_INSTANCE.

@code
#include "sv/Svar.h"

using namespace sv;

int add(int a,int b){
    return a+b;
}

class ApplicationDemo{
public:
    ApplicationDemo(std::string name):_name(name){
        std::cout<<"Application Created.";
    }
    std::string name()const{return _name;}
    std::string gslam_version()const{return "3.2";}
    std::string _name;
};

REGISTER_SVAR_MODULE(sample)// see, so easy, haha
{
    svar["__name__"]="sample_module";
    svar["__doc__"]="This is a demo to show how to export a module using svar.";
    svar["add"]=add;

    Class<ApplicationDemo>()
            .construct<std::string>()
            .def("name",&ApplicationDemo::name)
            .def("gslam_version",&ApplicationDemo::gslam_version);

    svar["ApplicationDemo"]=SvarClass::instance<ApplicationDemo>();
}

EXPORT_SVAR_INSTANCE // export the symbol of Svar::instance
@endcode

Compile this file to a shared library "libsample.so" or "sample.dll", and we are going to access this module with Svar later.

\subsection s_svar_doc Documentation of Svar Module
A Svar module is designed to be self maintained, which means the module can be called without header or documentation.
Since SvarFunction auto generated function signatures, so that users are able to know how to call Svar functions.

One can use the 'svar' application from https://github.com/zdzhaoyong/Svar to access the context.

@code
-> svar doc -plugin sample
{
  "ApplicationDemo" : <class at 0x15707d0>,
  "__builtin__" : {
    "Json" : <class at 0x156fd80>,
    "version" : 256
  },
  "__doc__" : "This is a demo to show how to export a module using svar.",
  "__name__" : "sample_module",
  "add" : <function at 0x15706e0>
}
-> svar doc -plugin sample -key ApplicationDemo
class ApplicationDemo():
|  ApplicationDemo.__init__(...)
|      ApplicationDemo.__init__(str)->ApplicationDemo
|
|
|  Methods defined here:
|  ApplicationDemo.__init__(...)
|      ApplicationDemo.__init__(str)->ApplicationDemo
|
|
|  ApplicationDemo.gslam_version(...)
|      ApplicationDemo.gslam_version(ApplicationDemo const*)->str
|
|
|  ApplicationDemo.name(...)
|      ApplicationDemo.name(ApplicationDemo const*)->str
|
|
@endcode

\subsection s_svar_usecpp Use Svar Module in C++
We are able to load the Svar instance exported by marco with EXPORT_SVAR_INSTANCE
using svar.import().

@code
#include "Svar/Svar.h"

using namespace sv;

int main(int argc,char** argv){
    Svar sampleModule=svar.import("sample");

    Svar ApplicationDemo=sampleModule["ApplicationDemo"];

    Svar instance=ApplicationDemo("zhaoyong");
    std::cout<<instance.call("gslam_version");

    std::cout<<ApplicationDemo.as<SvarClass>();
    return 0;
}
@endcode

 */

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

    Svar Call(std::vector<Svar> argv)const{
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

                    arg& a_ = a.as<arg>();
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
        return def("__init__",[](Args... args){
            return std::unique_ptr<C>(new C(args...));
        });
    }

    template <typename... Args>
    Class& unique_construct(){
        return def("__init__",[](Args... args){
            return std::unique_ptr<C>(new C(args...));
        });
    }

    template <typename... Args>
    Class& shared_construct(){
        return def("__init__",[](Args... args){
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

    std::map<std::type_index,value_t> lut={{typeid(void),undefined_t},
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
                                           {typeid(SvarClass),value_t::svarclass_t}};
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
    return is<SvarClass::SvarProperty>();
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

        Svar cl=var.classObject();
        if(cl.isClass()){
            SvarClass& srcClass=cl.as<SvarClass>();
            Svar cvt=srcClass._attr["__"+SvarClass::Class<T>().name()+"__"];
            if(cvt.isFunction()){
                Svar ret=cvt(var);
                if(ret.is<T>()) return ret;
            }
        }

        SvarClass& destClass=*SvarClass::instance<T>();
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
};

template <typename T>
Svar::Svar(const T& var):Svar(caster<T>::to(var))
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
        return o<<"<"<<typeName()<<" at "<<std::to_string((long)value().get())<<">";
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
    Svar clsobj=classObject();
    Svar eq_func=clsobj.as<SvarClass>()["__eq__"];
    if(!eq_func.isFunction()) return _obj==rh.value();
    Svar ret=eq_func(*this,rh);
    assert(ret.is<bool>());
    return ret.as<bool>();
}

inline bool Svar::operator <(const Svar& rh)const{
    Svar clsobj=classObject();
    Svar lt_func=clsobj.as<SvarClass>()["__lt__"];
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

/*!
@brief implements the Grisu2 algorithm for binary to decimal floating-point
conversion.
*/
namespace dtoa_impl
{

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

    // Split M+ = f * 2^e into two parts p1 and p2 (note: e < 0):
    //
    //      M+ = f * 2^e
    //         = ((f div 2^-e) * 2^-e + (f mod 2^-e)) * 2^e
    //         = ((p1        ) * 2^-e + (p2        )) * 2^e
    //         = p1 + p2 * 2^e

    const diyfp one(std::uint64_t{1} << -M_plus.e, M_plus.e);

    auto p1 = static_cast<std::uint32_t>(M_plus.f >> -one.e); // p1 = f div 2^-e (Since -e >= 32, p1 fits into a 32-bit int.)
    std::uint64_t p2 = M_plus.f & (one.f - 1);                    // p2 = f mod 2^-e

    // 1)
    //
    // Generate the digits of the integral part p1 = d[n-1]...d[1]d[0]

    assert(p1 > 0);

    std::uint32_t pow10;
    const int k = find_largest_pow10(p1, pow10);

    //      10^(k-1) <= p1 < 10^k, pow10 = 10^(k-1)
    //
    //      p1 = (p1 div 10^(k-1)) * 10^(k-1) + (p1 mod 10^(k-1))
    //         = (d[k-1]         ) * 10^(k-1) + (p1 mod 10^(k-1))
    //
    //      M+ = p1                                             + p2 * 2^e
    //         = d[k-1] * 10^(k-1) + (p1 mod 10^(k-1))          + p2 * 2^e
    //         = d[k-1] * 10^(k-1) + ((p1 mod 10^(k-1)) * 2^-e + p2) * 2^e
    //         = d[k-1] * 10^(k-1) + (                         rest) * 2^e
    //
    // Now generate the digits d[n] of p1 from left to right (n = k-1,...,0)
    //
    //      p1 = d[k-1]...d[n] * 10^n + d[n-1]...d[0]
    //
    // but stop as soon as
    //
    //      rest * 2^e = (d[n-1]...d[0] * 2^-e + p2) * 2^e <= delta * 2^e

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
        //
        //      M+ = buffer * 10^n + (p1 + p2 * 2^e)
        //      pow10 = 10^n
        //

        // Now check if enough digits have been generated.
        // Compute
        //
        //      p1 + p2 * 2^e = (p1 * 2^-e + p2) * 2^e = rest * 2^e
        //
        // Note:
        // Since rest and delta share the same exponent e, it suffices to
        // compare the significands.
        const std::uint64_t rest = (std::uint64_t{p1} << -one.e) + p2;
        if (rest <= delta)
        {
            // V = buffer * 10^n, with M- <= V <= M+.

            decimal_exponent += n;

            // We may now just stop. But instead look if the buffer could be
            // decremented to bring V closer to w.
            //
            // pow10 = 10^n is now 1 ulp in the decimal representation V.
            // The rounding procedure works with diyfp's with an implicit
            // exponent of e.
            //
            //      10^n = (10^n * 2^-e) * 2^e = ulp * 2^e
            //
            const std::uint64_t ten_n = std::uint64_t{pow10} << -one.e;
            grisu2_round(buffer, length, dist, delta, rest, ten_n);

            return;
        }

        pow10 /= 10;
        //
        //      pow10 = 10^(n-1) <= p1 < 10^n
        // Invariants restored.
    }

    // 2)
    //
    // The digits of the integral part have been generated:
    //
    //      M+ = d[k-1]...d[1]d[0] + p2 * 2^e
    //         = buffer            + p2 * 2^e
    //
    // Now generate the digits of the fractional part p2 * 2^e.
    //
    // Note:
    // No decimal point is generated: the exponent is adjusted instead.
    //
    // p2 actually represents the fraction
    //
    //      p2 * 2^e
    //          = p2 / 2^-e
    //          = d[-1] / 10^1 + d[-2] / 10^2 + ...
    //
    // Now generate the digits d[-m] of p1 from left to right (m = 1,2,...)
    //
    //      p2 * 2^e = d[-1]d[-2]...d[-m] * 10^-m
    //                      + 10^-m * (d[-m-1] / 10^1 + d[-m-2] / 10^2 + ...)
    //
    // using
    //
    //      10^m * p2 = ((10^m * p2) div 2^-e) * 2^-e + ((10^m * p2) mod 2^-e)
    //                = (                   d) * 2^-e + (                   r)
    //
    // or
    //      10^m * p2 * 2^e = d + r * 2^e
    //
    // i.e.
    //
    //      M+ = buffer + p2 * 2^e
    //         = buffer + 10^-m * (d + r * 2^e)
    //         = (buffer * 10^m + d) * 10^-m + 10^-m * r * 2^e
    //
    // and stop as soon as 10^-m * r * 2^e <= delta * 2^e

    assert(p2 > delta);

    int m = 0;
    for (;;)
    {
        // Invariant:
        //      M+ = buffer * 10^-m + 10^-m * (d[-m-1] / 10 + d[-m-2] / 10^2 + ...) * 2^e
        //         = buffer * 10^-m + 10^-m * (p2                                 ) * 2^e
        //         = buffer * 10^-m + 10^-m * (1/10 * (10 * p2)                   ) * 2^e
        //         = buffer * 10^-m + 10^-m * (1/10 * ((10*p2 div 2^-e) * 2^-e + (10*p2 mod 2^-e)) * 2^e
        //
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
        //
        //      M+ = buffer * 10^-m + 10^-m * p2 * 2^e
        // Invariant restored.

        // Check if enough digits have been generated.
        //
        //      10^-m * p2 * 2^e <= delta * 2^e
        //              p2 * 2^e <= 10^m * delta * 2^e
        //                    p2 <= 10^m * delta
        delta *= 10;
        dist  *= 10;
        if (p2 <= delta)
        {
            break;
        }
    }

    // V = buffer * 10^-m, with M- <= V <= M+.

    decimal_exponent -= m;

    // 1 ulp in the decimal representation is now 10^-m.
    // Since delta and dist are now scaled by 10^m, we need to do the
    // same with ulp in order to keep the units in sync.
    //
    //      10^m * 10^-m = 1 = 2^-e * 2^e = ten_m * 2^e
    //
    const std::uint64_t ten_m = one.f;
    grisu2_round(buffer, length, dist, delta, p2, ten_m);

    // By construction this algorithm generates the shortest possible decimal
    // number (Loitsch, Theorem 6.2) which rounds back to w.
    // For an input number of precision p, at least
    //
    //      N = 1 + ceil(p * log_10(2))
    //
    // decimal digits are sufficient to identify all binary floating-point
    // numbers (Matula, "In-and-Out conversions").
    // This implies that the algorithm does not produce more than N decimal
    // digits.
    //
    //      N = 17 for p = 53 (IEEE double precision)
    //      N = 9  for p = 24 (IEEE single precision)
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

        // Decimal part
        if (str[i] == '.') {
            i++;
            if (!in_range(str[i], '0', '9'))
                return fail("at least one digit required in fractional part");

            while (in_range(str[i], '0', '9'))
                i++;
        }

        // Exponent part
        if (str[i] == 'e' || str[i] == 'E') {
            i++;

            if (str[i] == '+' || str[i] == '-')
                i++;

            if (!in_range(str[i], '0', '9'))
                return fail("at least one digit required in exponent");

            while (in_range(str[i], '0', '9'))
                i++;
        }

        return std::strtod(str.c_str() + start_pos, nullptr);
    }

    std::string parse_name(){
        size_t start_i=i-1;
        while(i<str.size()&&namemask[str[i]]>=1)++i;
        std::string ret=str.substr(start_i,i-start_i);
        return ret;
    }

    /// Expect that 'str' starts at the character that was just read. If it does, advance
    /// the input and return res. If not, flag an error.
    Svar expect(const std::string &expected, Svar res) {
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
                .def("__eq__",[](bool& self,bool& rh){return self==rh;});

        SvarClass::Class<double>()
                .def("__int__",[](double& d){return (int)d;})
                .def("__bool__",[](double& d){return (bool)d;})
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
