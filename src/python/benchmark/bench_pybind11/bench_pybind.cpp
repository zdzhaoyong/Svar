#include <pybind11/pybind11.h>

using namespace pybind11;

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

PYBIND11_MODULE(bench,m){

    class_<BenchClass>(m,"BenchClass")
            .def(pybind11::init<int>())
            .def_static("create",&BenchClass::create)
            .def("get",&BenchClass::get);

    m.def("sampleCFunc",sampleCFunc);
    m.add_object("sampleVariable",int_(1));
}

