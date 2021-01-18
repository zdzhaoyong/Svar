#include <Svar/SvarPy.h>

using namespace sv;

class BenchClass
{
public:
    struct BenchClassData{int a;};
    BenchClass(int i=1)
        : data(i){}

    static BenchClass create(int i){
        return BenchClass(i);
    }

    BenchClassData get(const int& idx){return data[idx];}

    std::vector<BenchClassData> data;
};

BenchClass sampleCFunc(const int& i){return BenchClass(i);}

SVAR_PYTHON_IMPL(bench){
    Class<BenchClass>()
            .construct<int>()
            .def_static("create",&BenchClass::create)
            .def("get",&BenchClass::get);

    svar["BenchClass"]=SvarClass::instance<BenchClass>();
    svar["sampleCFunc"]=sampleCFunc;
    svar["sampleVariable"]=1;
    svar["versionInfo"]="1.0";

    return SvarPy::getModule(svar);
}
