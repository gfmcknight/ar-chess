#include <jni.h>
#include <string>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include "NativeContext.h"

#define JNI_CALL(return_type, method_name) \
    extern "C" JNIEXPORT return_type JNICALL \
    Java_com_chess_1ar_handtracking_archess_JNIInterface_##method_name

// maintain a reference to the JVM so we can use it later.
static JavaVM *g_vm = nullptr;

inline jlong as_id (NativeContext* ctx) {
    return reinterpret_cast<intptr_t>(ctx);
}

inline NativeContext* from_id(jlong id) {
    return reinterpret_cast<NativeContext*>(id);
}

jint JNI_OnLoad(JavaVM *vm, void *) {
    g_vm = vm;
    return JNI_VERSION_1_6;
}

extern "C" JNIEnv *GetJniEnv() {
    JNIEnv *env;
    jint result = g_vm->AttachCurrentThread(&env, nullptr);
    return result == JNI_OK ? env : nullptr;
}

extern "C" jclass FindClass(const char *classname) {
    JNIEnv *env = GetJniEnv();
    return env->FindClass(classname);
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
        jobject assetManager,
        jobject jContext) {
    NativeContext* context = new NativeContext(AAssetManager_fromJava(env, assetManager), jContext, env);
    return as_id(context);
}

JNI_CALL(void, deleteContext)(
        JNIEnv *env,
        jclass c,
        jlong id) {
    NativeContext* context = from_id(id);
    delete context;
}

JNI_CALL(void, onPause)
        (JNIEnv *, jclass, jlong native_application) {
    from_id(native_application)->OnPause();
}

JNI_CALL(void, onResume)
        (JNIEnv *env, jclass, jlong native_application, jobject context,
         jobject activity) {
    from_id(native_application)->OnResume(env, context, activity);
}

JNI_CALL(void, onGlSurfaceCreated)
        (JNIEnv *, jclass, jlong native_application) {
    from_id(native_application)->OnSurfaceCreated();
}

JNI_CALL(void, onDisplayGeometryChanged)
        (JNIEnv *, jobject, jlong native_application, jint display_rotation, jint width,
         jint height) {
    from_id(native_application)
            ->OnDisplayGeometryChanged(display_rotation, width, height);
}

JNI_CALL(void, onGlSurfaceDrawFrame)
        (JNIEnv *, jclass, jlong native_application) {
    from_id(native_application)->OnDrawFrame();
}

JNI_CALL(jboolean, hasDetectedPlanes)
(JNIEnv *, jclass, jlong native_application) {
return static_cast<jboolean>(
    from_id(native_application)->HasDetectedPlanes() ? JNI_TRUE : JNI_FALSE);
}

JNI_CALL(void, onTouched)
        (JNIEnv *, jclass, jlong native_application, jfloat x, jfloat y) {
    from_id(native_application)->OnTouched(x, y);
}

