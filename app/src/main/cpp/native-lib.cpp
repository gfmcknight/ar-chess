#include <jni.h>
#include <string>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include "NativeContext.h"

#define JNI_CALL(return_type, method_name) \
    extern "C" JNIEXPORT return_type JNICALL \
    Java_com_chess_1ar_handtracking_archess_JNIInterface_##method_name

inline jlong as_id (NativeContext* ctx) {
    return reinterpret_cast<intptr_t>(ctx);
}

inline NativeContext* from_id(jlong id) {
    return reinterpret_cast<NativeContext*>(id);
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_chess_1ar_handtracking_archess_MainARActivity_stringFromJNI(
        JNIEnv *env,
        jobject /* this */) {
    std::string hello = "Hello from C++";
    return env->NewStringUTF(hello.c_str());
}

JNI_CALL(jlong, createContext)(
        JNIEnv *env,
        jclass c,
        jobject assetManager) {
    NativeContext* context = new NativeContext(AAssetManager_fromJava(env, assetManager));
    return as_id(context);
}

JNI_CALL(void, deleteContext)(
        JNIEnv *env,
        jclass c,
        jlong id) {
    NativeContext* context = from_id(id);
    delete context;
}

