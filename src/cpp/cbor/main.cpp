#include  <Svar.h>
#include  <Registry.h>
#include  <Glog.h>

using namespace sv;

int cbor(Svar config){
    std::string cborf=config.arg("in",std::string(""),"the cbor file want to load and show");
    std::string json=config.arg("json",std::string(""),"the json file want to transform");
    int bufsize=config.arg("bufsize",0,"the buffer size want to add");
    std::string output=config.arg("output",std::string(""),"save the cbor buffer to");

    if(config.get("help",false)) return config.help();

    Svar CBOR=Registry::load("svar_cbor");
    if(CBOR.isUndefined()){
        return -1;
    }

    if(cborf.size()){
        std::cerr<<CBOR["parse_cbor"](SvarBuffer::load(cborf));
        return 0;
    }

    Svar var={{"i",1},
              {"bool",false},
              {"double",434.},
              {"str","sfd"},
              {"vec",{1,2,3}},
              {"map",{{"name","value"}}}
             };
    if(json.size()) var=Svar::loadFile(json);
    if(bufsize){
        var["buffer"]=SvarBuffer(bufsize);
    }
    SvarBuffer bin=CBOR["dump_cbor"](var).as<SvarBuffer>();
    LOG(INFO)<<"cbor size is "<<bin.size();
    if(output.size())
        bin.save(output);
    return 0;
}

REGISTER_SVAR_MODULE(cbor){
    svar["apps"]["cbor"]=cbor;
}
