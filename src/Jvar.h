#ifndef GSLAM_JVAR_H
#define GSLAM_JVAR_H

#include <assert.h>
#include <cxxabi.h>
#include <deque>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <sstream>
#include <thread>
#include <typeindex>
#include <vector>
//#include </data/zhaoyong/Program/Apps/SLAM/GSLAM/GSLAM/core/JSON.h>
#include <GSLAM/core/Glog.h>

namespace Py {

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


#  if __cplusplus >= 201402L
using std::index_sequence;
using std::make_index_sequence;
#else
template<size_t ...> struct index_sequence  { };
template<size_t N, size_t ...S> struct make_index_sequence_impl : make_index_sequence_impl <N - 1, N - 1, S...> { };
template<size_t ...S> struct make_index_sequence_impl <0, S...> { typedef index_sequence<S...> type; };
template<size_t N> using make_index_sequence = typename make_index_sequence_impl<N>::type;
#endif

/* Concatenate type signatures at compile time */
template <size_t N, typename... Ts>
struct descr {
    char text[N + 1];

    constexpr descr() : text{'\0'} { }
    constexpr descr(char const (&s)[N+1]) : descr(s, make_index_sequence<N>()) { }

    template <size_t... Is>
    constexpr descr(char const (&s)[N+1], index_sequence<Is...>) : text{s[Is]..., '\0'} { }

    template <typename... Chars>
    constexpr descr(char c, Chars... cs) : text{c, static_cast<char>(cs)..., '\0'} { }

    static constexpr std::array<const std::type_info *, sizeof...(Ts) + 1> types() {
        return {{&typeid(Ts)..., nullptr}};
    }
};

template <size_t N1, size_t N2, typename... Ts1, typename... Ts2, size_t... Is1, size_t... Is2>
constexpr descr<N1 + N2, Ts1..., Ts2...> plus_impl(const descr<N1, Ts1...> &a, const descr<N2, Ts2...> &b,
                                                   index_sequence<Is1...>, index_sequence<Is2...>) {
    return {a.text[Is1]..., b.text[Is2]...};
}

template <size_t N1, size_t N2, typename... Ts1, typename... Ts2>
constexpr descr<N1 + N2, Ts1..., Ts2...> operator+(const descr<N1, Ts1...> &a, const descr<N2, Ts2...> &b) {
    return plus_impl(a, b, make_index_sequence<N1>(), make_index_sequence<N2>());
}

template <size_t N>
constexpr descr<N - 1> _(char const(&text)[N]) { return descr<N - 1>(text); }
constexpr descr<0> _(char const(&)[1]) { return {}; }

template <size_t Rem, size_t... Digits> struct int_to_str : int_to_str<Rem/10, Rem%10, Digits...> { };
template <size_t...Digits> struct int_to_str<0, Digits...> {
    static constexpr auto digits = descr<sizeof...(Digits)>(('0' + Digits)...);
};

// Ternary description (like std::conditional)
template <bool B, size_t N1, size_t N2>
constexpr enable_if_t<B, descr<N1 - 1>> _(char const(&text1)[N1], char const(&)[N2]) {
    return _(text1);
}
template <bool B, size_t N1, size_t N2>
constexpr enable_if_t<!B, descr<N2 - 1>> _(char const(&)[N1], char const(&text2)[N2]) {
    return _(text2);
}

template <bool B, typename T1, typename T2>
constexpr enable_if_t<B, T1> _(const T1 &d, const T2 &) { return d; }
template <bool B, typename T1, typename T2>
constexpr enable_if_t<!B, T2> _(const T1 &, const T2 &d) { return d; }

template <size_t Size> auto constexpr _() -> decltype(int_to_str<Size / 10, Size % 10>::digits) {
    return int_to_str<Size / 10, Size % 10>::digits;
}

template <typename Type> constexpr descr<1, Type> _() { return {'%'}; }

constexpr descr<0> concat() { return {}; }

template <size_t N, typename... Ts>
constexpr descr<N, Ts...> concat(const descr<N, Ts...> &descr) { return descr; }

template <size_t N, typename... Ts, typename... Args>
constexpr auto concat(const descr<N, Ts...> &d, const Args &...args)
    -> decltype(std::declval<descr<N + 2, Ts...>>() + concat(args...)) {
    return d + _(", ") + concat(args...);
}

template <size_t N, typename... Ts>
constexpr descr<N + 2, Ts...> type_descr(const descr<N, Ts...> &descr) {
    return _("{") + descr + _("}");
}

typedef char yes;
typedef char (&no)[2];

struct anyx {
  template <class T>
  anyx(const T&);
};

no operator<<(const anyx&, const anyx&);
no operator>>(const anyx&, const anyx&);

template <class T>
yes check(T const&);
no check(no);

template <typename StreamType, typename T>
struct has_loading_support {
  static StreamType& stream;
  static T& x;
  static const bool value = sizeof(check(stream >> x)) == sizeof(yes);
};

template <typename StreamType, typename T>
struct has_saving_support {
  static StreamType& stream;
  static T& x;
  static const bool value = sizeof(check(stream << x)) == sizeof(yes);
};

template <typename StreamType, typename T>
struct has_stream_operators {
  static const bool can_load = has_loading_support<StreamType, T>::value;
  static const bool can_save = has_saving_support<StreamType, T>::value;
  static const bool value = can_load && can_save;
};

}

