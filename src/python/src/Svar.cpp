#include <Svar/SvarPy.h>
#include <Svar/Registry.h>

using namespace jv;

PyObject* load(std::string pluginPath){
    return SvarPy::getModule(Registry::load(pluginPath));
}

SVAR_PYTHON_IMPL(svar)
{
    jvar.set("load",Svar(load));
    return SvarPy::getModule(jvar);
}
