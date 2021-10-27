#ifndef SVAR_SVARPY_H
#define SVAR_SVARPY_H

#include "Svar/Svar.h"
#include <Python.h>
#include <cstring>

#if PY_MAJOR_VERSION >= 3 /// Compatibility macros for various Python versions
#define SVAR_INSTANCE_METHOD_NEW(ptr, class_) PyInstanceMethod_New(ptr)
#define SVAR_BYTES_AS_STRING_AND_SIZE PyBytes_AsStringAndSize
#define SVAR_FROM_STRING PyUnicode_FromString
#define SVAR_PYTHON_IMPL(name) \
    extern "C" SVAR_EXPORT PyObject *PyInit_##name()

#else
#define SVAR_INSTANCE_METHOD_NEW(ptr, class_) PyMethod_New(ptr, nullptr, class_)
#define SVAR_BYTES_AS_STRING_AND_SIZE PyString_AsStringAndSize
#define SVAR_FROM_STRING PyString_FromString
#define SVAR_PYTHON_IMPL(name) \
    static PyObject *SVAR_init_wrapper();               \
    extern "C" SVAR_EXPORT void init##name() {          \
        (void)SVAR_init_wrapper();                      \
    }                                                       \
    PyObject *SVAR_init_wrapper()
#endif

#ifndef LOG
#define LOG(c) std::cout
#endif

namespace sv {

class PythonSpace{
public:
    PythonSpace(){
        Py_Initialize();
        PyEval_InitThreads();
        PyEval_ReleaseThread(PyThreadState_Get());
    }
    ~PythonSpace(){
        PyGILState_Ensure();
//        Py_Finalize();
    }

    static const char* safe_c_str(std::string s){
        static std::unordered_map<std::string,std::string> cache;
        auto it=cache.find(s);
        if(it == cache.end())
        {
            auto ret=cache.insert(std::pair<std::string,std::string>(s,s));
            it=ret.first;
        }

        return it->second.c_str();
    }
};

class PyThreadStateLock
{
public:
    PyThreadStateLock(void)
    {
        state = PyGILState_Ensure( );
    }

    ~PyThreadStateLock(void)
    {
         PyGILState_Release( state );
    }
private:
    PyGILState_STATE state;
};

struct PyObjectHolder{
    PyObjectHolder(const PyObjectHolder& r):obj(r.obj){
        Py_IncRef(obj);
    }
public:

    PyObjectHolder(PyObject* o,bool inc=true):obj(o){
        if(inc)
            Py_IncRef(obj);
    }

    ~PyObjectHolder(){
        PyThreadStateLock lock;
        Py_DecRef(obj);
    }

    operator PyObject*(){
        Py_IncRef(obj);
        return obj;
    }
    PyObject* obj;
};

struct SvarPy: public PyObject{
    Svar* var;

    static PyObject* incref(PyObject* obj){
        Py_IncRef(obj);
        return obj;
    }

    static PyObject* decref(PyObject* obj){
        Py_DecRef(obj);
        return obj;
    }

    static PyTypeObject* type_incref(PyTypeObject* obj){
        Py_IncRef((PyObject*)obj);
        return obj;
    }

    static PyTypeObject* type_decref(PyTypeObject* obj){
        Py_DecRef((PyObject*)obj);
        return obj;
    }