class Svar;
class SvarValue;
class SvarFunction;
class SvarClass;
class SvarObject;
class SvarArray;
class SvarExeption;
template <typename T>
class SvarClass_;
template <typename T>
class SvarValue_;

class Svar{
public:
    /// Basic buildin types
    Svar():Svar(Null()){}
    Svar(SvarValue* v):_obj(v){}
    Svar(bool b):Svar(b?True():False()){}
    Svar(int  i):Svar(create(i)){}
    Svar(double  d):Svar(create(d)){}
    Svar(const std::string& m);
    Svar(const std::vector<Svar>& vec);//Array
    Svar(const std::map<std::string,Svar>& m);//Object
    Svar(const std::map<Svar,Svar>& m);//Dict

    /// Redirect char array to string
    template <size_t N>
    Svar(const char (&t)[N])
        : Svar(std::string(t)) {
    }

    /// Object translation support
    template <class M, typename std::enable_if<
        std::is_constructible<std::string, decltype(std::declval<M>().begin()->first)>::value,
            int>::type = 0>
    Svar(const M & m) : Svar(std::map<std::string,Svar>(m.begin(), m.end())) {}

    /// Dict translation support
    template <class M, typename std::enable_if<
                  !std::is_constructible<std::string, decltype(std::declval<M>().begin()->first)>::value
                  &&std::is_constructible<Svar, decltype(std::declval<M>().begin()->first)>::value,
            int>::type = 0>
    Svar(const M & m) : Svar(std::map<Svar,Svar>(m.begin(), m.end())) {}

    /// Array translation support
    template <class V, typename std::enable_if<
        std::is_constructible<Svar, decltype(*std::declval<V>().begin())>::value,
            int>::type = 0>
    Svar(const V & v) : Svar(std::vector<Svar>(v.begin(), v.end())) {}

    /// Create any other c++ type instance
    template <class T>
    static Svar create(const T & t);

    static Svar Object(const std::map<std::string,Svar>& m={}){return Svar(m);}
    static Svar Array(const std::vector<Svar>& vec={}){return Svar(vec);}
    static Svar Dict(const std::map<Svar,Svar>& m={}){return Svar(m);}

    template <typename T>
    bool is()const;

    template <typename T>
    const T&   as()const;

    template <typename T>
    T& as();

    std::string typeName()const;

    bool isFunction() const{return is<SvarFunction>();}
    bool isClass() const{return is<SvarClass>();}
    bool isException() const{return is<SvarExeption>();}
    bool isObject() const;
    bool isArray()const;
    bool isDict()const;

