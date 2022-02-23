#include "Svar.h"

using namespace sv;

int complete(Svar config){
    std::string bin = config.arg<std::string>("bin","","Your binary name, eg: svar");

    if(config.get("help",false) || bin.empty()) {return config.help();}

    std::string comp_file = "/etc/bash_completion.d/" + bin;
    std::ofstream ofs(comp_file);
    if(!ofs.is_open()){
        std::cerr << "Can not open file " << comp_file;
    }

    ofs<<R"(# svar tab completion support
function_svar_complete()
{
  COMPREPLY=()
  local cur=${COMP_WORDS[COMP_CWORD]};
  local com=${COMP_WORDS[COMP_CWORD-1]};
  local can=$(${COMP_WORDS[*]:0:COMP_CWORD} -help -complete_function_request)
  local reg="-.+"
  if [[ $com =~ $reg ]];then
    COMPREPLY=($(compgen -df -W "$can" -- $cur))
  else
    COMPREPLY=($(compgen -W "$can" -- $cur))
  fi
}
         )";

    ofs<<"\ncomplete -F function_svar_complete "<<bin<<std::endl;

    std::cout << "tab completion is now supported by " << comp_file<<".\n";

    return 0;
}

REGISTER_SVAR_MODULE(complete){
    svar["apps"]["complete"]={complete,"Auto complete your svar based arguments."};
}
