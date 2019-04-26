#include "pybind11/pybind11.h"
#include "pybind11/stl.h"
#include <pybind11/functional.h>
#define __SVAR_BUILDIN__
#include "../../Svar/Svar.h"
#include "../../Svar/SharedLibrary.h"

using namespace GSLAM;
namespace py = pybind11;

NAMESPACE_BEGIN(PYBIND11_NAMESPACE)
NAMESPACE_BEGIN(detail)

template <> class type_caster<Svar> {
public:
    bool load(handle src, bool convert) {
        if (!src) return false;
        type_caster_base<Svar> loader;
        if(loader.load(src,convert))
        {
            value=loader;
            return true;
        }
        if(PyNumber_Check(src.ptr()))
        {
            if(PyFloat_Check(src.ptr())) value=src.cast<double>();
            else value=src.cast<int>();
            return true;
        }
        if(PyList_Check(src.ptr())){
            value=src.cast<std::vector<Svar> >();
            return true;
        }
        if(PyTuple_Check(src.ptr())){
            value=src.cast<std::vector<Svar> >();
            return true;
        }
        if(PyDict_Check(src.ptr())){
            value=src.cast<std::map<std::string,Svar> >();
            return true;
        }
        if(PyUnicode_Check(src.ptr()))
        {
            value=src.cast<std::string>();
            return true;
        }

        value=Svar();
        return true;
    }

    static handle cast(Svar src, return_value_policy policy, handle parent) {
        return type_caster_base<Svar>::cast(src,policy,parent);
    }

    PYBIND11_TYPE_CASTER(Svar, _("Svar"));

};

NAMESPACE_END(detail)
NAMESPACE_END(PYBIND11_NAMESPACE)

class svar_function: public py::cpp_function
{
public:
    svar_function(Svar src){
        using namespace pybind11;
        using namespace pybind11::detail;
        assert(src.isFunction());
        SvarFunction& svarFunc=src.as<SvarFunction>();
        std::function<Svar(const std::vector<Svar>&)> f=[src](const std::vector<Svar>& args)
        {
            return src.as<SvarFunction>().Call(args);
        };

        struct capture { std::function<Svar(const std::vector<Svar>&)> f; };

        /* Store the function including any extra state it might have (e.g. a lambda capture object) */
        py::detail::function_record* rec = make_function_record();

        rec->data[0] = new capture { std::forward<std::function<Svar(const std::vector<Svar>&)>>(f) };
        rec->free_data = [](function_record *r) { delete ((capture *) r->data[0]); };

        /* Type casters for the function arguments and return value */
//        using cast_in  = argument_loader<Args...>;
        using cast_out = make_caster<Svar>;

        /* Dispatch code which converts function arguments and performs the actual function call */
        rec->impl = [](function_call &call) -> handle {
//            LOG(INFO)<<call.args.size();
            for(auto arg:call.args){
//                LOG(INFO)<<arg.cast<Svar>();
            }
            std::vector<Svar> args;
            if(call.args.size()==1)
                args=call.args[0].cast<std::vector<Svar> >();
            else {
                args=call.args[1].cast<std::vector<Svar> >();
                args.insert(args.begin(),call.args[0].cast<Svar>());
            }

            /* Get a pointer to the capture object */
            auto data = (sizeof(capture) <= sizeof(call.func.data)
                         ? &call.func.data : call.func.data[0]);
            capture *cap = const_cast<capture *>(reinterpret_cast<const capture *>(data));

            /* Override policy for rvalues -- usually to enforce rvp::move on an rvalue */
            return_value_policy policy = return_value_policy_override<Svar>::policy(call.func.policy);

            /* Function scope guard -- defaults to the compile-to-nothing `void_type` */
//            using Guard = extract_guard_t<Extra...>;

            /* Perform the function call */
            handle result = cast_out::cast(cap->f(args), policy, call.parent);

            return result;
        };

        std::stringstream sst;
        sst<<src.as<SvarFunction>();
        /* Register the function with Python from generic (non-templated) code */
        initialize_generic(rec, sst.str().c_str(),svarFunc);

        rec->has_args = svarFunc.nargs;
        rec->has_kwargs = false;

//        /* Stash some additional information used by an important optimization in 'functional.h' */
//        constexpr bool is_function_ptr =sizeof(capture) == sizeof(void *);
//        if (is_function_ptr) {
//            rec->is_stateless = true;
//            rec->data[1] = const_cast<void *>(reinterpret_cast<const void *>(&typeid(FunctionType)));
//        }
    }

