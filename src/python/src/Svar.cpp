#define __SVAR_BUILDIN__
#include <Svar/Svar.h>
#include <Svar/SharedLibrary.h>
#include <Python.h>
#include <cstring>

#if PY_MAJOR_VERSION >= 3 /// Compatibility macros for various Python versions
#define SVAR_INSTANCE_METHOD_NEW(ptr, class_) PyInstanceMethod_New(ptr)
#define SVAR_INSTANCE_METHOD_CHECK PyInstanceMethod_Check
#define SVAR_INSTANCE_METHOD_GET_FUNCTION PyInstanceMethod_GET_FUNCTION
#define SVAR_BYTES_CHECK PyBytes_Check
#define SVAR_BYTES_FROM_STRING PyBytes_FromString
#define SVAR_BYTES_FROM_STRING_AND_SIZE PyBytes_FromStringAndSize
#define SVAR_BYTES_AS_STRING_AND_SIZE PyBytes_AsStringAndSize
#define SVAR_BYTES_AS_STRING PyBytes_AsString
#define SVAR_BYTES_SIZE PyBytes_Size
#define SVAR_LONG_CHECK(o) PyLong_Check(o)
#define SVAR_LONG_AS_LONGLONG(o) PyLong_AsLongLong(o)
#define SVAR_LONG_FROM_SIGNED(o) PyLong_FromSsize_t((ssize_t) o)
#define SVAR_LONG_FROM_UNSIGNED(o) PyLong_FromSize_t((size_t) o)
#define SVAR_BYTES_NAME "bytes"
#define SVAR_STRING_NAME "str"
#define SVAR_SLICE_OBJECT PyObject
#define SVAR_FROM_STRING PyUnicode_FromString
#define SVAR_STR_TYPE ::SVAR::str
#define SVAR_BOOL_ATTR "__bool__"
#define SVAR_NB_BOOL(ptr) ((ptr)->nb_bool)
#define SVAR_PLUGIN_IMPL(name) \
    extern "C" SVAR_EXPORT PyObject *PyInit_##name()

#else
#define SVAR_INSTANCE_METHOD_NEW(ptr, class_) PyMethod_New(ptr, nullptr, class_)
#define SVAR_INSTANCE_METHOD_CHECK PyMethod_Check
#define SVAR_INSTANCE_METHOD_GET_FUNCTION PyMethod_GET_FUNCTION
#define SVAR_BYTES_CHECK PyString_Check
#define SVAR_BYTES_FROM_STRING PyString_FromString
#define SVAR_BYTES_FROM_STRING_AND_SIZE PyString_FromStringAndSize
#define SVAR_BYTES_AS_STRING_AND_SIZE PyString_AsStringAndSize
#define SVAR_BYTES_AS_STRING PyString_AsString
#define SVAR_BYTES_SIZE PyString_Size
#define SVAR_LONG_CHECK(o) (PyInt_Check(o) || PyLong_Check(o))
#define SVAR_LONG_AS_LONGLONG(o) (PyInt_Check(o) ? (long long) PyLong_AsLong(o) : PyLong_AsLongLong(o))
#define SVAR_LONG_FROM_SIGNED(o) PyInt_FromSsize_t((ssize_t) o) // Returns long if needed.
#define SVAR_LONG_FROM_UNSIGNED(o) PyInt_FromSize_t((size_t) o) // Returns long if needed.
#define SVAR_BYTES_NAME "str"
#define SVAR_STRING_NAME "unicode"
#define SVAR_SLICE_OBJECT PySliceObject
#define SVAR_FROM_STRING PyString_FromString
#define SVAR_STR_TYPE ::SVAR::bytes
#define SVAR_BOOL_ATTR "__nonzero__"
#define SVAR_NB_BOOL(ptr) ((ptr)->nb_nonzero)
#define SVAR_PLUGIN_IMPL(name) \
    static PyObject *SVAR_init_wrapper();               \
    extern "C" SVAR_EXPORT void init##name() {          \
        (void)SVAR_init_wrapper();                      \
    }                                                       \
    PyObject *SVAR_init_wrapper()
