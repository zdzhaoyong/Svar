#define __SVAR_BUILDIN__
#include <Svar/SvarPy.h>

using namespace GSLAM;

PyObject* load(std::string pluginPath){
    SharedLibraryPtr plugin(new SharedLibrary(pluginPath));
    if(!plugin->isLoaded()){
        LOG(ERROR)<<"Unable to load plugin "<<pluginPath;
        return Py_None;
    }

    GSLAM::Svar* (*getInst)()=(GSLAM::Svar* (*)())plugin->getSymbol("svarInstance");
    if(!getInst){
        LOG(ERROR)<<"No svarInstance found in "<<pluginPath<<std::endl;
        return Py_None;
    }
    GSLAM::Svar* inst=getInst();
    if(!inst){
        LOG(ERROR)<<"svarInstance returned null.\n";
        return Py_None;
    }
    return SvarPy::getModule(*inst);
}

SVAR_PYTHON_IMPL(svar)
{
    svar.set("load",Svar(load));
    return SvarPy::getModule(svar);
}