    /// Register a function call with Python (generic non-templated code goes here)
    void initialize_generic(py::detail::function_record *rec, const std::string& text, SvarFunction& svarFunc) {

        /* Create copies of all referenced C-style strings */
        rec->name = strdup(rec->name ? rec->name : "");
        if (rec->doc) rec->doc = strdup(rec->doc);
        for (auto &a: rec->args) {
            if (a.name)
                a.name = strdup(a.name);
            if (a.descr)
                a.descr = strdup(a.descr);
            else if (a.value)
                a.descr = strdup(a.value.attr("__repr__")().cast<std::string>().c_str());
        }

        rec->is_constructor = !strcmp(rec->name, "__init__") || !strcmp(rec->name, "__setstate__");
        rec->is_method=svarFunc.is_method;

        /* Generate a proper function signature */
        std::string signature=text;

#if PY_MAJOR_VERSION < 3
        if (strcmp(rec->name, "__next__") == 0) {
            std::free(rec->name);
            rec->name = strdup("next");
        } else if (strcmp(rec->name, "__bool__") == 0) {
            std::free(rec->name);
            rec->name = strdup("__nonzero__");
        }
#endif
        rec->signature = strdup(signature.c_str());
        rec->args.shrink_to_fit();
        rec->nargs = (std::uint16_t) svarFunc.nargs;

        if (rec->sibling && PYBIND11_INSTANCE_METHOD_CHECK(rec->sibling.ptr()))
            rec->sibling = PYBIND11_INSTANCE_METHOD_GET_FUNCTION(rec->sibling.ptr());

        py::detail::function_record *chain = nullptr, *chain_start = rec;
        if (rec->sibling) {
            if (PyCFunction_Check(rec->sibling.ptr())) {
                auto rec_capsule = py::reinterpret_borrow<py::capsule>(PyCFunction_GET_SELF(rec->sibling.ptr()));
                chain = (py::detail::function_record *) rec_capsule;
                /* Never append a method to an overload chain of a parent class;
                   instead, hide the parent's overloads in this case */
                if (!chain->scope.is(rec->scope))
                    chain = nullptr;
            }
            // Don't trigger for things like the default __init__, which are wrapper_descriptors that we are intentionally replacing
            else if (!rec->sibling.is_none() && rec->name[0] != '_')
                py::pybind11_fail("Cannot overload existing non-function object \"" + std::string(rec->name) +
                        "\" with a function of the same name");
        }

        if (!chain) {
            /* No existing overload was found, create a new function object */
            rec->def = new PyMethodDef();
            std::memset(rec->def, 0, sizeof(PyMethodDef));
            rec->def->ml_name = rec->name;
            rec->def->ml_meth = reinterpret_cast<PyCFunction>(reinterpret_cast<void (*) (void)>(*dispatcher));
            rec->def->ml_flags = METH_VARARGS | METH_KEYWORDS;

            py::capsule rec_capsule(rec, [](void *ptr) {
                destruct((py::detail::function_record *) ptr);
            });

            py::object scope_module;
            if (rec->scope) {
                if (hasattr(rec->scope, "__module__")) {
                    scope_module = rec->scope.attr("__module__");
                } else if (hasattr(rec->scope, "__name__")) {
                    scope_module = rec->scope.attr("__name__");
                }
            }

            m_ptr = PyCFunction_NewEx(rec->def, rec_capsule.ptr(), scope_module.ptr());
            if (!m_ptr)
                py::pybind11_fail("cpp_function::cpp_function(): Could not allocate function object");
        } else {
            /* Append at the end of the overload chain */
            m_ptr = rec->sibling.ptr();
            inc_ref();
            chain_start = chain;
            if (chain->is_method != rec->is_method)
                py::pybind11_fail("overloading a method with both static and instance methods is not supported; "
                    #if defined(NDEBUG)
                        "compile in debug mode for more details"
                    #else
                        "error while attempting to bind " + std::string(rec->is_method ? "instance" : "static") + " method " +
                        std::string(pybind11::str(rec->scope.attr("__name__"))) + "." + std::string(rec->name) + signature
                    #endif
                );
            while (chain->next)
                chain = chain->next;
            chain->next = rec;
        }
        using namespace py;

        std::string signatures;
        int index = 0;
        /* Create a nice pydoc rec including all signatures and
           docstrings of the functions in the overload chain */
        if (chain && py::options::show_function_signatures()) {
            // First a generic signature
            signatures += rec->name;
            signatures += "(*args, **kwargs)\n";
            signatures += "Overloaded function.\n\n";
        }
        // Then specific overload signatures
        bool first_user_def = true;
        for (auto it = chain_start; it != nullptr; it = it->next) {
            if (py::options::show_function_signatures()) {
                if (index > 0) signatures += "\n";
                if (chain)
                    signatures += std::to_string(++index) + ". ";
                signatures += rec->name;
                signatures += it->signature;
                signatures += "\n";
            }
            if (it->doc && strlen(it->doc) > 0 && py::options::show_user_defined_docstrings()) {
                // If we're appending another docstring, and aren't printing function signatures, we
                // need to append a newline first:
                if (!py::options::show_function_signatures()) {
                    if (first_user_def) first_user_def = false;
                    else signatures += "\n";
                }
                if (options::show_function_signatures()) signatures += "\n";
                signatures += it->doc;
                if (options::show_function_signatures()) signatures += "\n";
            }
        }

        /* Install docstring */
        PyCFunctionObject *func = (PyCFunctionObject *) m_ptr;
        if (func->m_ml->ml_doc)
            std::free(const_cast<char *>(func->m_ml->ml_doc));
        func->m_ml->ml_doc = strdup(signatures.c_str());

        if (rec->is_method) {
            m_ptr = PYBIND11_INSTANCE_METHOD_NEW(m_ptr, rec->scope.ptr());
            if (!m_ptr)
                pybind11_fail("cpp_function::cpp_function(): Could not allocate instance method object");
            Py_DECREF(func);
        }
    }

};