    // The return holder should auto decref and release if not used
    static PyObjectHolder getPy(Svar src)
    {
        switch (src.jsontype()) {
        case sv::undefined_t:
            return Py_None;
            break;
        case sv::null_t:
            return Py_None;
            break;
        case sv::boolean_t:
            return src.as<bool>()?Py_True:Py_False;
            break;
        case sv::integer_t:
            return PyObjectHolder(PyLong_FromLong(src.as<int>()),false);
            break;
        case sv::float_t:
            return PyObjectHolder(PyFloat_FromDouble(src.as<double>()),false);
            break;
        case sv::string_t:{
            std::string& str=src.as<std::string>();
            return PyObjectHolder(PyUnicode_FromStringAndSize(str.data(),str.size()),false);
        };
            break;
        case sv::array_t:{
            PyObject* obj=PyTuple_New(src.length());
            size_t index = 0;
            for (Svar& value : src.as<SvarArray>()._var) {
                PyObjectHolder value_ =SvarPy::getPy(value);
                if (!value_.obj)
                {
                    decref(obj);
                    return Py_None;
                }
                PyTuple_SetItem(obj, (ssize_t) index++, value_); // should own
            }
            return PyObjectHolder(obj,false);
        }
            break;
        case sv::object_t:{
            PyObject* dict=PyDict_New();
            for (std::pair<std::string,Svar> kv : src) {
                PyObject* key = PyUnicode_FromStringAndSize(kv.first.data(),kv.first.size());
                PyObject* value = SvarPy::getPy(kv.second);
                if (!key || !value){
                    decref(dict);
                    decref(key);
                    decref(value);
                    return Py_None;
                }
                PyDict_SetItem(dict,key,value); // steal
                decref(key);
                decref(value);
            }
            return PyObjectHolder(dict,false);
        }
            break;
        case sv::function_t:
            return PyObjectHolder(SvarPy::getPyFunction(src),false);
            break;
        case sv::svarclass_t:
            return PyObjectHolder((PyObject*)SvarPy::getPyClass(src.as<SvarClass>()),false);
            break;
        case sv::property_t:
            return PyObjectHolder(SvarPy::getPyProperty(src),false);
            break;
        default:{
            if(src.is<PyObjectHolder>()) // 0.01s/100k
                return src.as<PyObjectHolder>();

            PyTypeObject* py_class = getPyClass(src.classObject()); // steal 0.01s/100k
            PyObject *self = py_class->tp_alloc(py_class, 0); // 0.02s/100k
            SvarPy* obj = reinterpret_cast<SvarPy*>(self);
            obj->var = new Svar(src);
            return PyObjectHolder(obj,false);
        }
            break;
        }
    }

    static PyObject* svar_func_wrapper(PyObject *capsule, PyObject *args, PyObject *kwargs){
        Svar* func = (Svar*)PyCapsule_GetPointer(capsule,"svar_function");
        SvarFunction& svarFunc = func->as<SvarFunction>();

        try{
            std::vector<Svar> svar_args;
            int nargs=PyTuple_Size(args);
            svar_args.reserve(nargs);
            for(Py_ssize_t i=0;i<nargs;++i)
                svar_args.push_back(fromPy(PyTuple_GetItem(args,i)));
            if(kwargs){
                Svar svar_kwargs = SvarPy::fromPy(kwargs);
                for(std::pair<std::string,Svar> kw : svar_kwargs)
                    svar_args.push_back(arg(kw.first)=kw.second);
            }

            auto ret=svarFunc.Call(svar_args);// 0.1s/100k
            return SvarPy::getPy(ret);// 0.02s/100k
        }
        catch(SvarExeption e){
            PyErr_SetString(PyExc_RuntimeError, e.what());
            return incref(Py_None);
        }
    }

    static PyObject* getPyProperty(Svar src){
      SvarClass::SvarProperty& property=src.as<SvarClass::SvarProperty>();
      PyObjectHolder py_args = getPy({property._fget,property._fset,property._doc});
      PyObject *result = PyObject_Call((PyObject*)&PyProperty_Type,
                                       py_args.obj,nullptr);
      if (!result){
        struct error_scope {
          PyObject *type, *value, *trace;
          error_scope() { PyErr_Fetch(&type, &value, &trace); }
          ~error_scope() { PyErr_Restore(type, value, trace); }
        }error;
        throw SvarExeption(fromPy(error.value));
      }
      return result;
    }

    static PyObject* getPyFunction(Svar src){
        SvarFunction& svar_func = src.as<SvarFunction>();
        PyMethodDef* func = new PyMethodDef();
        std::memset(func, 0, sizeof(PyMethodDef));
        func->ml_name  = svar_func.name.c_str();
        func->ml_flags = METH_VARARGS| METH_KEYWORDS ;
        if(svar_func.is_constructor)
            func->ml_flags |= METH_CLASS;
        if(svar_func.arg_types.size()){
            svar_func.meta["full_doc"] = svar_func.signature();
            func->ml_doc = svar_func.meta["full_doc"].as<std::string>().c_str();
        }
        else
            func->ml_doc = "";

        func->ml_meth = reinterpret_cast<PyCFunction>(svar_func_wrapper);

        PyObject* capsule = PyCapsule_New(new Svar(src),"svar_function",[](PyObject *o) {
            delete (Svar*)PyCapsule_GetPointer(o, "svar_function");
        });

        auto c_func = PyCFunction_NewEx(func,capsule,nullptr);
        decref(capsule);
        if(!svar_func.is_method)
            return c_func;

        auto m_ptr = SVAR_INSTANCE_METHOD_NEW(c_func, nullptr);
        if (!m_ptr)
            std::cerr<<("cpp_function::cpp_function(): Could not allocate instance method object");
//        Py_DECREF(c_func);
        return m_ptr;
    }

