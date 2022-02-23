#include "Svar.h"
#include "gtest.h"
#include "Timer.h"

using namespace sv;


int tests(Svar config){
    testing::InitGoogleTest(&config["argc"].as<int>(),config["argv"].as<char**>());
    return RUN_ALL_TESTS();
}

REGISTER_SVAR_MODULE(tests){
    svar["apps"]["tests"]={tests,"Use 'svar tests' to perform module testing"};
}