py::handle getPy(Svar src){
    SvarClass* cls=src.classPtr();
    Svar func=(*cls)["getPy"];
    if(func.isFunction())
        return func(src).as<py::handle>();

    std::function<py::handle(Svar)> convert;
    if(src.isNull())
        convert=[](Svar src){return pybind11::cast(nullptr);};
    else if(src.is<int>())
        convert=[](Svar src){return PyLong_FromLong(src.as<int>());};
    else if(src.is<double>())
        convert=[](Svar src){return PyFloat_FromDouble(src.as<double>());};
    else if(src.is<std::string>())
        convert=[](Svar src){
        return py::str(src.as<std::string>()).release();
    };
    else if(src.isArray())
        convert=[](Svar src){
        py::list l(src.length());
        size_t index = 0;
        for (Svar& value : src.as<SvarArray>()._var) {
            auto value_ = py::reinterpret_steal<py::object>(getPy(value));
            if (!value_)
                return py::handle();
            PyList_SET_ITEM(l.ptr(), (ssize_t) index++, value_.release().ptr()); // steals a reference
        }
        return l.release();
    };
    else if(src.isObject())
        convert=[](Svar src){
        py::dict d;
        for (auto kv : src.as<SvarObject>()._var) {
            auto key = py::str(kv.first);
            auto value = py::reinterpret_steal<py::object>(getPy(kv.second));
            if (!key || !value)
                return py::handle();
            d[key] = value;
        }
        return d.release();
    };
    else if(src.isFunction())
        convert=[](Svar src){
        return svar_function(src).release();
    };
    else convert=[](Svar src){return py::detail::type_caster_base<Svar>::cast(src,py::return_value_policy::automatic,py::handle());};

    cls->def("getPy",convert);
    return getPy(src);
}

py::handle getModule(GSLAM::Svar src){
    if(src.isObject()){
        py::module module("svar");
        for (auto kv : src.as<SvarObject>()._var) {
            module.attr(kv.first.c_str())=py::reinterpret_steal<py::object>(getModule(kv.second));
        }
        return module.release();
    };
    return getPy(src);
}

py::object loadSvarPlugin(std::string pluginPath){
    SharedLibraryPtr plugin(new SharedLibrary(pluginPath));
    if(!plugin){
        std::cerr<<"Unable to load plugin "<<pluginPath<<std::endl;
        return py::object();
    }

    GSLAM::Svar* (*getInst)()=(GSLAM::Svar* (*)())plugin->getSymbol("svarInstance");
    if(!getInst){
        std::cerr<<"No svarInstance found in "<<pluginPath<<std::endl;
        return py::object();
    }
    GSLAM::Svar* var=getInst();
    if(!var){
        std::cerr<<"svarInstance returned null.\n";
        return py::object();
    }

    if(var->isObject()){
        return py::reinterpret_steal<py::object>(getModule(*var));
    }

    return py::reinterpret_steal<py::object>(getPy(*var));
}


#undef svar

PYBIND11_MODULE(svar,m)
{
    PyEval_InitThreads();
    m.def("load",loadSvarPlugin);

    py::class_<Svar>(m,"Svar")
            .def(py::init<>())
            .def("__str__",[](const Svar& v){std::stringstream sst;sst<<v;return sst.str();})
            .def("__len__",&Svar::length)
            .def("__getitem__",[](const Svar& self,const Svar& idx){return self[idx];})
            .def("typeName",[](const Svar& v){return v.typeName();})
            .def("py",getPy);

}



