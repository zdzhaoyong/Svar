#include <Svar/Registry.h>
#include "SvarPy.h"

using namespace sv;

std::shared_ptr<PythonSpace> pythonSpace;

std::string  getFileName(const std::string& path){
    auto idx = std::string::npos;
    if ((idx = path.find_last_of('/')) == std::string::npos)
        idx = path.find_last_of('\\');
    if (idx != std::string::npos)
        return path.substr(idx + 1);
    else
        return path;
}

std::string getBaseName(const std::string& path) {
  std::string filename = getFileName(path);
  auto idx = filename.find_last_of('.');
  if (idx == std::string::npos)
    return filename;
  else
    return filename.substr(0, idx);
}


PyObject* load(std::string pluginPath){
    return SvarPy::getModule(Registry::load(pluginPath),getBaseName(pluginPath).c_str());
}

void set(std::string name, Svar value){
    svar.set(name,value);
}

SVAR_PYTHON_IMPL(svar)
{
    PyEval_InitThreads();

    Svar module;
    module["load"]=load;
    module["set"] =set;
    module["__name__"]="svar";
    PyObjectHolder pymodule= SvarPy::getModule(module);
    return pymodule;
}

sv::Svar import(std::string package){
    if(!pythonSpace){
        pythonSpace=std::make_shared<PythonSpace>();
    }

    {
        sv::PyThreadStateLock PyThreadLock;
        PyObject* pModule = PyImport_ImportModule(package.c_str());
        return sv::SvarPy::fromPy(pModule,true);
    }
}

void  run(std::string line){
    if(!pythonSpace)
        pythonSpace=std::make_shared<PythonSpace>();
    {
        sv::PyThreadStateLock PyThreadLock;
        PyRun_SimpleString(line.c_str());
    }
}

REGISTER_SVAR_MODULE(python){
    svar["import"]=import;
    svar["run"]=run;
    svar["fromPy"]=sv::SvarFunction([](sv::PyObjectHolder& holder){
            if(!pythonSpace)
                pythonSpace=std::make_shared<PythonSpace>();
            sv::PyThreadStateLock PyThreadLock;
      return sv::SvarPy::fromPy(holder.obj);
    });
    svar["getPy"]=sv::SvarFunction([](sv::Svar obj)->sv::Svar{
            if(!pythonSpace)
                pythonSpace=std::make_shared<PythonSpace>();
            sv::PyThreadStateLock PyThreadLock;
            auto pyobj=sv::SvarPy::getPy(obj);
      return pyobj;
    });
    svar["getPyModule"]=sv::SvarFunction([](sv::Svar obj)->sv::Svar{
            if(!pythonSpace)
                pythonSpace=std::make_shared<PythonSpace>();
            sv::PyThreadStateLock PyThreadLock;
            auto pyobj=sv::SvarPy::getModule(obj);
      return pyobj;
    });

    sv::Class<sv::PythonSpace>("PythonSpace")
            .def("__init__",[](){
        return std::make_shared<PythonSpace>();
    });

    sv::Class<sv::PyObjectHolder>("PyObjectHolder")
            .def("__str__",[](sv::PyObjectHolder& h)->std::string{
        sv::PyThreadStateLock PyThreadLock;
        PyObject* strObj=PyObject_Repr(h.obj);
        if(!strObj) return "<PyObjectHolder>";
        std::string str= sv::SvarPy::fromPy(strObj).as<std::string>();
        return str;
    })
    .def("__getitem__",[](sv::PyObjectHolder& h,const Svar& name){
        sv::PyThreadStateLock PyThreadLock;
        if(PyList_Check(h.obj))
            return SvarPy::fromPy(PyList_GetItem(h.obj,name.as<int>()),true);
        if(PyTuple_Check(h.obj))
            return SvarPy::fromPy(PyTuple_GetItem(h.obj,name.as<int>()),true);

        return SvarPy::fromPy(PyObject_GetAttrString(h.obj, name.as<std::string>().c_str()),true);
    })
    .def("__setitem__",[](sv::PyObjectHolder& h,const Svar& name,Svar def){
        sv::PyThreadStateLock PyThreadLock;
        if(PyList_Check(h.obj))
            return PyList_SetItem(h.obj,name.as<int>(),SvarPy::getPy(def));
        if(PyTuple_Check(h.obj))
            return PyTuple_SetItem(h.obj,name.as<int>(),SvarPy::getPy(def));

        return PyObject_SetAttrString(h.obj, name.as<std::string>().c_str(),SvarPy::getPy(def));
    })
    .def("__delitem__",[](sv::PyObjectHolder& h,const std::string& name){
        sv::PyThreadStateLock PyThreadLock;
        return PyObject_DelItemString(h.obj, (char*)name.c_str());
    })
    .def("append",[](sv::PyObjectHolder& h,const Svar& obj){
        sv::PyThreadStateLock PyThreadLock;
        return PyList_Append(h.obj, SvarPy::getPy(obj));
    });
}

EXPORT_SVAR_INSTANCE