    bool isNumber() { return is<int>()||is<double>(); }
    bool isNumeric() { return isNumber()||is<bool>(); }

    std::type_index         cpptype()const;
    const Svar&             classObject()const;
    size_t                  length() const;

    const Svar& operator[](size_t i) const;
    const Svar& operator[](const std::string &key) const;

    Svar& Get(const std::string& name);

    template <typename T>
    T& Get(const std::string& name,T def);
    int& GetInt(const std::string& name,int def=0){return Get<int>(name,def);}
    double& GetDouble(const std::string& name,double def=0){return Get<double>(name,def);}
    std::string& GetString(const std::string& name,std::string def=""){return Get<std::string>(name,def);}
    void*& GetPointer(const std::string& name,void* def=nullptr){return Get<void*>(name,def);}

    template <typename T>
    void Set(const std::string& name,const T& def);

    template <typename T, typename std::enable_if<
                  std::is_constructible<Svar,T>::value,int>::type = 0>
    void Set(const T& def);

    template <typename T, typename std::enable_if<
                  !std::is_constructible<Svar,T>::value,int>::type = 0>
    void Set(const T& def);

    Svar operator +(const Svar& rh)const;//__add__
    Svar operator -()const;              //__neg__
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

    static Svar& True();
    static Svar& False();
    static Svar& Null();
    static std::string typeName(std::string name);

    template <typename T>
    static std::string toString(const T& v);

    template <typename T>
    static T fromString(const std::string& str);

    const std::shared_ptr<SvarValue>& value()const{return _obj;}

    std::shared_ptr<SvarValue> _obj;
};

class SvarValue{
public:
    SvarValue(){}
    virtual ~SvarValue(){}
    typedef std::type_index TypeID;
    virtual TypeID          cpptype()const{return typeid(void);}
    virtual const void*     ptr() const{return nullptr;}
    virtual const Svar&     classObject()const{return Svar::Null();}
    virtual bool            equals(const Svar& other) const {return other.value()->ptr()==ptr();}
    virtual bool            less(const Svar& other) const {return other.value()->ptr()<ptr();}
    virtual size_t          length() const {return 0;}
    virtual std::mutex*     accessMutex()const{return nullptr;}

    virtual Svar& operator[](size_t i) ;
    virtual Svar& operator[](const std::string &key) {return Svar::Null();}
};

class SvarExeption: public std::exception{
public:
    SvarExeption(const Svar& wt=Svar())
        :_wt(wt){}

    virtual const char* what() const _GLIBCXX_USE_NOEXCEPT{
        if(_wt.is<std::string>())
            return _wt.as<std::string>().c_str();
        else return "";
    }

    Svar _wt;
};

class SvarFunction: public SvarValue{
public:
    SvarFunction(){}
//    template <typename Func, typename... Args>
//    SvarFunction(Func&& func,
//                 Args&&... args){
////        _args=Svar(std::vector<Svar>({std::forward<Args>(args)...}));
//        _func=std::bind(std::forward<Func>(func),
//                        std::forward<Args>(args)...);
//    }

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

    virtual TypeID          cpptype()const{return typeid(SvarFunction);}
    virtual const void*     ptr() const{return this;}

    template <typename... Args>
    Svar call(Args... args)const{
        std::vector<Svar> argv = {
                (Svar::create(std::move(args)))...
        };
        return _func(argv);
    }

    /// Special internal constructor for functors, lambda functions, methods etc.
    template <typename Func, typename Return, typename... Args, typename... Extra>
    void initialize(Func &&f, Return (*)(Args...), const Extra&... extra)
    {
        std::vector<std::type_index> types={typeid(Args)...};
        std::stringstream signature;signature<<"(";
        for(int i=0;i<types.size();i++)
            signature<<Svar::typeName(types[i].name())<<(i+1==types.size()?")->":",");
        signature<<Svar::typeName(typeid(Return).name());
        doc=signature.str();
        nargs=types.size();
        _func=[&](Svar args)->Svar{
            if(args.length()!=nargs)
                throw SvarExeption("Function "+name+":"
                                   +Svar::typeName(typeid(Return (*)(Args...)).name())
                                   +" expect "+Svar::toString(nargs)
                                   +" arguments but obtained "+Svar::toString(args.length())+".");
            return Svar::create(f(false));
        };
    }

