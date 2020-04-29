#include <Svar.h>
#include <sstream>
#include <algorithm> // transform
#include <array> // array

#include "Timer.h"

namespace sv {

/// Yaml save and load class
class CBOR final{
public:
    struct OSize{

        template <typename T>
        OSize& operator <<(const T& c){
            sz+=sizeof(T);
            return *this;
        }

        void write(const char* s,size_t length){
            sz+=length;
        }
        size_t sz=0;
    };

    struct OStream{
        OStream(std::vector<char>& ost):o(ost),reverse(little_endianess()),ptr(ost.data()){}

        OStream& operator <<(char c){
            *(ptr++)=c;
            return *this;
        }

        template <typename T>
        OStream& operator <<(const T& c){
            std::array<char,sizeof(T)>& a=*(std::array<char,sizeof(T)>*)ptr;
            memcpy(a.data(),&c,sizeof(T));
            if(reverse){
                std::reverse(a.begin(), a.end());
            }
            ptr+=sizeof(T);
            return *this;
        }

        void write(const char* s,size_t length){
            memcpy(ptr,s,length);
            ptr+=length;
        }

        static constexpr bool little_endianess() noexcept
        {
            return 0x78 == (char)0x12345678;
        }

        std::vector<char>& o;
        bool         reverse;
        char*        ptr;
    };

    struct IStream{
        IStream(SvarBuffer& ist):i(ist),reverse(OStream::little_endianess()),ptr((const char*)i._ptr){}
        template<typename T>
        IStream& operator >>(T & c){
            if(reverse){
                std::array<char,sizeof(T)>& a=*(std::array<char,sizeof(T)>*)&c;
                memcpy(a.data(),ptr,a.size());
                std::reverse(a.begin(),a.end());
            }
            else
                memcpy(&c,ptr,sizeof(T));
            ptr+=sizeof(T);
            return *this;
        }

//        IStream& operator >>(uint8_t& c){
//            c=*((const uint8_t*)ptr++);
//            return *this;
//        }

        const void* read(size_t sz){
            const void* ret=ptr;
            ptr+=sz;
            return ret;
        }
        SvarBuffer& i;
        const char*     ptr;
        bool        reverse;
    };

    static Svar load(SvarBuffer input){
        IStream i(input);
        return loadStream(i);
    }

    static Svar loadStream(IStream& i){
        typedef unsigned char u_char;
        static std::vector<std::function<Svar(u_char,IStream&)> > funcs;
        if(funcs.empty()){
            funcs.resize(256);
            for(int it=0;it<256;it++)funcs[it]=[](u_char c,IStream& i){return Svar::Undefined();};
            for(u_char it=0;it<=0x17;it++)
                funcs[it]=[](u_char c,IStream& i){return Svar((int)c);};
            funcs[0x18]=[](u_char c,IStream& i){uint8_t v;i>>v;return Svar((int)v);};
            funcs[0x19]=[](u_char c,IStream& i){uint16_t v;i>>v;return Svar((int)v);};
            funcs[0x1A]=[](u_char c,IStream& i){uint32_t v;i>>v;return Svar((int)v);};
            funcs[0x1B]=[](u_char c,IStream& i){uint64_t v;i>>v;return Svar((int)v);};//positive int
            for(u_char it=0x20;it<=0x37;it++)
                funcs[it]=[](u_char c,IStream& i){return Svar((int)(-1-(c-0x20)));};
            funcs[0x38]=[](u_char c,IStream& i){uint8_t v;i>>v;return Svar((int)(-1-v));};
            funcs[0x39]=[](u_char c,IStream& i){uint16_t v;i>>v;return Svar((int)(-1-v));};
            funcs[0x3A]=[](u_char c,IStream& i){uint32_t v;i>>v;return Svar((int)(-1-v));};
            funcs[0x3B]=[](u_char c,IStream& i){uint64_t v;i>>v;return Svar((int)(-1-v));};//negative int
            for(u_char it=0x40;it<=0x5F;it++)
                funcs[it]=[](u_char c,IStream& i){
                    int len = funcs[c-0x40](c-0x40,i).as<int>();
                    std::string* buf=new std::string((char*)i.read(len),len);
                    return Svar::create(SvarBuffer(buf->data(),buf->size(),std::unique_ptr<std::string>(buf)));
                };//binary string
            for(u_char it=0x60;it<=0x7B;it++)
                funcs[it]=[](u_char c,IStream& i){
                    int len = funcs[c-0x60](c-0x60,i).as<int>();
                    return Svar(std::string((char*)i.read(len),len));
                };//string
            for(u_char it=0x80;it<=0x9B;it++)
                funcs[it]=[](u_char c,IStream& i){
                    int len = funcs[c-0x80](c-0x80,i).as<int>();
                    std::vector<Svar> vec(len);
                    for(int j=0;j<len;j++){
                        uint8_t t;i>>t;
                        vec[j]=funcs[t](t,i);
                    }
                    return Svar(vec);
                };//array
            for(u_char it=0xA0;it<=0xBB;it++)
                funcs[it]=[](u_char c,IStream& i){
                    int len = funcs[c-0xA0](c-0xA0,i).as<int>();
                    std::map<std::string,Svar> m;
                    for(int j=0;j<len;j++){
                        uint8_t t1,t2;
                        i>>t1;
                        Svar f=funcs[t1](t1,i);
                        i>>t2;
                        m[f.castAs<std::string>()]=funcs[t2](t2,i);
                    }
                    return Svar(m);
                };//map
            funcs[0xF4]=[](u_char c,IStream& i){return Svar(false);};//false
            funcs[0xF5]=[](u_char c,IStream& i){return Svar(true);};//true
            funcs[0xF6]=[](u_char c,IStream& i){return Svar::Null();};//null
            funcs[0xFB]=[](u_char c,IStream& i){double d;i>>d;return Svar(d);};//double
        }
        u_char c;
        i>>c;
        return funcs[c](c,i);
    }

