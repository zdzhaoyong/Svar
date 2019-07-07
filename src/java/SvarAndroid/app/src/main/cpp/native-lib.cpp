#include <jni.h>
#include <string>
#define __SVAR_BUILDIN__
#include "Svar.h"
#include <android/log.h>

#define TAG    "SvarDebug" // 这个是自定义的LOG的标识
#define LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG,TAG,__VA_ARGS__) // 定义LOGD类型

using namespace GSLAM;

Svar& getThisSvar(JNIEnv *env, jobject instance){
    jclass    jcInfo  = env->GetObjectClass(instance);
    jfieldID  ptrField= env->GetFieldID(jcInfo,"native_ptr","J");
    jlong     ptr=env->GetLongField(instance,ptrField);
    return *((Svar*)ptr);
}

jobject Svar2JObject(JNIEnv *env,Svar var){
    jclass jcInfo = env->FindClass("com/rtmapper/myapplication/Svar");
    jmethodID method= env->GetStaticMethodID(jcInfo,"createFromNative","(J)Lcom/rtmapper/myapplication/Svar;");
    return env->CallStaticObjectMethod(jcInfo,method,(jlong)new Svar(var));
}



extern "C"
{


JNIEXPORT jobject JNICALL
Java_com_rtmapper_myapplication_Svar_callNative__Lcom_rtmapper_myapplication_Svar_2(JNIEnv *env,
                                                                              jobject instance,
                                                                              jobject args) {
    Svar var=getThisSvar(env,instance);
    Svar argv=getThisSvar(env,args);
    auto argVec=argv.castAs<std::vector<Svar>>();
    if(var.isFunction())
        return Svar2JObject(env,var.as<SvarFunction>().Call(argVec));
    else if(var.isClass()){
        const SvarClass& cls=var.as<SvarClass>();
        if(!cls.__init__.isFunction())
            throw SvarExeption("Class "+cls.__name__+" does not have __init__ function.");
        return Svar2JObject(env,cls.__init__.as<SvarFunction>().Call(argVec));
    }
    throw SvarExeption(var.typeName()+" can't be called as a function or constructor.");
    return nullptr;
}

JNIEXPORT jobject JNICALL
Java_com_rtmapper_myapplication_Svar_callNative__Ljava_lang_String_2Lcom_rtmapper_myapplication_Svar_2(
        JNIEnv *env, jobject instance, jstring name_, jobject args) {
    const char *name = env->GetStringUTFChars(name_, 0);
    Svar var=getThisSvar(env,instance);
    Svar argv=getThisSvar(env,args);
    auto argVec=argv.castAs<std::vector<Svar>>();


    Svar ret;
    try{
    // call as static methods without check
    if(var.isClass())
        ret=var.as<SvarClass>().Call(Svar(),name,argVec);
    else{
        auto clsPtr=var.classPtr();
        if(!clsPtr) throw SvarExeption("Unable to call "+var.typeName()+"."+name);
        ret= clsPtr->Call(var,name,argVec);
    }
    }
    catch (std::exception& e){
        __android_log_print(ANDROID_LOG_FATAL,"SvarFatal","########## i = %s",e.what());
        return Svar2JObject(env,Svar(e.what()));
    }
    env->ReleaseStringUTFChars(name_, name);
    return Svar2JObject(env,ret);
}

JNIEXPORT void JNICALL
Java_com_rtmapper_myapplication_Svar_clear(JNIEnv *env, jobject instance) {
    getThisSvar(env,instance);
}

JNIEXPORT void JNICALL
Java_com_rtmapper_myapplication_Svar_erase(JNIEnv *env, jobject instance, jstring id_) {
    const char *id = env->GetStringUTFChars(id_, 0);

    getThisSvar(env,instance).erase(id);

    env->ReleaseStringUTFChars(id_, id);
}

JNIEXPORT jobject JNICALL
Java_com_rtmapper_myapplication_Svar_instance(JNIEnv *env, jclass type) {
    return Svar2JObject(env,Svar::instance());
}

JNIEXPORT jboolean JNICALL
Java_com_rtmapper_myapplication_Svar_isJavaObject(JNIEnv *env, jobject instance) {
    return getThisSvar(env,instance).is<jobject>();
}

JNIEXPORT void JNICALL
Java_com_rtmapper_myapplication_Svar_set(JNIEnv *env, jobject instance, jstring id_, jobject obj) {
    const char *id = env->GetStringUTFChars(id_, 0);

    getThisSvar(env,instance).set(id,getThisSvar(env,obj));
    env->ReleaseStringUTFChars(id_, id);
}

JNIEXPORT jobject JNICALL
Java_com_rtmapper_myapplication_Svar_mapGet(JNIEnv *env, jobject instance, jstring value_) {
    const char *value = env->GetStringUTFChars(value_, 0);

    auto var=getThisSvar(env,instance)[value];
    env->ReleaseStringUTFChars(value_, value);
    return Svar2JObject(env,var);
}


JNIEXPORT jint JNICALL
Java_com_rtmapper_myapplication_Svar_length(JNIEnv *env, jobject instance) {
    return getThisSvar(env,instance).length();
}

JNIEXPORT jobject JNICALL
Java_com_rtmapper_myapplication_Svar_get(JNIEnv *env, jobject instance, jint value) {
    return Svar2JObject(env,getThisSvar(env,instance)[value]);
}

JNIEXPORT jobject JNICALL
Java_com_rtmapper_myapplication_Svar_getMap(JNIEnv *env, jobject instance, jstring id_) {
    const char *id = env->GetStringUTFChars(id_, 0);
    auto ret= Svar2JObject(env,getThisSvar(env,instance)[id]);

    env->ReleaseStringUTFChars(id_, id);
    return ret;
}

JNIEXPORT jlong JNICALL
Java_com_rtmapper_myapplication_Svar_createList(JNIEnv *env, jobject instance, jint length,
                                                jobjectArray list) {

    if(length<=0) return (jlong)new Svar(Svar::array());

    std::vector<Svar> vec;
    vec.reserve(length);
    for(int i=0;i<length;++i){
        jobject obj=env->GetObjectArrayElement(list,i);
        vec.push_back(getThisSvar(env,obj));
    }
    return (jlong)new Svar(vec);
}

JNIEXPORT void JNICALL
Java_com_rtmapper_myapplication_Svar_add(JNIEnv *env, jobject instance, jobject obj) {
    getThisSvar(env,instance).push_back(getThisSvar(env,obj));
}


JNIEXPORT jboolean JNICALL
Java_com_rtmapper_myapplication_Svar_isClass(JNIEnv *env, jobject instance) {
    return getThisSvar(env,instance).isClass();
}

JNIEXPORT jstring JNICALL
Java_com_rtmapper_myapplication_Svar_type(JNIEnv *env, jobject instance) {
    return env->NewStringUTF(getThisSvar(env,instance).typeName().c_str());
}

JNIEXPORT jstring JNICALL
Java_com_rtmapper_myapplication_Svar_toString(JNIEnv *env, jobject instance) {
    std::stringstream sst;
    sst<<getThisSvar(env,instance);
    return env->NewStringUTF(sst.str().c_str());
}

JNIEXPORT jboolean JNICALL
Java_com_rtmapper_myapplication_Svar_isFunction(JNIEnv *env, jobject instance) {
    return getThisSvar(env,instance).isFunction();
}

JNIEXPORT jboolean JNICALL
Java_com_rtmapper_myapplication_Svar_isUndefined(JNIEnv *env, jobject instance) {
    return getThisSvar(env,instance).isUndefined();
}

JNIEXPORT jboolean JNICALL
Java_com_rtmapper_myapplication_Svar_isNull(JNIEnv *env, jobject instance) {
    return getThisSvar(env,instance).isNull();
}

JNIEXPORT jboolean JNICALL
Java_com_rtmapper_myapplication_Svar_isBoolean(JNIEnv *env, jobject instance) {
    return getThisSvar(env,instance).is<bool>();
}

JNIEXPORT jboolean JNICALL
Java_com_rtmapper_myapplication_Svar_isChar(JNIEnv *env, jobject instance) {
    return getThisSvar(env,instance).is<char>();
}

JNIEXPORT jboolean JNICALL
Java_com_rtmapper_myapplication_Svar_isByte(JNIEnv *env, jobject instance) {
    return getThisSvar(env,instance).is<u_char>();
}

JNIEXPORT jboolean JNICALL
Java_com_rtmapper_myapplication_Svar_isShort(JNIEnv *env, jobject instance) {
    return getThisSvar(env,instance).is<int16_t >();
}

extern "C"
JNIEXPORT jboolean JNICALL
Java_com_rtmapper_myapplication_Svar_isInt(JNIEnv *env, jobject instance) {
    return getThisSvar(env,instance).is<int>();
}

extern "C"
JNIEXPORT jboolean JNICALL
Java_com_rtmapper_myapplication_Svar_isDouble(JNIEnv *env, jobject instance) {
    return getThisSvar(env,instance).is<double>();
}

extern "C"
JNIEXPORT jlong JNICALL
Java_com_rtmapper_myapplication_Svar_Undefined(JNIEnv *env, jobject instance) {
    return (jlong)new Svar();
}

extern "C"
JNIEXPORT jlong JNICALL
Java_com_rtmapper_myapplication_Svar_Null(JNIEnv *env, jobject instance) {
    return (jlong)new Svar(nullptr);
}

extern "C"
JNIEXPORT jlong JNICALL
Java_com_rtmapper_myapplication_Svar_create__Z(JNIEnv *env, jobject instance, jboolean value) {
    return (jlong)new Svar((bool)value);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_rtmapper_myapplication_Svar_release(JNIEnv *env, jclass instance, jlong ptr) {
    std::cerr<<((Svar*)ptr)<<" released.";
    delete (Svar*)ptr;
}

extern "C"
JNIEXPORT jstring JNICALL
Java_com_rtmapper_myapplication_MainActivity_stringFromJNI(
        JNIEnv *env,
        jobject /* this */) {
    std::string hello = "Hello from C++";
    return env->NewStringUTF(hello.c_str());
}


JNIEXPORT jboolean JNICALL
Java_com_rtmapper_myapplication_Svar_isList(JNIEnv *env, jobject instance) {
    return getThisSvar(env,instance).isArray();
}

JNIEXPORT jboolean JNICALL
Java_com_rtmapper_myapplication_Svar_isMap(JNIEnv *env, jobject instance) {
    return getThisSvar(env,instance).isObject();
}

JNIEXPORT jlong JNICALL
Java_com_rtmapper_myapplication_Svar_create__I(JNIEnv *env, jobject instance, jint value) {
    return (jlong)new Svar((int)value);
}

JNIEXPORT jlong JNICALL
Java_com_rtmapper_myapplication_Svar_create__D(JNIEnv *env, jobject instance, jdouble value) {
    return (jlong)new Svar((int)value);
}

JNIEXPORT jlong JNICALL
Java_com_rtmapper_myapplication_Svar_create__Ljava_lang_String_2(JNIEnv *env, jobject instance,
                                                                 jstring value_) {
    const char *value = env->GetStringUTFChars(value_, 0);
    env->ReleaseStringUTFChars(value_, value);
    return (jlong)new Svar(std::string(value));
}

}

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

REGISTER_SVAR_MODULE(bench){
        Class<BenchClass>()
                .construct<int>()
                .def_static("create",&BenchClass::create)
                .def("get",&BenchClass::get);

        svar["BenchClass"]=SvarClass::instance<BenchClass>();
        svar["sampleCFunc"]=sampleCFunc;
        svar["sampleVariable"]=1;
        svar["versionInfo"]="1.0";
}