    std::string   name,doc,signature;
    std::uint16_t nargs;
    Svar          scope,next;

    std::function<Svar(Svar)> _func;
};

class SvarClass: public SvarValue{
public:
    std::string  __name__;

    /// buildin functions
    Svar __int__,__float__,__str__;
    Svar __init__,__add__,__sub__,__mul__,__div__,__mod__;
    Svar __xor__,__or__,__and__;
    Svar __le__,__lt__,__ne__,__eq__,__ge__,__gt__;
    Svar __len__,__getitem__,__setitem__,__iter__,__next__;
};

template <typename T>
class SvarClass_
{
public:
    static Svar& Class();
};

template <typename T>
class SvarValue_: public SvarValue{
public:
    SvarValue_(const T& v):_var(v){}

    virtual TypeID          cpptype()const{return typeid(T);}
    virtual const void*     ptr() const{return &_var;}
    virtual const Svar&     classObject()const{return Svar::Null();}
    virtual bool            equals(const Svar& other) const {
        LOG(WARNING)<<"Compare between "<<Svar::typeName(typeid(int).name())
                   <<" and "<<other.typeName()<<" should not happen!";
        return false;
    }
    virtual bool            less(const Svar& other) const {
        LOG(WARNING)<<"Compare between "<<Svar::typeName(typeid(int).name())
                   <<" and "<<other.typeName()<<" should not happen!";
        return false;
    }

    T _var;
};

template <>
class SvarValue_<int>: public SvarValue{
public:
    SvarValue_<int>(const int& v):_var(v){}

    virtual TypeID          cpptype()const{return typeid(int);}
    virtual const void*     ptr() const{return &_var;}
    virtual const Svar&     classObject()const{return Svar::Null();}
    virtual bool            equals(const Svar& other) const {
        if(other.is<int>()) return other.as<int>()==_var;
        if(other.is<double>()) return other.as<double>()==_var;
        return SvarValue::equals(other);
    }
    virtual bool            less(const Svar& other) const {
        if(other.is<int>()) return other.as<int>()<_var;
        if(other.is<double>()) return other.as<double>()<_var;
        return SvarValue::less(other);
    }

    int _var;
};

template <>
class SvarValue_<double>: public SvarValue{
public:
    SvarValue_<double>(const double& v):_var(v){}

    virtual TypeID          cpptype()const{return typeid(double);}
    virtual const void*     ptr() const{return &_var;}
    virtual const Svar&     classObject()const{return Svar::Null();}
    virtual bool            equals(const Svar& other) const {
        if(other.is<int>()) return other.as<int>()==_var;
        if(other.is<double>()) return other.as<double>()==_var;
        return SvarValue::equals(other);
    }
    virtual bool            less(const Svar& other) const {
        if(other.is<int>()) return other.as<int>()<_var;
        if(other.is<double>()) return other.as<double>()<_var;
        return SvarValue::less(other);
    }
    double _var;
};

template <>
class SvarValue_<std::string>: public SvarValue{
public:
    SvarValue_<std::string>(const std::string& v):_var(v){}

    virtual TypeID          cpptype()const{return typeid(std::string);}
    virtual const void*     ptr() const{return &_var;}
    virtual const Svar&     classObject()const{return Svar::Null();}
    virtual bool            equals(const Svar& other) const {
        if(other.is<std::string>()) return other.as<std::string>()==_var;
        return SvarValue::equals(other);
    }
    virtual bool            less(const Svar& other) const {
        if(other.is<std::string>()) return other.as<std::string>()<_var;
        return SvarValue::less(other);
    }

