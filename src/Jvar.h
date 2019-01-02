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

template <bool B> using bool_constant = std::integral_constant<bool, B>;
template <typename T> struct negation : bool_constant<!T::value> { };
template <bool...> struct bools {};
template <class... Ts> using all_of = std::is_same<
    bools<Ts::value..., true>,
    bools<true, Ts::value...>>;
template <class... Ts> using any_of = negation<all_of<negation<Ts>...>>;
template <class... Ts> using none_of = negation<any_of<Ts...>>;

template <class T, template<class> class... Predicates> using satisfies_all_of = all_of<Predicates<T>...>;
template <class T, template<class> class... Predicates> using satisfies_any_of = any_of<Predicates<T>...>;
template <class T, template<class> class... Predicates> using satisfies_none_of = none_of<Predicates<T>...>;
template <typename T> using is_lambda = satisfies_none_of<remove_reference_t<T>,
        std::is_function, std::is_pointer, std::is_member_pointer>;

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
class SvarValue_;

class Svar{
public:
    /// Basic buildin types
    Svar():Svar(Undefined()){}
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

    /// Construct a cpp_function from a vanilla function pointer
    template <typename Return, typename... Args, typename... Extra>
    Svar(Return (*f)(Args...), const Extra&... extra);

    /// Construct a cpp_function from a lambda function (possibly with internal state)
    template <typename Func, typename... Extra>
    static Svar lambda(Func &&f, const Extra&... extra);

    /// Construct a cpp_function from a class method (non-const)
    template <typename Return, typename Class, typename... Arg, typename... Extra>
    Svar(Return (Class::*f)(Arg...), const Extra&... extra);

    /// Construct a cpp_function from a class method (const)
    template <typename Return, typename Class, typename... Arg, typename... Extra>
    Svar(Return (Class::*f)(Arg...) const, const Extra&... extra);

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

    template <typename T>
    Svar cast()const;

    template <typename T>
    detail::enable_if_t<std::is_pointer<T>::value,T>
    castAs();

    template <typename T>
    detail::enable_if_t<std::is_reference<T>::value,const T&>
    castAs();

    template <typename T>
    detail::enable_if_t<!std::is_reference<T>::value&&!std::is_pointer<T>::value,const T&>
    castAs()const;

    std::string typeName()const;

    bool isUndefined()const{return is<void>();}
    bool isNull()const;
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
    SvarClass*              classPtr()const;
    size_t                  length() const;

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

    template <typename... Args>
    Svar call(Args... args)const;

    template <typename... Args>
    Svar operator()(Args... args)const{
        return call(args...);
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
    Svar operator[](const Svar& i) const;//__getitem__

    friend std::ostream& operator <<(std::ostream& ost,const Svar& self);

    static Svar& Undefined();
    static Svar& Null();
    static Svar& True();
    static Svar& False();
    static Svar& instance();
    static std::string typeName(std::string name);

    template <typename T>
    static std::string type_id(){
        return typeName(typeid(T).name());
    }

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
    virtual const Svar&     classObject()const{return Svar::Undefined();}
    virtual bool            equals(const Svar& other) const {return other.value()->ptr()==ptr();}
    virtual bool            less(const Svar& other) const {return other.value()->ptr()<ptr();}
    virtual size_t          length() const {return 0;}
    virtual std::mutex*     accessMutex()const{return nullptr;}

    virtual Svar& operator[](size_t ) {return Svar::Undefined();}
    virtual Svar& operator[](const std::string &) {return Svar::Undefined();}
};

class SvarExeption: public std::exception{
public:
    SvarExeption(const Svar& wt=Svar())
        :_wt(wt){}

    virtual const char* what() const _GLIBCXX_USE_NOEXCEPT{
        if(_wt.is<std::string>())
            return _wt.as<std::string>().c_str();
        else
            return "SvarException";
    }

    Svar _wt;
};

class SvarIterEnd: public std::exception{
public:
    virtual const char* what() const _GLIBCXX_USE_NOEXCEPT{
        return "SvarIterEnd";
    }
};

class SvarFunction: public SvarValue{
public:
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
    virtual const Svar&     classObject()const;