#endif

using namespace GSLAM;

struct PyObjectHolder{
    PyObjectHolder(PyObject* o):obj(o){
        Py_IncRef(obj);
    }
    ~PyObjectHolder(){
        Py_DecRef(obj);
    }
    PyObject* obj;
};

struct SvarPy: public PyObject{
    Svar* var;

    static PyTypeObject& default_type()
    {
        static PyTypeObject* type=nullptr;
        if(type) return *type;
        constexpr auto *name = "svar_object";
        auto name_obj = SVAR_FROM_STRING(name);

        /* Danger zone: from now (and until PyType_Ready), make sure to
           issue no Python C API calls which could potentially invoke the
           garbage collector (the GC will call type_traverse(), which will in
           turn find the newly constructed type in an invalid state) */
        auto heap_type = (PyHeapTypeObject *) PyType_Type.tp_alloc(&PyType_Type, 0);
        if (!heap_type)
            std::cerr<<("make_default_metaclass(): error allocating metaclass!");

        heap_type->ht_name = name_obj;

        type = &heap_type->ht_type;
        type->tp_name = name;
        type->tp_base = type_incref(&PyType_Type);
        type->tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HEAPTYPE;

//        type->tp_setattro = pybind11_meta_setattro;
//    #if PY_MAJOR_VERSION >= 3
//        type->tp_getattro = pybind11_meta_getattro;
//    #endif

        if (PyType_Ready(type) < 0)
            std::cerr<<("make_default_metaclass(): failure in PyType_Ready()!");

        PyObject_SetAttrString((PyObject *) type, "__module__", PyUnicode_FromString("svar_builtin"));
        return *type;
    }

    static PyTypeObject* type_incref(PyTypeObject* obj){
        Py_IncRef((PyObject*)obj);
        return obj;
    }

    static PyTypeObject* type_decref(PyTypeObject* obj){
        Py_DecRef((PyObject*)obj);
        return obj;
    }

    static PyObject* getPy(Svar src){
        if(src.is<PyObjectHolder>())
            return src.as<PyObjectHolder>().obj;

        SvarClass* cls=src.classPtr();
        Svar func=(*cls)["getPy"];
        if(func.isFunction())
            return func(src).as<PyObject*>();

        PyTypeObject* py_class=getPyClass(src.classObject());
        SvarPy* obj=(SvarPy*)py_class->tp_new(py_class,nullptr,nullptr);
        obj->var=new Svar(src);

        return (PyObject*)obj;
    }