    std::string _var;
};

class SvarObject : public SvarValue_<std::map<std::string,Svar> >{
public:
    SvarObject(const std::map<std::string,Svar>& m)
        : SvarValue_<std::map<std::string,Svar>>(m){}
    virtual size_t          length() const {return _var.size();}
    virtual std::mutex*     accessMutex()const{return &_mutex;}

    virtual Svar& operator[](const std::string &key) {
        std::unique_lock<std::mutex> lock(_mutex);
        auto it=_var.find(key);
        if(it==_var.end()){
            auto ret=_var.insert(std::make_pair(key,Svar()));
            return ret.first->second;
        }
        return it->second;
    }
    mutable std::mutex _mutex;
};

class SvarArray : public SvarValue_<std::vector<Svar> >{
public:
    SvarArray(const std::vector<Svar>& v)
        :SvarValue_<std::vector<Svar>>(v){}
    virtual size_t          length() const {return _var.size();}
    virtual std::mutex*     accessMutex()const{return &_mutex;}
    virtual Svar& operator[](size_t i) {
        std::unique_lock<std::mutex> lock(_mutex);
        return Svar::Null();
    }
    mutable std::mutex _mutex;
};

class SvarDict : public SvarValue_<std::map<Svar,Svar> >{
public:
    SvarDict(const std::map<Svar,Svar>& dict)
        :SvarValue_<std::map<Svar,Svar> >(dict){

    }
    virtual size_t          length() const {return _var.size();}
    virtual std::mutex*     accessMutex()const{return &_mutex;}
    virtual Svar& operator[](const Svar& i) {
        std::unique_lock<std::mutex> lock(_mutex);
        return Svar::Null();
    }
    mutable std::mutex _mutex;
};

template <typename T>
inline std::string Svar::toString(const T& def) {
  std::ostringstream sst;
  sst << def;
  return sst.str();
}

template <>
inline std::string Svar::toString(const std::string& def) {
  return def;
}

template <>
inline std::string Svar::toString(const double& def) {
  using namespace std;
  ostringstream ost;
  ost << setiosflags(ios::fixed) << setprecision(12) << def;
  return ost.str();
}

template <>
inline std::string Svar::toString(const bool& def) {
  return def ? "true" : "false";
}

template <typename T>
inline T Svar::fromString(const std::string& str) {
  std::istringstream sst(str);
  T def;
  try {
    sst >> def;
  } catch (std::exception e) {
    std::cerr << "Failed to read value from " << str << std::endl;
  }
  return def;
}

template <>
inline std::string Svar::fromString(const std::string& str) {
  return str;
}

template <>
inline bool Svar::fromString<bool>(const std::string& str) {
  if (str.empty()) return false;
  if (str == "0") return false;
  if (str == "false") return false;
  return true;
}

template <class T>
inline Svar Svar::create(const T & t)
{
    return (SvarValue*)new SvarValue_<T>(t);
}

inline Svar::Svar(const std::string& m)
    :_obj(std::make_shared<SvarValue_<std::string>>(m)){}

inline Svar::Svar(const std::vector<Svar>& vec)
    :_obj(std::make_shared<SvarArray>(vec)){}

inline Svar::Svar(const std::map<std::string,Svar>& m)
    :_obj(std::make_shared<SvarObject>(m)){}

inline Svar::Svar(const std::map<Svar,Svar>& m)
    :_obj(std::make_shared<SvarDict>(m)){}

template <typename T>
inline bool Svar::is()const{return _obj->cpptype()==typeid(T);}

bool Svar::isObject() const{
    return std::dynamic_pointer_cast<SvarObject>(_obj)!=nullptr;
}

bool Svar::isArray()const{
    return std::dynamic_pointer_cast<SvarArray>(_obj)!=nullptr;
}

