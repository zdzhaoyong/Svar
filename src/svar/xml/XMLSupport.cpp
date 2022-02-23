#include "Svar.h"
#ifdef HAS_GSLAM
#define GSLAM_SVAR_HAS_XML
#endif
#ifdef GSLAM_SVAR_HAS_XML
#include <GSLAM/core/XML.h>
#endif

namespace sv {

/// Xml save and load class
class Xml final{
public:
    static Svar load(const std::string& in){
#ifdef GSLAM_SVAR_HAS_XML
        XMLDocument doc;
        if(XML_SUCCESS!=doc.Parse(in.c_str(),in.size())) return Svar();
        Svar var;
        loadXML(var,&doc);
        return var;
#endif
        return Svar();
    }

    static std::string dump(Svar var){
#ifdef GSLAM_SVAR_HAS_XML
        tinyxml2::XMLDocument doc;
        doc.InsertEndChild(doc.NewDeclaration());
        exportXML(var,&doc);
        XMLPrinter printer;
        doc.Print( &printer );
        return printer.CStr();
#endif

        return "";
    }

#ifdef GSLAM_SVAR_HAS_XML
    static void loadXML(Svar& var,XMLNode* node){
        if(auto text=node->ToText()){
            if(var.isUndefined())
                var.set(std::string(text->Value()));
            else
                var.set("#text",std::string(text->Value()));
            return;
        }
        if(auto comment=node->ToComment()){
            return;
        }
        if(auto doc=node->ToDocument()){
            for(auto it=doc->FirstChild();it;it=it->NextSibling())
            {
                loadXML(var,it);
            }
            return;
        }
        if(node->ToDeclaration()){
            return;
        }

        XMLElement* ele=node->ToElement();
        if(!ele) {
            std::cerr<<"Unkown XMLElement type.";
            return;
        }

        GSLAM::Svar child;
        for(auto it=ele->FirstAttribute();it!=nullptr;it=it->Next())
        {
            child.set(std::string("@")+it->Name(),std::string(it->Value()));
        }
        for(auto it=ele->FirstChild();it;it=it->NextSibling())
        {
            loadXML(child,it);
        }
        auto old=var[std::string(ele->Name())];
        if(old.isUndefined())
            var.set(ele->Name(),child);
        else if(old.isArray())
            old.push_back(child);
        else
            var.set(ele->Name(),Svar::array({old,child}));
    }

    static void exportXML(const Svar& var,XMLNode* node){
        if(auto doc=node->ToDocument()){
            if(!var.isObject()) return;
            for(std::pair<std::string,Svar> child:var.as<SvarObject>().getCopy())
            {
                XMLElement* ele = doc->NewElement(child.first.c_str());
                doc->InsertEndChild(ele);
                exportXML(child.second,ele);
            }
            return;
        }

        auto ele=node->ToElement();
        if(!var.isObject()) {
            ele->SetText(toString(var).c_str());
            return;
        }
        for(std::pair<std::string,Svar> child:var.as<SvarObject>().getCopy())
        {
            if(child.first[0]=='@')
                ele->SetAttribute(child.first.substr(1).c_str(),toString(child.second).c_str());
            else if(child.first[0]=='#')
                ele->SetText(toString(child.second).c_str());
            else if(child.second.isArray()){
                for(size_t i=0;i<child.second.length();i++){
                    XMLElement* newEle = ele->GetDocument()->NewElement(child.first.c_str());
                    exportXML(child.second[(int)i],newEle);
                    ele->InsertEndChild(newEle);
                }
            }
            else if(child.second.isObject()){
                XMLElement* newEle = ele->GetDocument()->NewElement(child.first.c_str());
                exportXML(child.second,newEle);
                ele->InsertEndChild(newEle);
            }
            else {
                XMLElement* newEle = ele->GetDocument()->NewElement(child.first.c_str());
                newEle->SetText(toString(child.second).c_str());
                ele->InsertEndChild(newEle);
            }
        }

    }

    static std::string toString(Svar var){
        if(var.is<std::string>()) return var.as<std::string>();
        else return Svar::toString(var).c_str();
    }
#endif
};


REGISTER_SVAR_MODULE(XML){
        SvarClass::Class<Xml>()
                .def("load",&Xml::load)
                .def("dump",&Xml::dump);
#ifdef GSLAM_SVAR_HAS_XML
        Svar::instance().set("__builtin__.Xml",SvarClass::instance<Xml>());
#endif
    }
}
