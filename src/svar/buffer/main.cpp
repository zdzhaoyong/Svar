#include "Svar.h"
#include "Glog.h"
#include "Timer.h"

using namespace sv;
using namespace GSLAM;

int md5(Svar config){
    auto filename=config["md5"].as<std::string>();
    SvarBuffer buf=SvarBuffer::load(filename);
    std::cerr<<buf.md5()<<" "<<filename<<std::endl;
    return 0;
}

int base64(Svar config){
    auto filename=config["base64"].as<std::string>();
    SvarBuffer buf=SvarBuffer::load(filename);
    std::cerr<<buf.base64()<<std::endl;
    return 0;
}

int hex(Svar config){
    auto filename=config["hex"].as<std::string>();
    SvarBuffer buf=SvarBuffer::load(filename);
    std::cerr<<buf.hex()<<std::endl;
    return 0;
}

int speed(Svar config)
{
    SvarBuffer buf=SvarBuffer::load(config["speed"].as<std::string>());
    LOG(INFO)<<"Buffer size is "<<buf.size();
    std::string str;
    {
        ScopedTimer tm("HexEncode");
        str=buf.hex();
    }
    {
        ScopedTimer tm("HexDecode");
        buf=SvarBuffer::fromHex(str);
    }
    {
        ScopedTimer tm("Base64Encode");
        str=buf.base64();
    }
    {
        ScopedTimer tm("Base64Decode");
        buf=SvarBuffer::fromBase64(str);
    }
    return 0;
}

int buffer(Svar config){
    std::string f_md5=config.arg<std::string>("md5",config["argv"].as<char**>()[0],"the file to show md5");
    std::string f_base64=config.arg<std::string>("base64","","the file to show base64");
    std::string f_hex=config.arg<std::string>("hex","","the file to show hex");
    std::string f_speed=config.arg<std::string>("speed","","the file to testing speed");

    if(config.get("help",false)) return config.help();

    if(f_base64.size()) return base64(config);
    if(f_hex.size()) return hex(config);
    if(f_speed.size()) return speed(config);
    if(f_md5.size()) return md5(config);

    return 0;
}

REGISTER_SVAR_MODULE(buffer){
    svar["apps"]["buffer"]={buffer,"SvarBuffer to operate md5, base64 and hex"};
}