    static PyTypeObject* getPyClass(Svar var){
        SvarClass& cls=var.as<SvarClass>();
        if(cls._attr.exist("PyTypeObject"))
            return cls._attr["PyTypeObject"].castAs<PyTypeObject*>();

//        return (PyTypeObject*)Py_None;

        auto heap_type = (PyHeapTypeObject *) default_type().tp_alloc(&default_type(), 0);
        if (!heap_type){
            std::cerr<<("Error allocating type!")<<std::endl;
            return (PyTypeObject*)Py_None;
        }

        heap_type->ht_name = SVAR_FROM_STRING(cls.__name__.c_str());

        PyTypeObject* type = &heap_type->ht_type;
        type->tp_name = cls.__name__.c_str();
        type->tp_base = type_incref(&PyBaseObject_Type);
        type->tp_basicsize=sizeof(SvarPy);
        type->tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HEAPTYPE;
        type->tp_doc="";
        type->tp_repr = [](PyObject* obj){
            std::stringstream sst;
            sst<<"<"<<obj->ob_type->tp_name<<" object at "<<obj<<">";
            return SVAR_FROM_STRING(sst.str().c_str());
        };

        type->tp_as_number = &heap_type->as_number;
        type->tp_as_sequence = &heap_type->as_sequence;
        type->tp_as_mapping = &heap_type->as_mapping;

        type->tp_new=[](PyTypeObject *type, PyObject *, PyObject *)->PyObject * {
    #if defined(PYPY_VERSION)
            // PyPy gets tp_basicsize wrong (issue 2482) under multiple inheritance when the first inherited
            // object is a a plain Python type (i.e. not derived from an extension type).  Fix it.
            ssize_t instance_size = static_cast<ssize_t>(sizeof(instance));
            if (type->tp_basicsize < instance_size) {
                type->tp_basicsize = instance_size;
            }
    #endif
            PyObject *self = type->tp_alloc(type, 0);
            auto inst = reinterpret_cast<SvarPy*>(self);
            // Allocate the value/holder internals:
            inst->var=nullptr;
            std::cerr<<"Creating "<<type->tp_name<<std::endl;
            return self;
        };

        type->tp_init=[](PyObject *self, PyObject *, PyObject *)->int{
            PyTypeObject *type = Py_TYPE(self);
            std::string msg;
        #if defined(PYPY_VERSION)
            msg += handle((PyObject *) type).attr("__module__").cast<std::string>() + ".";
        #endif
            msg += type->tp_name;
            msg += ": No constructor defined!";
            PyErr_SetString(PyExc_TypeError, msg.c_str());
            return -1;
        };

        type->tp_dealloc=[](PyObject *self) {
            SvarPy* obj=(SvarPy*)self;
            if(obj->var)
            {
                std::cerr<<"Deleting "<<*obj->var;
                delete obj->var;
                obj->var=nullptr;
            }

            auto type = Py_TYPE(self);
            type->tp_free(self);
            Py_DECREF(type);
        };

        if (PyType_Ready(type) < 0)
            std::cerr<<("make_static_property_type(): failure in PyType_Ready()!");

        PyObject_SetAttrString((PyObject *) type, "__module__", SVAR_FROM_STRING("svar_builtins"));
        cls._attr.set("PyTypeObject",type);
        for(std::pair<std::string,Svar> f:cls._methods.as<SvarObject>()._var)
        {
            if(f.first=="__init__"){
                Svar func_init=Svar::lambda([f](std::vector<Svar> args){
                        SvarPy* self=args[0].as<SvarPy*>();
                        std::vector<Svar> used_args(args.begin()+1,args.end());
                        Svar ret=f.second.as<SvarFunction>().Call(used_args);
                        self->var=new Svar(ret);
                        Py_IncRef(self);
                        return PyObjectHolder(Py_None);
            });
                func_init.as<SvarFunction>().is_constructor=true;
                func_init.as<SvarFunction>().is_method=true;
                PyObject_SetAttrString((PyObject*)type,"__init__",getPy(func_init));
                continue;
            }
            PyObject_SetAttrString((PyObject*)type,f.first.c_str(),getPy(f.second));
        }

        PyObject* capsule=PyCapsule_New(new Svar(var),nullptr,[](PyObject *o) {
            delete (Svar*)PyCapsule_GetPointer(o, nullptr);
        });

        PyObject_SetAttrString((PyObject*)type,"svar_class",capsule);


        return (PyTypeObject*)type;
    }