    static PyTypeObject* getPyClass(SvarClass& cls){
        if(cls.__setitem__.is<PyTypeObject*>())
            return cls.__setitem__.as<PyTypeObject*>();

        auto heap_type = (PyHeapTypeObject *) PyType_Type.tp_alloc(&PyType_Type, 0);
        if (!heap_type){
            LOG(ERROR)<<("Error allocating type!")<<std::endl;
            return (PyTypeObject*)Py_None;
        }

        heap_type->ht_name = SVAR_FROM_STRING(cls.__name__.c_str());

        PyTypeObject* type = &heap_type->ht_type;
        type->tp_name = cls.__name__.c_str();
        type->tp_base = type_incref(&PyBaseObject_Type);// FIXME: Why cause segment fault help when inherit svar_object
        type->tp_basicsize=sizeof(SvarPy);
        type->tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HEAPTYPE;
        type->tp_doc="";
        type->tp_as_number = &heap_type->as_number;
        type->tp_as_sequence = &heap_type->as_sequence;
        type->tp_as_mapping = &heap_type->as_mapping;

        const SvarObject& attr = cls._attr.as<SvarObject>();
        if(attr["__buffer__"].isFunction()){
          type->tp_as_buffer= & heap_type->as_buffer;
          heap_type->as_buffer.bf_getbuffer=[](PyObject *obj, Py_buffer *view, int flags)->int{
             Svar* var=((SvarPy*)obj)->var;
             Svar  ret=var->call("__buffer__");
             if (view == nullptr || obj == nullptr || ret.isUndefined()) {
                 if (view)
                     view->obj = nullptr;
                 PyErr_SetString(PyExc_BufferError, "Svar: bf_getbuffer Internal error");
                 return -1;
             }
             SvarBuffer& buffer=ret.as<SvarBuffer>();
             std::memset(view, 0, sizeof(Py_buffer));
             view->buf=buffer._ptr;
             view->format   = (char*) buffer._format.c_str();
             view->internal = new Svar(ret);
             view->obj      = obj;
             view->itemsize = buffer.itemsize();
             view->len      = buffer.size();
             view->ndim     = buffer.shape.size();
             view->readonly = buffer._holder.isUndefined();
             view->shape    = buffer.shape.data();
             view->strides  = buffer.strides.data();
             incref(obj);
             return 0;
          };

          heap_type->as_buffer.bf_releasebuffer=[](PyObject *obj, Py_buffer *view)->void {
            delete (Svar*) view->internal;
          };
        }

        type->tp_new=[](PyTypeObject *type, PyObject *, PyObject *)->PyObject * {
            PyObject *self = type->tp_alloc(type, 0);
            auto inst = reinterpret_cast<SvarPy*>(self);
            inst->var=nullptr;
            return self;
        };

        type->tp_repr = [](PyObject* obj){
            std::stringstream sst;
            sst<<"<"<<obj->ob_type->tp_name<<" object at "<<obj<<">";
            return SVAR_FROM_STRING(sst.str().c_str());// TODO: this may safe?
        };

        type->tp_dealloc=[](PyObject *self) {
            SvarPy* obj=(SvarPy*)self;
            if(obj->var)
            {
                delete obj->var;
                obj->var=nullptr;
            }

            auto type = Py_TYPE(self);
            type->tp_free(self);
            Py_DECREF(type);
        };

        if (PyType_Ready(type) < 0)
            LOG(ERROR)<<("make_static_property_type(): failure in PyType_Ready()!");

        for(std::pair<std::string,Svar> f:cls._attr)
        {
            if(f.first == "__init__"){
                SvarFunction& func_old = f.second.as<SvarFunction>();
                SvarFunction func_new;
                func_new.name    = func_old.name;
                func_new.doc     = func_old.doc;
                func_new.meta    = func_old.meta;
                func_new.next    = func_old.next;
                func_new.arg_types = func_old.arg_types;
                func_new.kwargs = func_old.kwargs;
                func_new.do_argcheck=false;
                func_new._func  = [f,type](std::vector<Svar> args){
                    PyObject *self = type->tp_alloc(type, 0);
                    SvarPy* obj = reinterpret_cast<SvarPy*>(self);
                    std::vector<Svar> argv(args.begin()+1,args.end());
                    obj->var = new Svar(f.second.as<SvarFunction>().Call(argv));
                    return PyObjectHolder(obj,false);
                };
                PyObject_SetAttrString((PyObject*)type,"__new__",getPy(func_new));
                continue;
            }
            else
                PyObject_SetAttrString((PyObject*)type,f.first.c_str(),getPy(f.second));
        }

        PyObject_SetAttrString((PyObject *) type, "__module__", SVAR_FROM_STRING("svar_builtins"));
        cls.__setitem__ = type;

        PyObject* capsule=PyCapsule_New(new Svar(&cls),nullptr,[](PyObject *o) {
            delete (Svar*)PyCapsule_GetPointer(o, nullptr);
        });

        PyObject_SetAttrString((PyObject*)type,"svar_class",capsule);

        return type_incref((PyTypeObject*)type);
    }