    template <typename... Args>
    Svar call(Args... args)const{
        std::vector<Svar> argv = {
                (Svar::create(std::move(args)))...
        };

        const SvarFunction* overload=this;
        std::vector<SvarExeption> catches;
        for(;true;overload=&overload->next.as<SvarFunction>()){
            if(overload->nargs!=argv.size())
            {
                if(!overload->next.isFunction()) {
                    overload=nullptr;break;
                }
                continue;
            }
            try{
                return _func(argv);
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
            stream<<"Failed to call method "<<name<<" with imput arguments: [";
            for(auto it=argv.begin();it!=argv.end();it++)
            {
                stream<<(it==argv.begin()?"":",")<<it->typeName();
            }
            stream<<"]\n"<<"Overload candidates:\n"<<(*this)<<std::endl;
            for(auto it:catches) stream<<it.what()<<std::endl;
            throw SvarExeption(stream.str());
        }

        return Svar::Undefined();
    }

    /// Special internal constructor for functors, lambda functions, methods etc.
    template <typename Func, typename Return, typename... Args, typename... Extra>
    void initialize(Func &&f, Return (*)(Args...), const Extra&... extra)
    {
        std::vector<std::type_index> types={typeid(Args)...};
        std::stringstream signature;signature<<"(";
        for(size_t i=0;i<types.size();i++)
            signature<<Svar::typeName(types[i].name())<<(i+1==types.size()?")->":",");
        signature<<Svar::typeName(typeid(Return).name());
        this->signature=signature.str();
        nargs=types.size();
        _func=[this,f](std::vector<Svar>& args)->Svar{
            using indices = detail::make_index_sequence<sizeof...(Args)>;
            return call_impl(f,(Return (*) (Args...)) nullptr,args,indices{});
        };
    }

    template <typename Func, typename Return, typename... Args,size_t... Is>
    detail::enable_if_t<std::is_void<Return>::value, Svar>
    call_impl(Func&& f,Return (*)(Args...),std::vector<Svar>& args,detail::index_sequence<Is...>){
        f(args[Is].castAs<Args>()...);
        return Svar::Undefined();
    }

    template <typename Func, typename Return, typename... Args,size_t... Is>
    detail::enable_if_t<!std::is_void<Return>::value, Svar>
    call_impl(Func&& f,Return (*)(Args...),std::vector<Svar>& args,detail::index_sequence<Is...>){
        return Svar::create<Return>(f(args[Is].castAs<Args>()...));
    }

    friend std::ostream& operator<<(std::ostream& sst,const SvarFunction& self){
        sst<<self.name<<"(...)\n    ";
        const SvarFunction* overload=&self;
        while(overload){
            sst<<overload->name<<overload->signature;
            if(!overload->next.isFunction()) {
                overload=nullptr;break;
            }
            overload=&overload->next.as<SvarFunction>();
        }
        return sst;
    }

    std::string   name="function",signature;
    std::uint16_t nargs;
    Svar          stack,next;
    bool          is_method;

    std::function<Svar(std::vector<Svar>&)> _func;
};

class SvarClass: public SvarValue{
public:
    std::string  __name__;
    std::type_index _cpptype;
    SvarClass(const std::string& name,std::type_index cpp_type)
        : __name__(name),_cpptype(cpp_type){}

    virtual TypeID          cpptype()const{return typeid(SvarClass);}
    virtual const void*     ptr() const{return this;}

