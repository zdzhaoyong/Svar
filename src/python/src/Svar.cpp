#define __SVAR_BUILDIN__
#include <Svar/Svar.h>
#include <Python.h>
#include <cstring>

using namespace GSLAM;

struct SvarPy
{
    PyObject_HEAD
    GSLAM::Svar var;
};

struct SvarPyTypeObject: public PyHeapTypeObject
{
    Svar cls;
};

PyObject* getPy(Svar src);


PyObject* getPyClass(Svar var){
    SvarClass& cls=var.as<SvarClass>();
    if(cls._attr.exist("PyTypeObject"))
        return cls._attr["PyTypeObject"].castAs<PyObject*>();

    auto heap_type = (PyHeapTypeObject *) PyType_Type.tp_alloc(&PyType_Type, 0);
    if (!heap_type){
        std::cerr<<("Error allocating type!")<<std::endl;
        return Py_None;
    }

    heap_type->ht_name = PyUnicode_FromString(cls.__name__.c_str());

    auto type = &heap_type->ht_type;
    type->tp_name = cls.__name__.c_str();
    Py_IncRef((PyObject*)&PyProperty_Type);
    type->tp_base = &PyProperty_Type;
    type->tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HEAPTYPE;
    if (PyType_Ready(type) < 0)
        std::cerr<<("make_static_property_type(): failure in PyType_Ready()!");

    PyObject_SetAttrString((PyObject *) type, "__module__", PyUnicode_FromString("pybind11_builtins"));
    cls._attr.set("PyTypeObject",type);
    for(std::pair<std::string,Svar> f:cls._methods.as<SvarObject>()._var)
    {
        PyObject_SetAttrString((PyObject*)type,f.first.c_str(),getPy(f.second));
    }


    return (PyObject*)type;
}

PyObject* getPy(Svar src){
    SvarClass* cls=src.classPtr();
    Svar func=(*cls)["getPy"];
    if(func.isFunction())
        return func(src).as<PyObject*>();

    std::function<PyObject*(Svar)> convert;
    if(src.isNull())
        convert=[](Svar src){return Py_None;};
    else if(src.is<int>())
        convert=[](Svar src){return PyLong_FromLong(src.as<int>());};
    else if(src.is<double>())
        convert=[](Svar src){return PyFloat_FromDouble(src.as<double>());};
    else if(src.is<std::string>())
        convert=[](Svar src){
            std::string& str=src.as<std::string>();
            return PyUnicode_FromStringAndSize(str.data(),str.size());
        };
    else if(src.isArray())
        convert=[](Svar src){
            PyObject* obj=PyList_New(src.length());
            size_t index = 0;
            for (Svar& value : src.as<SvarArray>()._var) {
                PyObject* value_ =getPy(value);
                if (!value_)
                {
                    delete value_;
                    return Py_None;
                }
                PyList_SET_ITEM(obj, (ssize_t) index++, value_); // steals a reference
            }
            return obj;
        };
    else if(src.isObject())
        convert=[](Svar src){
            PyObject* dict=PyDict_New();
            for (auto kv : src.as<SvarObject>()._var) {
                PyObject* key = PyUnicode_FromStringAndSize(kv.first.data(),kv.first.size());
                PyObject* value = getPy(kv.second);
                if (!key || !value)
                    return Py_None;
                PyDict_SetItem(dict,key,value);
            }
            return dict;
        };
    else if(src.isFunction())
        convert=[](Svar src){
            return Py_None;
            SvarFunction& svar_func=src.as<SvarFunction>();
            PyMethodDef* func = new PyMethodDef();
            std::memset(func, 0, sizeof(PyMethodDef));
            func->ml_name = svar_func.name.c_str();
            func->ml_meth = [](PyObject *, PyObject *)->PyObject*{
                return Py_None;
            };
            func->ml_flags = METH_VARARGS ;
            func->ml_doc=svar_func.signature.c_str();
            return (PyObject*)func;
        };
    else if(src.isClass()){
        convert=[](Svar src){
            return getPyClass(src);
        };
    }
    else convert=[](Svar src){
        return Py_None;
    };

    cls->def("getPy",convert);
    return getPy(src);
}

PyObject* getModule(GSLAM::Svar var){
    if(var.isObject())
    {
        PyModuleDef *def = new PyModuleDef();
        memset(def, 0, sizeof(PyModuleDef));
        def->m_name = var.GetString("__name__","svar").c_str();
        def->m_doc =  var.GetString("__doc__","Python interface of Svar.").c_str();
        def->m_size = -1;
        Py_INCREF(def);

        PyObject* pyModule = nullptr;
        pyModule = PyModule_Create(def);

        if(!pyModule)
            return nullptr;

        for(std::pair<std::string,Svar> pair:var.as<SvarObject>()._var)
        {
            PyObject* obj=getModule(pair.second);
            std::cerr<<"add object "<<pair.first<<std::endl;
            PyModule_AddObject(pyModule, pair.first.c_str(), obj);
        }

        return pyModule;
    }

    return getPy(var);
}

extern "C" SVAR_EXPORT PyObject* PyInit_svar()
{
    return getModule(svar);
}

