#include <Registry.h>

using namespace sv;

int version(Svar config){

    if(config.get("help",false)) {return config.help();}

    std::cout<<"version: "<<SVAR_VERSION<<std::endl
            <<"tag:"<<svar["__builtin__"]["tag"]<<std::endl
            <<"date:"<<__DATE__<<" "<<__TIME__<<std::endl;
    return 0;
}

REGISTER_SVAR_MODULE(version){
    svar["apps"]["version"]={version,"Show the build version information."};
}