    SvarClass& def(const std::string& name,const Svar& function,bool isMethod=true)
    {
        assert(function.isFunction());
        Svar& dest=_methods.Get(name);
        while(dest.isFunction()) dest=dest.as<SvarFunction>().next;
        dest=function;
        dest.as<SvarFunction>().is_method=isMethod;
        dest.as<SvarFunction>().name=__name__+"."+name;

        if(__init__.is<void>()&&name=="__init__") __init__=function;
        if(__int__.is<void>()&&name=="__int__") __int__=function;
        if(__double__.is<void>()&&name=="__double__") __double__=function;
        if(__str__.is<void>()&&name=="__str__") __str__=function;
        if(__neg__.is<void>()&&name=="__neg__") __neg__=function;
        if(__add__.is<void>()&&name=="__add__") __add__=function;
        if(__sub__.is<void>()&&name=="__sub__") __sub__=function;
        if(__mul__.is<void>()&&name=="__mul__") __mul__=function;
        if(__div__.is<void>()&&name=="__div__") __div__=function;
        if(__mod__.is<void>()&&name=="__mod__") __mod__=function;
        if(__xor__.is<void>()&&name=="__xor__") __xor__=function;
        if(__or__.is<void>()&&name=="__or__") __or__=function;
        if(__and__.is<void>()&&name=="__and__") __and__=function;
        if(__le__.is<void>()&&name=="__le__") __le__=function;
        if(__lt__.is<void>()&&name=="__lt__") __lt__=function;
        if(__ne__.is<void>()&&name=="__ne__") __ne__=function;
        if(__eq__.is<void>()&&name=="__eq__") __eq__=function;
        if(__ge__.is<void>()&&name=="__ge__") __ge__=function;
        if(__gt__.is<void>()&&name=="__gt__") __gt__=function;
        if(__len__.is<void>()&&name=="__len__") __len__=function;
        if(__getitem__.is<void>()&&name=="__getitem__") __getitem__=function;
        if(__setitem__.is<void>()&&name=="__setitem__") __setitem__=function;
        if(__iter__.is<void>()&&name=="__iter__") __iter__=function;
        if(__next__.is<void>()&&name=="__next__") __next__=function;
        return *this;
    }

    SvarClass& def_static(const std::string& name,const Svar& function)
    {
        return def(name,function,false);
    }

    template <typename Func>
    SvarClass& def(const std::string& name,Func &&f){
        return def(name,Svar::lambda(f),true);
    }

    /// Construct a cpp_function from a lambda function (possibly with internal state)
    template <typename Func>
    SvarClass& def_static(const std::string& name,Func &&f){
        return def(name,Svar::lambda(f),false);
    }


    template <typename T>
    static Svar& instance();

    template <typename T>
    static SvarClass& Class(){
        Svar& inst=instance<T>();
        return inst.as<SvarClass>();
    }

    friend std::ostream& operator<<(std::ostream& ost,const SvarClass& rh);

    /// buildin functions
    Svar __int__,__double__,__str__;
    Svar __init__,__neg__,__add__,__sub__,__mul__,__div__,__mod__;
    Svar __xor__,__or__,__and__;
    Svar __le__,__lt__,__ne__,__eq__,__ge__,__gt__;
    Svar __len__,__getitem__,__setitem__,__iter__,__next__;
    Svar _methods,_getters,_setters,_doc;
    std::vector<Svar> _parents;
};


template <typename T>
Svar& SvarClass::instance()
{
    static Svar cl;
    if(cl.isClass()) return cl;
    cl=(SvarValue*)new SvarClass(Svar::type_id<T>(),typeid(T));
    return cl;
}

template <typename T>
class SvarValue_: public SvarValue{
public:
    SvarValue_(const T& v):_var(v){}

    virtual TypeID          cpptype()const{return typeid(T);}
    virtual const void*     ptr() const{return &_var;}
    virtual const Svar&     classObject()const{return SvarClass::instance<T>();}
    virtual bool            equals(const Svar& other) const {
        const Svar& clsobj=classObject();
        if(!clsobj.isClass()) return SvarValue::equals(other);
        Svar eq_func=clsobj.as<SvarClass>().__eq__;
        if(!eq_func.isFunction()) return SvarValue::equals(other);
        Svar ret=eq_func.call(_var,other);
        assert(ret.is<bool>());
        return ret.as<bool>();
    }
    virtual bool            less(const Svar& other) const {
        const Svar& clsobj=classObject();
        if(!clsobj.isClass()) return SvarValue::less(other);
        Svar lt_func=clsobj.as<SvarClass>().__lt__;
        if(!lt_func.isFunction()) return SvarValue::less(other);
        Svar ret=lt_func.call(_var,other);
        assert(ret.is<bool>());
        return ret.as<bool>();
    }

