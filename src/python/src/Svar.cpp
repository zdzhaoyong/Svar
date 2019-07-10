#define __SVAR_BUILDIN__
#include <Svar/SvarPy.h>
#include <Svar/Registry.h>

using namespace GSLAM;

PyObject* load(std::string pluginPath){
    return SvarPy::getModule(Registry::load(pluginPath));
}

SVAR_PYTHON_IMPL(svar)
{
    svar.set("load",Svar(load));
    return SvarPy::getModule(svar);
}
