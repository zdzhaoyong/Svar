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
#include <typeinfo>
#include <vector>
#include </data/zhaoyong/Program/Apps/SLAM/GSLAM/GSLAM/core/JSON.h>
#include <GSLAM/core/Glog.h>

namespace Py {

class Svar;
class SvarValue;
class SvarFunction;
class SvarClass;
class SvarObject;
class SvarArray;
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

//    template <class T, typename std::enable_if<!std::is_constructible<Svar, T>::value,
//                  int>::type = 0>
//    Svar(const T & t);

    /// Create any other c++ type instance
    template <class T>
    static Svar create(const T & t);

    template <typename T>
    bool is()const;

    template <typename T>
    const T&   as()const;

    std::string typeName()const;

    bool isFunction() const{return is<SvarFunction>();}
    bool isClass() const{return is<SvarClass>();}
    bool isObject() const;
    bool isArray()const;
    bool isDict()const;

    bool isNumber() { return is<int>()||is<double>(); }
    bool isNumeric() { return isNumber()||is<bool>(); }

    const std::type_info*   cpptype()const;
    const Svar&             classObject()const;
    size_t                  length() const;

    const Svar& operator[](size_t i) const;
    const Svar& operator[](const std::string &key) const;

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

    const std::shared_ptr<SvarValue>& value()const{return _obj;}

    std::shared_ptr<SvarValue> _obj;
};

class SvarValue{
public:
    SvarValue(){}
    virtual ~SvarValue(){}
    typedef const std::type_info* TypeID;
    virtual TypeID          cpptype()const{return &typeid(void);}
    virtual const void*     ptr() const{return nullptr;}
    virtual const Svar&     classObject()const{return Svar::Null();}
    virtual bool            equals(const Svar& other) const {return other.value()->ptr()==ptr();}
    virtual bool            less(const Svar& other) const {return false;}
    virtual size_t          length() const {return 0;}
    virtual std::mutex*     accessMutex()const{return nullptr;}

    virtual const Svar& operator[](size_t i) const{return Svar::Null();}
    virtual const Svar& operator[](const std::string &key) const{return Svar::Null();}
};

class SvarFunction: public SvarValue{
    template <typename Func, typename... Args>
    SvarFunction(Func&& func,
                 Args&&... args){
        _func=std::bind(std::forward<Func>(func),
                        std::forward<Args>(args)...);
    }

    virtual TypeID          cpptype()const{return &typeid(SvarFunction);}
    virtual const void*     ptr() const{return this;}

    Svar call(Svar args)const{
//        assert(args.length()==_args.length());
        return _func(args);
    }

    std::function<Svar(Svar)> _func;
    Svar _args,_return,_intro;
};

class SvarClass: public SvarValue{
public:
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

    virtual TypeID          cpptype()const{return &typeid(T);}
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

    virtual TypeID          cpptype()const{return &typeid(int);}
    virtual const void*     ptr() const{return &_var;}
    virtual const Svar&     classObject()const{return Svar::Null();}
    virtual bool            equals(const Svar& other) const {
        if(other.is<int>()) return other.as<int>()==_var;
        if(other.is<double>()) return other.as<double>()==_var;
        LOG(WARNING)<<"Compare between "<<Svar::typeName(typeid(int).name())
                   <<" and "<<other.typeName()<<" should not happen!";
        return false;
    }
    virtual bool            less(const Svar& other) const {return false;}

    int _var;
};

template <>
class SvarValue_<double>: public SvarValue{
public:
    SvarValue_<double>(const double& v):_var(v){}

    virtual TypeID          cpptype()const{return &typeid(double);}
    virtual const void*     ptr() const{return &_var;}
    virtual const Svar&     classObject()const{return Svar::Null();}
    virtual bool            equals(const Svar& other) const {return other.is<void>();}
    virtual bool            less(const Svar& other) const {return false;}

    double _var;
};

template <>
class SvarValue_<std::string>: public SvarValue{
public:
    SvarValue_<std::string>(const std::string& v):_var(v){}

    virtual TypeID          cpptype()const{return &typeid(std::string);}
    virtual const void*     ptr() const{return &_var;}
    virtual const Svar&     classObject()const{return Svar::Null();}
    virtual bool            equals(const Svar& other) const {return other.is<void>();}
    virtual bool            less(const Svar& other) const {return false;}

    std::string _var;
};


class SvarObject : public SvarValue_<std::map<std::string,Svar> >{
public:
    SvarObject(const std::map<std::string,Svar>& m)
        : SvarValue_<std::map<std::string,Svar>>(m){}
    virtual size_t          length() const {return _var.size();}
    virtual std::mutex*     accessMutex()const{return &_mutex;}

    virtual const Svar& operator[](const std::string &key) const{
        std::unique_lock<std::mutex> lock(_mutex);
        auto it=_var.find(key);
        if(it==_var.end())
            return Svar::Null();
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
    virtual const Svar& operator[](size_t i) const{
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
    virtual const Svar& operator[](const Svar& i) const{
        std::unique_lock<std::mutex> lock(_mutex);
        return Svar::Null();
    }
    mutable std::mutex _mutex;
};

namespace streamable_check {
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
inline bool Svar::is()const{return typeid(T)==*_obj->cpptype();}

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

inline std::string Svar::typeName()const{
    return typeName(_obj->cpptype()->name());
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


}

#endif
