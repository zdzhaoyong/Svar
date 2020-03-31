#include "Svar.h"
#ifdef GSLAM_SVAR_HAS_YAML
#include <yaml-cpp/yaml.h>
#endif

namespace sv {

/// Yaml save and load class
class Yaml final{
public:
    static Svar load(const std::string& input){
#ifdef GSLAM_SVAR_HAS_YAML
        YAML::Node node=YAML::Load(input);
        std::cout<<node;
        return toSvar(node);
#endif
        return Svar();
    }
    static std::string dump(Svar var){return "";}

#ifdef GSLAM_SVAR_HAS_YAML
    static Svar toSvar(const YAML::Node& node){
        if(!node.IsDefined()) return Svar::Undefined();
        if(node.IsScalar()) return Svar(node.Scalar());
        if(node.IsSequence()){
            std::vector<Svar> vars;
            vars.reserve(node.size());
            for(auto it:node)
                vars.push_back(toSvar(it));
            return vars;
        }
        else {
            std::map<std::string,Svar> vars;
            for(YAML::const_iterator it=node.begin();it!=node.end();++it)
            {
//                std::cout<<it->first<<it->second;
            }
            return vars;
        }
        return Svar::Undefined();
    }

    static YAML::Node toNode(Svar var){
        return YAML::Node();
    }
#endif
};

REGISTER_SVAR_MODULE(Yaml){

        SvarClass::Class<Yaml>()
                .def("load",&Yaml::load)
                .def("dump",&Yaml::dump);

#ifdef GSLAM_SVAR_HAS_YAML
        Svar::instance().set("__builtin__.Yaml",SvarClass::instance<Yaml>());
#endif
}


}