    static Svar fromPy(PyObject* obj)// this never fails
    {
        if(PyNumber_Check(obj))
        {
            if(PyFloat_Check(obj))
                return PyFloat_AsDouble(obj);
            else return (int)PyLong_AsLong(obj);
        }

        if(PyList_Check(obj)){
            std::vector<Svar> array(PyList_Size(obj));
            for(Py_ssize_t i=0;i<array.size();++i)
                array[i]=fromPy(PyList_GetItem(obj,i));
            return Svar(array);
        }

        if (PyUnicode_Check(obj)) {
            PyObject* buf=PyUnicode_AsUTF8String(obj);
            char *buffer=nullptr;
            ssize_t length=0;

            if (SVAR_BYTES_AS_STRING_AND_SIZE(buf, &buffer, &length))
                std::cerr<<("Unable to extract string contents! (invalid type)");
            return std::string(buffer, (size_t) length);
        }

        if(PyTuple_Check(obj)){
            std::vector<Svar> array(PyTuple_Size(obj));
            for(Py_ssize_t i=0;i<array.size();++i)
                array[i]=fromPy(PyTuple_GetItem(obj,i));
            return Svar(array);
        }

        if(PyDict_Check(obj)){
            std::map<std::string,Svar> dict;
            PyObject *key, *value;
            Py_ssize_t pos = 0;

            while (PyDict_Next(obj, &pos, &key, &value)) {
                dict.insert(make_pair(fromPy(key).castAs<std::string>(),
                                      fromPy(value)));
            }
            return dict;
        }


        PyTypeObject *type = Py_TYPE(obj);

        if(PyObject_HasAttrString((PyObject*)type,"svar_class"))
        {
    //        PyObject *capsule=PyObject_GetAttrString((PyObject*)type,"svar_class");
    //        assert(PyCapsule_GetPointer(capsule,nullptr));
            SvarPy* o=(SvarPy*)obj;
            if(!o->var)
            {
               return PyObjectHolder(obj);
            }
            return *o->var;
        }

        return Svar(PyObjectHolder(obj));
    }