    static SvarBuffer dump(Svar var){
        OSize sz;
        dumpStream(sz,var);
        std::vector<char>* ret=new std::vector<char>();
        ret->resize(sz.sz);
        OStream ost(*ret);
        dumpStream(ost,var);
        return SvarBuffer(ret->data(),ret->size(),std::unique_ptr<std::vector<char>>(ret));
    }

    static char c(std::uint8_t x){return *reinterpret_cast<char*>(&x);}

    template <typename T>
    static T& dumpStream(T& o,Svar var)
    {
        if(var.isUndefined())
            return o<<c(0xF6);

        if(var.is<bool>())
            return o<<(var.as<bool>()? c(0xF5) : c(0xF4));

        if(var.is<double>())
            return o<<c(0xFB)<<var.as<double>();

        if(var.is<int>())
        {
            const int& v=var.as<int>();
            if (v >= 0)
            {
                if (v <= 0x17)
                {
                    return o<<c(static_cast<std::uint8_t>(v));
                }
                else if (v <= (std::numeric_limits<std::uint8_t>::max)())
                {
                    return o<<c(0x18)<<c(static_cast<std::uint8_t>(v));
                }
                else if (v <= (std::numeric_limits<std::uint16_t>::max)())
                {
                    return o<<c(0x19)<<(static_cast<std::uint16_t>(v));
                }
                else if (v <= (std::numeric_limits<std::uint32_t>::max)())
                {
                    return o<<c(0x1A)<<static_cast<std::uint32_t>(v);
                }
                else// this never happens
                {
                    return o<<c(0x1B)<<static_cast<std::uint64_t>(v);
                }
            }
            else
            {
                // The conversions below encode the sign in the first
                // byte, and the value is converted to a positive number.
                const auto n = -1 - v;
                if (v >= -24)
                {
                    return o<<static_cast<std::uint8_t>(0x20 + n);
                }
                else if (n <= (std::numeric_limits<std::uint8_t>::max)())
                {
                    return o<<c(0x38)<<static_cast<std::uint8_t>(n);
                }
                else if (n <= (std::numeric_limits<std::uint16_t>::max)())
                {
                    return o<<c(0x39)<<static_cast<std::uint16_t>(n);
                }
                else if (n <= (std::numeric_limits<std::uint32_t>::max)())
                {
                    return o<<c(0x3A)<<static_cast<std::uint32_t>(n);
                }
                else
                {
                    return o<<c(0x3B)<<static_cast<std::uint64_t>(n);
                }
            }
        }

        if(var.is<SvarBuffer>()){
            SvarBuffer& s=var.as<SvarBuffer>();
            const auto N = s.length();
            if (N <= 0x17)
            {
                o<<c(static_cast<std::uint8_t>(0x40 + N));
            }
            else if (N <= (std::numeric_limits<std::uint8_t>::max)())
            {
                o<<c(0x58)<<(static_cast<std::uint8_t>(N));
            }
            else if (N <= (std::numeric_limits<std::uint16_t>::max)())
            {
                o<<c(0x59)<<static_cast<std::uint16_t>(N);
            }
            else if (N <= (std::numeric_limits<std::uint32_t>::max)())
            {
                o<<c(0x5A)<<static_cast<std::uint32_t>(N);
            }
            // LCOV_EXCL_START
            else if (N <= (std::numeric_limits<std::uint64_t>::max)())
            {
                o<<c(0x5B)<<(static_cast<std::uint64_t>(N));
            }
            // LCOV_EXCL_STOP

            // step 2: write the string
            o.write(reinterpret_cast<const char*>(s._ptr),N);
            return o;
        }

        if(var.is<std::string>())
        {
            // step 1: write control byte and the string length
            std::string& s=var.as<std::string>();
            const auto N = s.size();
            if (N <= 0x17)
            {
                o<<c(static_cast<std::uint8_t>(0x60 + N));
            }
            else if (N <= (std::numeric_limits<std::uint8_t>::max)())
            {
                o<<c(0x78)<<(static_cast<std::uint8_t>(N));
            }
            else if (N <= (std::numeric_limits<std::uint16_t>::max)())
            {
                o<<c(0x79)<<static_cast<std::uint16_t>(N);
            }
            else if (N <= (std::numeric_limits<std::uint32_t>::max)())
            {
                o<<c(0x7A)<<static_cast<std::uint32_t>(N);
            }
            // LCOV_EXCL_START
            else if (N <= (std::numeric_limits<std::uint64_t>::max)())
            {
                o<<c(0x7B)<<(static_cast<std::uint64_t>(N));
            }
            // LCOV_EXCL_STOP

            // step 2: write the string
            o.write(reinterpret_cast<const char*>(s.data()),N);
            return o;
        }

        if(var.isArray())
        {
            std::vector<Svar>& vec=var.as<SvarArray>()._var;
            const auto N = vec.size();
            if (N <= 0x17)
            {
                o<<c(static_cast<std::uint8_t>(0x80 + N));
            }
            else if (N <= (std::numeric_limits<std::uint8_t>::max)())
            {
                o<<c(0x98)<<(static_cast<std::uint8_t>(N));
            }
            else if (N <= (std::numeric_limits<std::uint16_t>::max)())
            {
                o<<c(0x99)<<static_cast<std::uint16_t>(N);
            }
            else if (N <= (std::numeric_limits<std::uint32_t>::max)())
            {
                o<<c(0x9A)<<static_cast<std::uint32_t>(N);
            }
            // LCOV_EXCL_START
            else if (N <= (std::numeric_limits<std::uint64_t>::max)())
            {
                o<<c(0x9B)<<static_cast<std::uint64_t>(N);
            }
            // LCOV_EXCL_STOP

            // step 2: write each element
            for (const auto& el : vec)
            {
                dumpStream(o,el);
            }
            return o;
        }

        if(var.isObject())
        {
            auto& obj=var.as<SvarObject>()._var;
            const auto N = obj.size();
            if (N <= 0x17)
            {
                o<<(static_cast<std::uint8_t>(0xA0 + N));
            }
            else if (N <= (std::numeric_limits<std::uint8_t>::max)())
            {
                o<<c(0xB8)<<(static_cast<std::uint8_t>(N));
            }
            else if (N <= (std::numeric_limits<std::uint16_t>::max)())
            {
                o<<c(0xB9)<<(static_cast<std::uint16_t>(N));
            }
            else if (N <= (std::numeric_limits<std::uint32_t>::max)())
            {
                o<<c(0xBA)<<(static_cast<std::uint32_t>(N));
            }
            // LCOV_EXCL_START
            else if (N <= (std::numeric_limits<std::uint64_t>::max)())
            {
                o<<c(0xBB)<<(static_cast<std::uint64_t>(N));
            }
            // LCOV_EXCL_STOP

            // step 2: write each element
            for (const auto& el : obj)
            {
                dumpStream(o,el.first);
                dumpStream(o,el.second);
            }
            return o;
        }

        Svar func_buffer=var.classPtr()->_attr["__buffer__"];
        if(func_buffer.isFunction()){
            dumpStream(o,func_buffer(var));
        }

        throw SvarExeption("Unable dump cbor for type " + var.typeName());
    }
};

REGISTER_SVAR_MODULE(CBOR){
    svar["parse_cbor"]=&CBOR::load;
    svar["dump_cbor"]=&CBOR::dump;
}

EXPORT_SVAR_INSTANCE

}

