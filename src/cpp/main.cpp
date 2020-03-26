#include "Svar.h"
#include "Registry.h"
#include "Timer.h"

using namespace jv;

int main(int argc,char** argv){
    Svar var=jvar;
    std::vector<std::string> unParsed=var.parseMain(argc,argv);

    Svar apps=jvar["apps"];

    if(var.get("complete_function_request",false)&&unParsed.empty())
    {
        for(std::pair<std::string,Svar> app:apps)
        std::cout<<app.first<<" ";
    }

    if(unParsed.empty()){
        Svar description;
        for(std::pair<std::string,Svar> app:apps)
            description[app.first]=app.second.isFunction()?
                        "This app don't have description.":app.second[1];
        std::stringstream sst;
        sst<<"svar [app_name] [-option value] [-help]\n"
          <<"The following apps can be used:\n";
        var["__usage__"]=sst.str()+description.dump_json();
        return var.help();
    }

    if(unParsed.size()>1) LOG(ERROR)<<"Should only has one app, there are "<<Svar(unParsed);

    Svar app=apps[unParsed.front()];
    if(!app.isFunction()) app=app[0];
    return app(var).as<int>();
}
