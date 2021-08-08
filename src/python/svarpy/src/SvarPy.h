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
        Py_Finalize();
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

    static PyObjectHolder getPy(Svar src)// This is for python to use, every python func will perform decref
    {
        if(src.is<PyObject*>())
            return PyObjectHolder(src.as<PyObject*>());

        if(src.is<PyObjectHolder>())
            return src.as<PyObjectHolder>();

        SvarClass& cls=src.classObject();
        Svar func=cls["getPy"];
        if(func.isFunction())
            return func.as<SvarFunction>().Call({src}).as<PyObjectHolder>();

        std::function<PyObjectHolder(Svar)> convert;

        if(src.is<bool>())
          convert=[](Svar src){return src.as<bool>()?Py_True:Py_False;};
        else if(src.is<int>())
            convert=[](Svar src){return PyObjectHolder(PyLong_FromLong(src.as<int>()),false);};
        else if(src.is<double>())
            convert=[](Svar src){return PyObjectHolder(PyFloat_FromDouble(src.as<double>()),false);};

        else if(src.isNull()||src.isUndefined())
            convert=[](Svar src){return Py_None;};

        else if(src.is<std::string>())
            convert=[](Svar src){
                std::string& str=src.as<std::string>();
                return PyObjectHolder(PyUnicode_FromStringAndSize(str.data(),str.size()),false);
            };
        else if(src.is<SvarArray>())
            convert=[](Svar src)->PyObjectHolder{

            PyObject* obj=PyTuple_New(src.length());
            size_t index = 0;
            for (Svar& value : src.as<SvarArray>()._var) {
                PyObject* value_ =SvarPy::getPy(value);
                if (!value_)
                {
                    decref(obj);
                    decref(value_);
                    return Py_None;
                }
                PyTuple_SetItem(obj, (ssize_t) index++, value_); // steals a reference
            }
            return PyObjectHolder(obj,false);
        };
        else if(src.is<SvarObject>())
            convert=[](Svar src)->PyObjectHolder{
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
                PyDict_SetItem(dict,key,value);
                decref(key);
                decref(value);
            }
            return PyObjectHolder(dict,false);
        };
        else if(src.is<SvarFunction>())
            convert=[](Svar src){return PyObjectHolder(SvarPy::getPyFunction(src),false);};
        else if(src.is<SvarClass>())
            convert=[](Svar src){ return PyObjectHolder((PyObject*)SvarPy::getPyClass(src));};
        else if(src.is<SvarClass::SvarProperty>()){
            convert=[](Svar src){return PyObjectHolder(SvarPy::getPyProperty(src));};
        }
        else convert=[](Svar src){
            PyTypeObject* py_class=getPyClass(src.classObject());
            PyObject *self = py_class->tp_alloc(py_class, 0);
            SvarPy* obj = reinterpret_cast<SvarPy*>(self);

//            SvarPy* obj=(SvarPy*)py_class->tp_new(py_class,nullptr,nullptr);
            obj->var=new Svar(src);
            return PyObjectHolder(obj,false);
        };

        cls.def("getPy",convert);

        return convert(src);
    }

    static PyObject* svar_func_wrapper(PyObject *capsule, PyObject *args, PyObject *kwargs){
        Svar* func=(Svar*)PyCapsule_GetPointer(capsule,"svar_function");
        SvarFunction& svarFunc=func->as<SvarFunction>();

        try{
            Svar svar_args=SvarPy::fromPy(args);
            if(kwargs){
                Svar svar_kwargs=SvarPy::fromPy(kwargs);
                for(std::pair<std::string,Svar> kw:svar_kwargs)
                    svar_args.push_back(arg(kw.first)=kw.second);
            }
            if(svarFunc.is_constructor)// FIXME: constructor is different since first argument is self?
            {
                return SvarPy::getPy(svarFunc.Call({svar_args}));
            }

            return SvarPy::getPy(svarFunc.Call(svar_args.as<SvarArray>()._var));
        }
        catch(SvarExeption e){
            PyErr_SetString(PyExc_RuntimeError, e.what());
            return incref(Py_None);
        }
    }

    static PyObject* getPyProperty(Svar src){
      SvarClass::SvarProperty& property=src.as<SvarClass::SvarProperty>();
      auto py_args=getPy({property._fget,property._fset,property._doc});
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
        SvarFunction& svar_func=src.as<SvarFunction>();
        PyMethodDef* func = new PyMethodDef();
        std::memset(func, 0, sizeof(PyMethodDef));
        func->ml_name = svar_func.name.c_str();
        func->ml_flags = METH_VARARGS| METH_KEYWORDS ;
        if(svar_func.is_constructor)
            func->ml_flags|=METH_CLASS;
        if(svar_func.arg_types.size()){
            svar_func.meta["full_doc"]=svar_func.signature();
            func->ml_doc=svar_func.meta["full_doc"].as<std::string>().c_str();
        }
        else
            func->ml_doc="";

        func->ml_meth = reinterpret_cast<PyCFunction>(svar_func_wrapper);

        PyObject* capsule=PyCapsule_New(new Svar(src),"svar_function",[](PyObject *o) {
            delete (Svar*)PyCapsule_GetPointer(o, "svar_function");
        });

        auto c_func=PyCFunction_NewEx(func,capsule,nullptr);
        decref(capsule);
        if(!svar_func.is_method)
            return c_func;

        auto m_ptr = SVAR_INSTANCE_METHOD_NEW(c_func, nullptr);
        if (!m_ptr)
            std::cerr<<("cpp_function::cpp_function(): Could not allocate instance method object");
//        Py_DECREF(c_func);
        return m_ptr;
    }

    static PyTypeObject* getPyClass(Svar var){// FIXME: segment fault when python cleaning
        SvarClass& cls=var.as<SvarClass>();
        if(cls._attr.exist("PyTypeObject"))
            return cls._attr["PyTypeObject"].castAs<PyTypeObject*>();

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

        if(cls._attr["__buffer__"].isFunction()){
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
            // Allocate the value/holder internals:
            inst->var=nullptr;
//            DLOG(INFO)<<"Creating <"<<type->tp_name<<" object at "<<self<<">";
            return self;
        };

        type->tp_init=[](PyObject *self, PyObject *, PyObject *)->int{
            // In fact this function should never be called
            PyTypeObject *type = Py_TYPE(self);
            static std::string msg;
            msg="";
        #if defined(PYPY_VERSION)
            msg += handle((PyObject *) type).attr("__module__").cast<std::string>() + ".";
        #endif
            msg += type->tp_name;
            msg += ": No constructor defined!";
            PyErr_SetString(PyExc_TypeError, msg.c_str());
            return -1;
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
//                DLOG(INFO)<<"Deleting <"<<self->ob_type->tp_name<<" object at "<<self<<">";
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
            if(f.first=="__init__"){
                Svar func_init=Svar::lambda([f](std::vector<Svar> args){
                        SvarPy* self=(SvarPy*)args[0].as<PyObjectHolder>().obj;
                        std::vector<Svar> used_args(args.begin()+1,args.end());
                        Svar ret=f.second.as<SvarFunction>().Call(used_args);
                        self->var=new Svar(ret);
                        return incref(Py_None);
            });
                func_init.as<SvarFunction>().is_constructor=true;
                func_init.as<SvarFunction>().is_method=true;
                func_init.as<SvarFunction>().arg_types=f.second.as<SvarFunction>().arg_types;
                func_init.as<SvarFunction>().do_argcheck=false;
                PyObject_SetAttrString((PyObject*)type,"__init__",getPy(func_init));
                continue;
            }
            else
                PyObject_SetAttrString((PyObject*)type,f.first.c_str(),getPy(f.second));
        }

        PyObject_SetAttrString((PyObject *) type, "__module__", SVAR_FROM_STRING("svar_builtins"));
        cls._attr.set("PyTypeObject",type);

        PyObject* capsule=PyCapsule_New(new Svar(var),nullptr,[](PyObject *o) {
            delete (Svar*)PyCapsule_GetPointer(o, nullptr);
        });

        PyObject_SetAttrString((PyObject*)type,"svar_class",capsule);

        return type_incref((PyTypeObject*)type);
    }

    static Svar fromPy(PyObject* obj,bool abortComplex=false)// this never fails
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
                PyObject* buf=PyUnicode_AsUTF8String(obj);
                char *buffer=nullptr;
                ssize_t length=0;

                if (SVAR_BYTES_AS_STRING_AND_SIZE(buf, &buffer, &length))
                    LOG(ERROR)<<("Unable to extract string contents! (invalid type)");
                return std::string(buffer, (size_t) length);
            };

            lut[&PyList_Type] =[](PyObject* obj)->Svar{
                std::vector<Svar> array(PyList_Size(obj));
                for(Py_ssize_t i=0;i<array.size();++i)
                    array[i]=fromPy(PyList_GetItem(obj,i));
                return Svar(array);
            };

            lut[&PyTuple_Type] =[](PyObject* obj)->Svar{
                std::vector<Svar> array(PyTuple_Size(obj));
                for(Py_ssize_t i=0;i<array.size();++i)
                    array[i]=fromPy(PyTuple_GetItem(obj,i));
                return Svar(array);
            };

            lut[&PyDict_Type] =[](PyObject* obj)->Svar{
                Svar dict;
                PyObject *key, *value;
                Py_ssize_t pos = 0;

                while (PyDict_Next(obj, &pos, &key, &value)) {
                    dict[fromPy(key).castAs<std::string>()] = fromPy(value);
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
                incref(obj);
                std::map<std::string,Svar> dict;
                PyObject *key, *value;
                Py_ssize_t pos = 0;

                while (PyDict_Next(obj, &pos, &key, &value)) {
                    std::string keyStr=fromPy(key).castAs<std::string>();
                    if(keyStr.find_first_of("_")==0) continue;
                    if(PyModule_Check(value)) continue;
                    dict.insert(make_pair(keyStr,
                                          fromPy(value)));
                }
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

        return Svar();// FIXME: Svar(PyObjectHolder(obj)) may lead to segment fault?
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