    static PyObject* getModule(GSLAM::Svar var){
        if(var.isObject())
        {
            PyModuleDef *def = new PyModuleDef();
            memset(def, 0, sizeof(PyModuleDef));
            if(var.exist("__name__"))
                def->m_name = var["__name__"].castAs<std::string>().c_str();
            else def->m_name="svar";
            if(var.exist("__doc__"))
                def->m_doc =  var["__doc__"].castAs<std::string>().c_str();
            else def->m_doc="";
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

};

PyObject* load(std::string pluginPath){
    SharedLibraryPtr plugin(new SharedLibrary(pluginPath));
    if(!plugin->isLoaded()){
        std::cerr<<"Unable to load plugin "<<pluginPath<<std::endl;
        return Py_None;
    }

    GSLAM::Svar* (*getInst)()=(GSLAM::Svar* (*)())plugin->getSymbol("svarInstance");
    if(!getInst){
        std::cerr<<"No svarInstance found in "<<pluginPath<<std::endl;
        return Py_None;
    }
    GSLAM::Svar* inst=getInst();
    if(!inst){
        std::cerr<<"svarInstance returned null.\n";
        return Py_None;
    }
    return SvarPy::getModule(*inst);
}


class BBase{
public:
    virtual bool isBBase(){return true;}
};

class BaseClass:public BBase{
public:
    BaseClass():age_(0){}
    BaseClass(int age):age_(age){}

    int getAge()const{return age_;}
    void setAge(int a){age_=a;}

    static BaseClass create(int a){return BaseClass(a);}
    static BaseClass create1(){return BaseClass();}

    virtual std::string intro()const{return "age:"+std::to_string(age_);}
    virtual bool isBBase(){return false;}

    int age_;
};

class BaseClass1{
public:
    BaseClass1():score(100){}

    double getScore(){return score;}

    double score;
};

class InheritClass: public BaseClass,public BaseClass1
{
public:
    InheritClass(int age,std::string name):BaseClass(age),name_(name){}

    std::string name()const{return name_;}
    void setName(std::string name){name_=name;}

    virtual std::string intro() const{return BaseClass::intro()+", name:"+name_;}

    std::string* pointer(){return &name_;}
    std::string name_;
};

REGISTER_SVAR_MODULE(python_builtin)
{
    SvarClass::Class<InheritClass>()
            .def("__init__",[](int age,std::string name){return InheritClass(age,name);})
            .def("name",&InheritClass::name)
            .def("intro",&InheritClass::intro)
            .def("pointer",&InheritClass::pointer);

    svar["agename"]=SvarClass::instance<InheritClass>();

    svar["int"]=SvarClass::instance<int>();
    svar.set("load",Svar(load));

    std::string getPyName="getPy";
    SvarClass::Class<int>()
            .def_static(getPyName,[](Svar src){return PyLong_FromLong(src.as<int>());});

    Svar(1.).classPtr()
            ->def_static(getPyName,[](Svar src){return PyFloat_FromDouble(src.as<double>());});

    Svar::Null().classPtr()
            ->def_static(getPyName,[](Svar src){return Py_None;});

    SvarClass::Class<std::string>()
            .def_static(getPyName,[](Svar src){
        std::string& str=src.as<std::string>();
        return PyUnicode_FromStringAndSize(str.data(),str.size());
    });

    SvarClass::Class<SvarArray>()
            .def_static(getPyName,[](Svar src){
        PyObject* obj=PyList_New(src.length());
        size_t index = 0;
        for (Svar& value : src.as<SvarArray>()._var) {
            PyObject* value_ =SvarPy::getPy(value);
            if (!value_)
            {
                delete value_;
                return Py_None;
            }
            PyList_SET_ITEM(obj, (ssize_t) index++, value_); // steals a reference
        }
        return obj;
    });

    SvarClass::Class<SvarObject>()
            .def_static(getPyName,[](Svar src){
        PyObject* dict=PyDict_New();
        for (auto kv : src.as<SvarObject>()._var) {
            PyObject* key = PyUnicode_FromStringAndSize(kv.first.data(),kv.first.size());
            PyObject* value = SvarPy::getPy(kv.second);
            if (!key || !value)
                return Py_None;
            PyDict_SetItem(dict,key,value);
        }
        return dict;
    });

    SvarClass::Class<SvarFunction>()
            .def_static(getPyName,[](Svar src){
        SvarFunction& svar_func=src.as<SvarFunction>();
        PyMethodDef* func = new PyMethodDef();
        std::memset(func, 0, sizeof(PyMethodDef));
        func->ml_name = svar_func.name.c_str();
        func->ml_flags = METH_VARARGS| METH_KEYWORDS ;
        if(svar_func.is_constructor)
            func->ml_flags|=METH_CLASS;
        func->ml_doc=svar_func.signature.c_str();

        func->ml_meth = [](PyObject *capsule, PyObject *args)->PyObject*{
            Svar* func=(Svar*)PyCapsule_GetPointer(capsule,nullptr);
            SvarFunction& svarFunc=func->as<SvarFunction>();
            Svar svar_args=SvarPy::fromPy(args);
            std::cerr<<svar_args;
            if(svarFunc.is_constructor)
            {
                return SvarPy::getPy(svarFunc.Call({svar_args}));
            }

            return SvarPy::getPy(svarFunc.Call(svar_args.as<SvarArray>()._var));
        };

        PyObject* capsule=PyCapsule_New(new Svar(src),nullptr,[](PyObject *o) {
            delete (Svar*)PyCapsule_GetPointer(o, nullptr);
        });

        auto c_func=PyCFunction_NewEx(func,capsule,nullptr);
        if(!svar_func.is_method)
            return c_func;

        auto m_ptr = SVAR_INSTANCE_METHOD_NEW(c_func, nullptr);
        if (!m_ptr)
            std::cerr<<("cpp_function::cpp_function(): Could not allocate instance method object");
        Py_DECREF(c_func);
        return m_ptr;
    });

    SvarClass::Class<SvarClass>()
            .def_static(getPyName,[](Svar src){
        return SvarPy::getPyClass(src);
    });
}

SVAR_PLUGIN_IMPL(svar)
{
    return SvarPy::getModule(svar);
}
