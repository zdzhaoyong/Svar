#include <Svar.h>
#include <sstream>
#include "json.hpp"

#include "Timer.h"

namespace GSLAM {

/// Yaml save and load class
class CBOR final{
public:
    struct OStream{
        OStream(std::ostream& ost):o(ost),reverse(little_endianess()){}

        OStream& operator <<(char c){
            o.put(c);
            return *this;
        }

        template <typename T>
        OStream& operator <<(const T& c){
            if(reverse){
                std::array<char,sizeof(T)> a;
                memcpy(a.data(),&c,sizeof(T));
                std::reverse(a.begin(), a.end());
                o.write(a.data(),a.size());
            }
            else{
                o.write((const char*)&c,sizeof(T));
            }
            return *this;
        }

        static constexpr bool little_endianess(int num = 1) noexcept
        {
            return *reinterpret_cast<char*>(&num) == 1;
        }

        operator std::ostream& (){return o;}
        std::ostream& o;
        bool         reverse;
    };

    struct IStream{

    };

    static nlohmann::json convert(Svar var){
        using namespace nlohmann;
        if(var.isNull()) return json(nullptr);
        else if(var.is<bool>()) return json(var.as<bool>());
        else if(var.is<int>()) return json(var.as<int>());
        else if(var.is<double>()) return json(var.as<double>());
        else if(var.is<std::string>()) return json(var.as<std::string>());
        else if(var.isArray()) {
            json ret=json::array();
            for(Svar& v:var.as<SvarArray>()._var)
                ret.push_back(convert(v));
            return ret;
        }
        else if(var.isObject()){
            json ret=json::object();
            for(auto v:var.as<SvarObject>()._var){
                ret[v.first]=convert(v.second);
            }
            return ret;
        }
        return json();
    }

    static Svar load(const std::vector<char>& input){
        std::stringstream sst(std::string(input.data(),input.size()));

        return Svar();
    }

    static std::vector<char> dumpCheck(Svar var){
        auto js=convert(var);
        timer.enter("json_cbor");
        std::vector<char> cbor;
        js.to_cbor(js,cbor);
        timer.leave("json_cbor");

        timer.enter("svar_cbor");
        std::vector<char> svar_cbor=dump(var);
        timer.leave("svar_cbor");

        assert(cbor.size()==svar_cbor.size());
        for(int i=0;i<cbor.size();i++){
            assert(cbor[i]==svar_cbor[i]);
        }

        return svar_cbor;
    }

    static std::vector<char> dump(Svar var){
        std::stringstream sst;
        OStream ost(sst);
        dumpStream(ost,var).o.flush();

        sst.seekg (0, sst.end);
        int length = sst.tellg();
        sst.seekg (0, sst.beg);
        std::vector<char> ret(length);
        sst.read (ret.data(),length);
        return ret;
    }

    static char c(std::uint8_t x){return *reinterpret_cast<char*>(&x);}
    static Svar loadStream(std::istream& in){}

    static OStream& dumpStream(OStream& o,Svar var)
    {
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
            o.o.write(reinterpret_cast<const char*>(s._ptr),N);
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
            o.o.write(reinterpret_cast<const char*>(s.data()),N);
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

        return o<<c(0xF6);
    }
};

REGISTER_SVAR_MODULE(CBOR){

        SvarClass::Class<CBOR>()
                .def("load",&CBOR::load)
                .def("dump",&CBOR::dump)
                .def("dumpCheck",&CBOR::dumpCheck);

        Svar::instance().set("__builtin__.CBOR",SvarClass::instance<CBOR>());
}

}