    T _var;
};

class SvarObject : public SvarValue_<std::map<std::string,Svar> >{
public:
    SvarObject(const std::map<std::string,Svar>& m)
        : SvarValue_<std::map<std::string,Svar>>(m){}
    virtual TypeID          cpptype()const{return typeid(SvarObject);}
    virtual const void*     ptr() const{return this;}
    virtual size_t          length() const {return _var.size();}
    virtual std::mutex*     accessMutex()const{return &_mutex;}
    virtual const Svar&     classObject()const{
        if(_class.isClass()) return _class;
        return SvarClass::instance<SvarObject>();
    }

    virtual Svar& operator[](const std::string &key) {
        std::unique_lock<std::mutex> lock(_mutex);
        auto it=_var.find(key);
        if(it==_var.end()){
            auto ret=_var.insert(std::make_pair(key,Svar()));
            return ret.first->second;
        }
        return it->second;
    }

    friend std::ostream& operator<<(std::ostream& ost,const SvarObject& rh){
        std::unique_lock<std::mutex> lock(rh._mutex);
        ost<<"{";
        for(auto it=rh._var.begin();it!=rh._var.end();it++)
        {
            ost<<(it==rh._var.begin()?"":",\n")<<Svar(it->first)<<" : "<<it->second;
        }
        ost<<"}";
        return ost;
    }
    mutable std::mutex _mutex;
    Svar _class;
};

class SvarArray : public SvarValue_<std::vector<Svar> >{
public:
    SvarArray(const std::vector<Svar>& v)
        :SvarValue_<std::vector<Svar>>(v){}
    virtual TypeID          cpptype()const{return typeid(SvarArray);}
    virtual const void*     ptr() const{return this;}
    virtual const Svar&     classObject()const{return SvarClass::instance<SvarArray>();}
    virtual size_t          length() const {return _var.size();}
    virtual std::mutex*     accessMutex()const{return &_mutex;}
    virtual Svar& operator[](size_t i) {
        std::unique_lock<std::mutex> lock(_mutex);
        if(i<_var.size()) return _var[i];
        return Svar::Undefined();
    }

    friend std::ostream& operator<<(std::ostream& ost,const SvarArray& rh){
        std::unique_lock<std::mutex> lock(rh._mutex);
        ost<<"[";
        for(size_t i=0;i<rh._var.size();++i)
            ost<<rh._var[i]<<(i+1==rh._var.size()?"":",");
        ost<<"]";
        return ost;
    }

    Svar operator+(Svar other){
        std::unique_lock<std::mutex> lock(_mutex);
        std::vector<Svar> ret=_var;
        if(other.isArray()){
            SvarArray& rh=other.as<SvarArray>();
            std::unique_lock<std::mutex> lock(rh._mutex);
            ret.insert(ret.end(),rh._var.begin(),rh._var.end());
        }
        else ret.push_back(other);
        return Svar::Array(ret);
    }