bool Svar::isDict()const{
    return std::dynamic_pointer_cast<SvarDict>(_obj)!=nullptr;
}

template <typename T>
inline const T& Svar::as()const{
    assert(is<T>());
    return *(T*)_obj->ptr();
}

template <typename T>
T& Svar::as(){
    assert(is<T>());
    return *(T*)_obj->ptr();
}

inline std::string Svar::typeName()const{
    return typeName(_obj->cpptype().name());
}

inline SvarValue::TypeID Svar::cpptype()const{
    return _obj->cpptype();
}

inline const Svar&       Svar::classObject()const{
    return _obj->classObject();
}

inline size_t            Svar::length() const{
    return _obj->length();
}

inline const Svar& Svar::operator[](size_t i) const{
    return (*_obj)[i];
}

inline const Svar& Svar::operator[](const std::string &key) const{
    return (*_obj)[key];
}

template <typename T>
T& Svar::Get(const std::string& name,T def){
    auto idx = name.find(".");
    if (idx != std::string::npos) {
      return Get(name.substr(0, idx)).Get(name.substr(idx + 1), def);
    }
    Svar& var=Get(name);
    if(var.is<T>()) return var.as<T>();

    if(var.isObject()) {
        return var.Get(typeName(typeid(T).name()),def);
    }

    if(var.is<std::string>());
    var=Svar::create(def);
    return var.as<T>();
}

Svar& Svar::Get(const std::string& name){
    if(is<void>()){
        (*this)=Svar::Object();
    }
    assert(isObject());
    return (*_obj)[name];
}

template <typename T>
inline void Svar::Set(const std::string& name,const T& def){
    auto idx = name.find(".");
    if (idx != std::string::npos) {
      return Get(name.substr(0, idx)).Set(name.substr(idx + 1), def);
    }
    Get(name).Set(def);
}

template <typename T, typename std::enable_if<
              std::is_constructible<Svar,T>::value,int>::type>
void Svar::Set(const T& def)
{
    if(is<T>()) as<T>()=def;
    else (*this)=Svar(def);
}

template <typename T, typename std::enable_if<
              !std::is_constructible<Svar,T>::value,int>::type>
void Svar::Set(const T& def){
    if(is<T>()) as<T>()=def;
    else (*this)=Svar::create(def);
}

bool Svar::operator ==(const Svar& rh)const{
    return _obj->equals(rh);
}

bool Svar::operator <(const Svar& rh)const{
    return _obj->less(rh);
}

inline Svar& Svar::True(){
    static Svar v((SvarValue*)new SvarValue_<bool>(true));
    return v;
}

inline Svar& Svar::False(){
    static Svar v((SvarValue*)new SvarValue_<bool>(false));
    return v;
}

inline Svar& Svar::Null(){
    static Svar v(new SvarValue());
    return v;
}

inline std::string Svar::typeName(std::string name) {
  static std::map<std::string, std::string> decode = {
      {typeid(int32_t).name(), "int32_t"},
      {typeid(int64_t).name(), "int64_t"},
      {typeid(uint32_t).name(), "uint32_t"},
      {typeid(uint64_t).name(), "uint64_t"},
      {typeid(u_char).name(), "u_char"},
      {typeid(char).name(), "char"},
      {typeid(float).name(), "float"},
      {typeid(double).name(), "double"},
      {typeid(std::string).name(), "string"},
      {typeid(bool).name(), "bool"},
  };
  auto it = decode.find(name);
  if (it != decode.end()) return it->second;

  int status;
  char* realname = abi::__cxa_demangle(name.c_str(), 0, 0, &status);
  std::string result(realname);
  free(realname);
  return result;
}

inline Svar& SvarValue::operator[](size_t i) {
    if(classObject().is<void>()) return Svar::Null();
    const SvarClass& cl=classObject().as<SvarClass>();
//    if(cl.__getitem__.isFunction())
//        return cl.__getitem__.as<SvarFunction>().call(i);
    return Svar::Null();
}


}

#endif
