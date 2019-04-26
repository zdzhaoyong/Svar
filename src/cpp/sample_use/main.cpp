#include "Svar.h"

using namespace GSLAM;

int main(int argc,char** argv){

    Svar ApplicationDemo=svar["ApplicationDemo"];

    std::cout<<ApplicationDemo.as<SvarClass>();
    Svar inst=ApplicationDemo("name");
    return 0;

}
