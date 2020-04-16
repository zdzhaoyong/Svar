#include "Svar/SvarPy.h"

class PythonSpace{
public:
    PythonSpace(){
        Py_Initialize();
        PyEval_InitThreads();
        PyEval_ReleaseThread(PyThreadState_Get());
    }
    ~PythonSpace(){
        PyGILState_Ensure();
        Py_Finalize();
    }
};

std::shared_ptr<PythonSpace> pythonSpace;

sv::Svar import(std::string package){
    if(!pythonSpace)
        pythonSpace=std::make_shared<PythonSpace>();

    {
        sv::PyThreadStateLock PyThreadLock;
        PyObject* pModule = PyImport_ImportModule(package.c_str());
        return sv::SvarPy::fromPy(pModule);
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

    sv::Class<sv::PyObjectHolder>("PyObjectHolder")
            .def("__str__",[](sv::PyObjectHolder& h)->std::string{
//        sv::PyThreadStateLock PyThreadLock;
//        PyObject* strObj=PyObject_Repr(h.obj);
//        if(!strObj) return "PyObjectHolder:";
//        std::string str= sv::SvarPy::fromPy(strObj).as<std::string>();
        return std::string("holder:")+h.obj->ob_type->tp_name;
    });
}

EXPORT_SVAR_INSTANCE