    void append(Svar other){
        std::unique_lock<std::mutex> lock(_mutex);
        _var.push_back(other);
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
        auto it=_var.find(i);
        if(it!=_var.end()) return it->second;
        return Svar::Undefined();
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

template <typename Return, typename... Args, typename... Extra>
Svar::Svar(Return (*f)(Args...), const Extra&... extra)
    :_obj(std::make_shared<SvarFunction>(f,extra...)){}

/// Construct a cpp_function from a lambda function (possibly with internal state)
template <typename Func, typename... Extra>
Svar Svar::lambda(Func &&f, const Extra&... extra)
{
    return Svar(((SvarValue*)new SvarFunction(f,extra...)));
}

/// Construct a cpp_function from a class method (non-const)
template <typename Return, typename Class, typename... Arg, typename... Extra>
Svar::Svar(Return (Class::*f)(Arg...), const Extra&... extra)
    :_obj(std::make_shared<SvarFunction>(f,extra...)){}

/// Construct a cpp_function from a class method (const)
template <typename Return, typename Class, typename... Arg, typename... Extra>
Svar::Svar(Return (Class::*f)(Arg...) const, const Extra&... extra)
    :_obj(std::make_shared<SvarFunction>(f,extra...)){}

template <class T>
inline Svar Svar::create(const T & t)
{
    return (SvarValue*)new SvarValue_<T>(t);
}

template <>
inline Svar Svar::create(const Svar & t)
{
    return t;
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


bool Svar::isNull()const{
    return Null()==(*this);
}

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

template <>
inline const Svar& Svar::as<Svar>()const{
    return *this;
}

template <typename T>
T& Svar::as(){
    return *(T*)_obj->ptr();
}

template <typename T>
Svar Svar::cast()const{
    if(is<T>()) return (*this);

    Svar cl=_obj->classObject();
    if(cl.isClass()){
        SvarClass& srcClass=cl.as<SvarClass>();
        Svar cvt=srcClass._methods["__"+type_id<T>()+"__"];
        if(cvt.isFunction()){
            Svar ret=cvt.call(*this);
            if(ret.is<T>()) return ret;
        }
    }

    SvarClass& destClass=SvarClass::instance<T>().as<SvarClass>();
    if(destClass.__init__.isFunction()){
        Svar ret=destClass.__init__.call(*this);
        if(ret.is<T>()) return ret;
    }

    return Undefined();
}

template <typename T>
detail::enable_if_t<!std::is_reference<T>::value&&!std::is_pointer<T>::value,const T&>
Svar::castAs()const
{
    auto ret=cast<T>();
    if(!ret.is<T>())
        throw SvarExeption("Unable cast "+typeName()+" to "+type_id<T>());
    return ret.as<T>();// let compiler happy
}

template <typename T>
detail::enable_if_t<std::is_reference<T>::value,const T&>
Svar::castAs(){
    if(!is<typename std::remove_reference<T>::type>())
        throw SvarExeption("Unable cast "+typeName()+" to "+type_id<T>());

    auto& ref= as<typename std::remove_reference<T>::type>();
    return ref;
}

template <typename T>
detail::enable_if_t<std::is_pointer<T>::value,T>
Svar::castAs(){
    // nullptr
    if(is<void>()) return (T)nullptr;
    // T* -> T*
    if(is<T>()) return as<T>();

    // T* -> T const*
    if(is<typename std::remove_const<typename std::remove_pointer<T>::type>::type*>())
        return as<typename std::remove_const<typename std::remove_pointer<T>::type>::type*>();

    // T -> T*
    if(is<typename std::remove_pointer<T>::type>())
        return &as<typename std::remove_pointer<T>::type>();

    throw SvarExeption("Unable cast "+typeName()+" to "+type_id<T>());

    return as<T>();
}

template <>
const Svar& Svar::castAs<Svar>()const{
    return *this;
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

SvarClass*   Svar::classPtr()const
{
    auto clsObj=classObject();
    if(clsObj.isClass()) return &clsObj.as<SvarClass>();
    return nullptr;
}

inline size_t            Svar::length() const{
    return _obj->length();
}

Svar Svar::operator[](const Svar& i) const{
    if(isObject()) return (*std::dynamic_pointer_cast<SvarObject>(_obj))[i.castAs<std::string>()];
    if(classObject().is<void>()) return Undefined();
    const SvarClass& cl=classObject().as<SvarClass>();
    if(cl.__getitem__.isFunction()){
        return cl.__getitem__.call((*this),i);
    }
    return Undefined();
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


template <typename... Args>
Svar Svar::call(Args... args)const
{
    return as<SvarFunction>().call(args...);
}


Svar Svar::operator -()const
{
    auto cls=classPtr();
    if(!cls) return Undefined();
    if(!cls->__neg__.isFunction()) return Undefined();
    return cls->__neg__.call((*this));
}

Svar Svar::operator +(const Svar& rh)const
{
    auto cls=classPtr();
    if(!cls) return Undefined();
    if(!cls->__add__.isFunction()) return Undefined();
    return cls->__add__.call((*this),rh);
}

Svar Svar::operator -(const Svar& rh)const{
    auto cls=classPtr();
    if(!cls) return Undefined();
    if(!cls->__sub__.isFunction()) return Undefined();
    return cls->__sub__.call((*this),rh);
}

Svar Svar::operator *(const Svar& rh)const
{
    auto cls=classPtr();
    if(!cls) return Undefined();
    if(!cls->__mul__.isFunction()) return Undefined();
    return cls->__mul__.call((*this),rh);
}

Svar Svar::operator /(const Svar& rh)const
{
    auto cls=classPtr();
    if(!cls) return Undefined();
    if(!cls->__div__.isFunction()) return Undefined();
    return cls->__div__.call((*this),rh);
}

Svar Svar::operator %(const Svar& rh)const{
    auto cls=classPtr();
    if(!cls) return Undefined();
    if(!cls->__mod__.isFunction()) return Undefined();
    return cls->__mod__.call((*this),rh);
}

Svar Svar::operator ^(const Svar& rh)const{

    auto cls=classPtr();
    if(!cls) return Undefined();
    if(!cls->__xor__.isFunction()) return Undefined();
    return cls->__xor__.call((*this),rh);
}

Svar Svar::operator |(const Svar& rh)const
{
    auto cls=classPtr();
    if(!cls) return Undefined();
    if(!cls->__or__.isFunction()) return Undefined();
    return cls->__or__.call((*this),rh);
}

Svar Svar::operator &(const Svar& rh)const{
    auto cls=classPtr();
    if(!cls) return Undefined();
    if(!cls->__and__.isFunction()) return Undefined();
    return cls->__and__.call((*this),rh);
}

bool Svar::operator ==(const Svar& rh)const{
    return _obj->equals(rh);
}


std::ostream& operator <<(std::ostream& ost,const Svar& self)
{
    if(self.isUndefined()) {
        ost<<"Undefined";
        return ost;
    }
    if(self.isNull()){
        ost<<"Null";
        return ost;
    }
    auto cls=self.classPtr();
    if(cls&&cls->__str__.isFunction()){
        ost<<cls->__str__.call(self).as<std::string>();
        return ost;
    }
    ost<<"["<<self.typeName()<<": at "<<self.value()->ptr()<<"]";
    return ost;
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

inline Svar& Svar::Undefined(){
    static Svar v(new SvarValue());
    return v;
}

inline Svar& Svar::Null()
{
    static Svar v=create<void*>(nullptr);
    return v;
}

inline Svar& Svar::instance(){
    static Svar v=Svar::Object();
    return v;
}

inline std::string Svar::typeName(std::string name) {
  static std::map<std::string, std::string> decode = {
      {typeid(int32_t).name(), "int"},
      {typeid(int64_t).name(), "int64_t"},
      {typeid(uint32_t).name(), "uint32_t"},
      {typeid(uint64_t).name(), "uint64_t"},
      {typeid(u_char).name(), "u_char"},
      {typeid(char).name(), "char"},
      {typeid(float).name(), "float"},
      {typeid(double).name(), "double"},
      {typeid(std::string).name(), "str"},
      {typeid(bool).name(), "bool"},
      {typeid(SvarDict).name(), "dict"},
//      {typeid(SvarObject).name(), "object"},
      {typeid(SvarArray).name(), "array"},
      {typeid(SvarClass).name(), "class"},
      {typeid(Svar).name(), "object"},
  };
  auto it = decode.find(name);
  if (it != decode.end()) return it->second;

  int status;
  char* realname = abi::__cxa_demangle(name.c_str(), 0, 0, &status);
  std::string result(realname);
  free(realname);
  return result;
}

inline const Svar&  SvarFunction::classObject()const{return SvarClass::instance<SvarFunction>();}

std::ostream& operator<<(std::ostream& ost,const SvarClass& rh){
    ost<<"class "<<rh.__name__<<"():\n";
    std::stringstream  content;
    if(rh.__init__.isFunction()) content<<rh.__init__<<std::endl<<std::endl;
    if(rh._doc.is<std::string>()) content<<rh._doc.as<std::string>()<<std::endl;
    if(rh._methods.isObject()&&rh._methods.length()){
        content<<"Methods defined here:\n";
        const SvarObject& methods=rh._methods.as<SvarObject>();
        std::unique_lock<std::mutex> lock(methods._mutex);
        for(std::pair<std::string,Svar> it:methods._var){
            content<<it.second<<std::endl<<std::endl;
        }
    }
    std::string line;
    while(std::getline(content,line)){ost<<"|  "<<line<<std::endl;}
    return ost;
}

class SvarBuildin{
public:
    SvarBuildin(){
        SvarClass::Class<void>()
                .def("__str__",[](Svar){return "Null";});
        SvarClass::Class<int>()
                .def("__init__",&SvarBuildin::int_create)
                .def("__double__",&SvarBuildin::int_to_double)
                .def("__bool__",[](int i){return (bool)i;})
                .def("__str__",[](int i){return Svar::toString(i);})
                .def("__eq__",&SvarBuildin::int_equal)
                .def("__lt__",&SvarBuildin::int_less)
                .def("__add__",[](int self,Svar rh){
            if(rh.is<int>()) return Svar(self+rh.as<int>());
            if(rh.is<double>()) return Svar(self+rh.as<double>());
            throw SvarExeption("Failed to __add__(self:int,arg0:"+rh.typeName()+")");
            return Svar::Undefined();
        })
                .def("__sub__",[](int self,Svar rh){
            if(rh.is<int>()) return Svar(self-rh.as<int>());
            if(rh.is<double>()) return Svar(self-rh.as<double>());
            throw SvarExeption("Failed to __sub__(self:int,arg0:"+rh.typeName()+")");
            return Svar::Undefined();
        })
                .def("__mul__",[](int self,Svar rh){
            if(rh.is<int>()) return Svar(self*rh.as<int>());
            if(rh.is<double>()) return Svar(self*rh.as<double>());
            throw SvarExeption("Failed to __mul__(self:int,arg0:"+rh.typeName()+")");
            return Svar::Undefined();
        })
                .def("__div__",[](int self,Svar rh){
            if(rh.is<int>()) return Svar(self/rh.as<int>());
            if(rh.is<double>()) return Svar(self/rh.as<double>());
            throw SvarExeption("Failed to __add__(self:int,arg0:"+rh.typeName()+")");
            return Svar::Undefined();
        })
                .def("__mod__",[](int self,int rh){
            return self%rh;
        })
                .def("__neg__",[](int self){return -self;})
                .def("__xor__",[](int self,int rh){return self^rh;})
                .def("__or__",[](int self,int rh){return self|rh;})
                .def("__and__",[](int self,int rh){return self&rh;});

        SvarClass::Class<bool>()
                .def("__int__",[](bool b){return (int)b;})
                .def("__double__",[](bool b){return (double)b;})
                .def("__str__",[](bool b){return Svar::toString(b);});

        SvarClass::Class<double>()
                .def("__int__",[](double d){return (int)d;})
                .def("__bool__",[](double d){return (bool)d;})
                .def("__str__",[](double d){return Svar::toString(d);})
                .def("__eq__",[](double self,double rh){return self==rh;})
                .def("__lt__",[](double self,double rh){return self<rh;})
                .def("__neg__",[](double self){return -self;})
                .def("__add__",[](double self,double rh){return self+rh;})
                .def("__sub__",[](double self,double rh){return self-rh;})
                .def("__mul__",[](double self,double rh){return self*rh;})
                .def("__div__",[](double self,double rh){return self/rh;})
                .def("__invert__",[](double self){return 1./self;});

        SvarClass::Class<std::string>()
                .def("__len__",[](const Svar& self){return self.as<std::string>().size();})
        .def("__str__",[](Svar self){
            std::string out;
            dump(self.as<std::string>(),out);
            return out;}
        ).def("__add__",[](const std::string& self,std::string rh){return self+rh;});

        SvarClass::Class<char const*>()
                .def("__str__",[](char const* str){return std::string(str);});

        SvarClass::Class<SvarArray>()
                .def("__getitem__",[](Svar self,int i){
            SvarArray& array=self.as<SvarArray>();
            return array[i];
        })
        .def("__str__",[](Svar self){return Svar::toString(self.as<SvarArray>());})
        .def("__iter__",[](Svar self){
            static Svar arrayinteratorClass;
            if(!arrayinteratorClass.isClass()){
                SvarClass* cls=new SvarClass("arrayiterator",typeid(SvarObject));
                arrayinteratorClass=(SvarValue*)cls;
                cls->def("next",[](Svar iter){
                    Svar arrObj=iter.Get("array");
                    SvarArray& array=arrObj.as<SvarArray>();
                    int& index=iter.Get<int>("index",0);
                    if(index<(int)array.length()){
                        return array[index++];
                    }
                    throw SvarIterEnd();
                    return Svar();
                });
            }
            Svar iter=Svar::Object({{"class",arrayinteratorClass},
                                   {"array",self},
                                   {"index",0}});
            std::cerr<<"iter:"<<iter;
            iter.as<SvarObject>()._class=arrayinteratorClass;
            return iter;
        })
        .def("__add__",[](Svar self,Svar other){
            return self.as<SvarArray>()+other;
        })
        .def("append",&SvarArray::append);

        SvarClass::Class<SvarDict>()
                .def("__getitem__",[](Svar self,Svar key){
            return self.as<SvarDict>()[key];
        });


        SvarClass::Class<SvarObject>()
                .def("__getitem__",[](Svar self,Svar key){
            return self.as<SvarObject>()[key.castAs<std::string>()];
        })
        .def("__str__",[](Svar self){return Svar::toString(self.as<SvarObject>());});

        SvarClass::Class<SvarFunction>()
                .def("__str__",[](Svar self){return Svar::toString(self.as<SvarFunction>());});

        SvarClass::Class<SvarClass>()
                .def("__str__",[](Svar self){return Svar::toString(self.as<SvarClass>());});

        Svar::instance().Set("__builtin__.int",SvarClass::instance<int>());
        Svar::instance().Set("__builtin__.double",SvarClass::instance<double>());
        Svar::instance().Set("__builtin__.int",SvarClass::instance<bool>());
        Svar::instance().Set("__builtin__.str",SvarClass::instance<std::string>());
        Svar::instance().Set("__builtin__.dict",SvarClass::instance<SvarDict>());
        Svar::instance().Set("__builtin__.object",SvarClass::instance<SvarObject>());
        Svar::instance().Set("__builtin__.array",SvarClass::instance<SvarArray>());
    }

    static int int_create(Svar rh){
        if(rh.is<int>()) return rh.as<int>();
        if(rh.is<std::string>()) return Svar::fromString<int>(rh.as<std::string>());
        if(rh.is<double>()) return (int)rh.as<double>();
        if(rh.is<bool>()) return (int)rh.as<bool>();

        throw SvarExeption("Can't construct int from "+rh.typeName()+".");
        return rh.as<int>();
    }

    static double int_to_double(int i){return (double)i;}
    static bool int_equal(int i,Svar v){return v.castAs<int>()==i;}
    static bool int_less(int i,Svar v){return i<v.castAs<int>();}
    static void dump(const std::string &value, std::string &out) {
        out += '"';
        for (size_t i = 0; i < value.length(); i++) {
            const char ch = value[i];
            if (ch == '\\') {
                out += "\\\\";
            } else if (ch == '"') {
                out += "\\\"";
            } else if (ch == '\b') {
                out += "\\b";
            } else if (ch == '\f') {
                out += "\\f";
            } else if (ch == '\n') {
                out += "\\n";
            } else if (ch == '\r') {
                out += "\\r";
            } else if (ch == '\t') {
                out += "\\t";
            } else if (static_cast<uint8_t>(ch) <= 0x1f) {
                char buf[8];
                snprintf(buf, sizeof buf, "\\u%04x", ch);
                out += buf;
            } else if (static_cast<uint8_t>(ch) == 0xe2 && static_cast<uint8_t>(value[i+1]) == 0x80
                       && static_cast<uint8_t>(value[i+2]) == 0xa8) {
                out += "\\u2028";
                i += 2;
            } else if (static_cast<uint8_t>(ch) == 0xe2 && static_cast<uint8_t>(value[i+1]) == 0x80
                       && static_cast<uint8_t>(value[i+2]) == 0xa9) {
                out += "\\u2029";
                i += 2;
            } else {
                out += ch;
            }
        }
        out += '"';
    }

}SvarBuildinInitializerinstance;


}

#endif