    static Svar fromPy(PyObject* obj,bool abortComplex=false)
    {
        if(!obj)
          return Svar();

#if PY_MAJOR_VERSION < 3
        if (PyString_Check(obj)){
          return std::string(PyString_AsString(obj));
        }
#endif

        static std::unordered_map<PyTypeObject*,std::function<Svar(PyObject*)>> lut;
        if(lut.empty()){
            lut[&_PyNone_Type] =[](PyObject* o)->Svar{return Svar::Null();};
            lut[&PyBool_Type] =[](PyObject* o)->Svar{return (bool)PyLong_AsLong(o);};
            lut[&PyFloat_Type]=[](PyObject* o)->Svar{return PyFloat_AsDouble(o);};
            lut[&PyLong_Type] =[](PyObject* o)->Svar{return (int)PyLong_AsLong(o);};

            lut[&PyUnicode_Type] =[](PyObject* obj)->Svar{
                PyObjectHolder buf(PyUnicode_AsUTF8String(obj),false);
                char *buffer=nullptr;
                ssize_t length=0;

                if (SVAR_BYTES_AS_STRING_AND_SIZE(buf.obj, &buffer, &length))
                    LOG(ERROR)<<("Unable to extract string contents! (invalid type)");
                return std::string(buffer, (size_t) length);
            };

            lut[&PyList_Type] =[](PyObject* obj)->Svar{
                std::vector<Svar> array;
                int num=PyList_Size(obj);
                array.reserve(num);
                for(Py_ssize_t i=0;i<num;++i)
                    array.push_back(fromPy(PyList_GetItem(obj,i)));
                return Svar(array);
            };

            lut[&PyTuple_Type] =[](PyObject* obj)->Svar{
                std::vector<Svar> array;
                int num=PyTuple_Size(obj);
                for(Py_ssize_t i=0;i<num;++i)
                    array.push_back(fromPy(PyTuple_GetItem(obj,i)));
                return Svar(array);
            };

            lut[&PyDict_Type] =[](PyObject* obj)->Svar{
                Svar dict;
                PyObject *key, *value;
                Py_ssize_t pos = 0;

                while (PyDict_Next(obj, &pos, &key, &value)) {
                    if(!PyUnicode_Check(key)) continue;
                    dict[fromPy(key).unsafe_as<std::string>()] = fromPy(value);
                }
                return dict;
            };

            lut[&PyFunction_Type] =[](PyObject* obj)->Svar{
                SvarFunction func;

                Svar holder=PyObjectHolder(incref(obj));
                func._func=[holder](std::vector<Svar>& args)->Svar{
                    PyThreadStateLock PyThreadLock;
                    PyObjectHolder py_args=SvarPy::getPy(args);
                    return SvarPy::fromPy(PyObject_Call(holder.as<PyObjectHolder>().obj, py_args.obj, nullptr));
                };
                func.do_argcheck=false;
                func.doc="PyFunction";
                return func;
            };

            lut[&PyCFunction_Type] =[](PyObject* obj)->Svar{
                //                if(capsule) incref(capsule);
                SvarFunction func;
                Svar holder=PyObjectHolder(obj);
                func._func=[holder](std::vector<Svar>& args)->Svar{
                    PyThreadStateLock PyThreadLock;
                    PyObjectHolder py_args=SvarPy::getPy(args);
                    return SvarPy::fromPy(PyCFunction_Call(holder.as<PyObjectHolder>().obj, py_args.obj,nullptr));
                };
                func.do_argcheck=false;
                func.doc="PyCFunction";
                return func;
            };

            lut[&PyModule_Type] =[](PyObject* obj)->Svar{
                obj=PyModule_GetDict(obj);
                std::map<std::string,Svar> dict;
                PyObject *key, *value;
                Py_ssize_t pos = 0;

                while (PyDict_Next(obj, &pos, &key, &value)) {
                    auto keyvar=fromPy(key);
                    if(!keyvar.is<std::string>()){
                        LOG(INFO)<<keyvar<<" not string but is "<<key->ob_type->tp_name<<std::endl;
                        continue;
                    }
                    std::string keyStr=fromPy(key).castAs<std::string>();
                    if(keyStr.find_first_of("_")==0) continue;
                    if(PyModule_Check(value))
                        dict.insert(make_pair(keyStr,Svar(PyObjectHolder(obj))));
                    else
                        dict.insert(make_pair(keyStr,fromPy(value)));
                }
                decref(obj);
                return dict;
            };

            lut[&PyMemoryView_Type] = [](PyObject* obj)->Svar{
                PyMemoryViewObject* view_obj=(PyMemoryViewObject*)obj;
                Py_buffer& view=view_obj->view;
                return SvarBuffer(view.buf, view.itemsize, view.format,
                                  std::vector<ssize_t>(view.shape,view.shape+view.ndim),
                                  std::vector<ssize_t>(view.strides,view.strides+view.ndim),
                                  PyObjectHolder(obj));
            };

            lut[&PyBytes_Type]=[](PyObject* obj)->Svar{
                return SvarBuffer(PyBytes_AsString(obj),PyBytes_Size(obj));
            };

            lut[&PyType_Type] = [](PyObject* obj)->Svar{
                if(PyObject_HasAttrString((PyObject*)obj,"svar_class"))// this is a c++ class
                {
                    PyObject *capsule=PyObject_GetAttrString((PyObject*)obj,"svar_class");
                    Svar* o=(Svar*)PyCapsule_GetPointer(capsule,nullptr);
                    if(!o)
                    {
                        return Svar();
                    }
                    return *o;
                }
                return sv::Svar();
            };

        }

        PyTypeObject *type = Py_TYPE(obj);

        auto it=lut.find(type);
        if(it!=lut.end()){
            return it->second(obj);
        }

        if(PyObject_HasAttrString((PyObject*)type,"svar_class"))// this is a c++ object
        {
            SvarPy* o=(SvarPy*)obj;
            if(!o->var)
            {
               return Svar();
            }
            return *o->var;
        }

        return Svar(PyObjectHolder(obj));// may lead to segment fault?
    }

    static PyObjectHolder getModule(Svar var,std::string name="svar"){
        if(var.isObject())
        {
            PyObject* pyModule = nullptr;
#if PY_MAJOR_VERSION<3
            pyModule=Py_InitModule3(PythonSpace::safe_c_str(var.get<std::string>("__name__",name)),
                                    nullptr,
                                    PythonSpace::safe_c_str(var.get<std::string>("__doc__","")));
#else
            PyModuleDef *def = new PyModuleDef();
            memset(def, 0, sizeof(PyModuleDef));
            if(!var.exist("__name__")) var["__name__"]=name;
            if(!var.exist("__doc__"))  var["__doc__"] ="";
            def->m_name= var["__name__"].as<std::string>().c_str();
            def->m_doc = var["__doc__"].as<std::string>().c_str();
            def->m_size = -1;
            Py_INCREF(def);

            pyModule = PyModule_Create(def);
#endif
            if(!pyModule)
                return nullptr;

            for(std::pair<std::string,Svar> pair:var)
            {
                PyObject* obj=getModule(pair.second,pair.first);
                PyModule_AddObject(pyModule, pair.first.c_str(), obj);
            }

            return pyModule;
        }

        return getPy(var);
    }

};


}

#endif
