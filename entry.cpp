#include <cstdint>
#include <new>
#include <stdexcept>

#include <jni.h>

#include "audio_processor.h"

namespace
{
constexpr char kAudioProcessorNativeClassName[] = "me/ley/jloopback/AudioProcessorNative";

void throw_java_exception(JNIEnv* env, const char* class_name, const char* message)
{
    if (env == nullptr || env->ExceptionCheck()) {
        return;
    }

    jclass exception_class = env->FindClass(class_name);
    if (exception_class == nullptr) {
        return;
    }

    env->ThrowNew(exception_class, message);
    env->DeleteLocalRef(exception_class);
}

void throw_runtime_exception(JNIEnv* env, const char* message)
{
    throw_java_exception(env, "java/lang/RuntimeException", message);
}

void throw_illegal_state(JNIEnv* env, const char* message)
{
    throw_java_exception(env, "java/lang/IllegalStateException", message);
}

c_audio_processor* from_handle(jlong handle)
{
    return reinterpret_cast<c_audio_processor*>(static_cast<std::intptr_t>(handle));
}

jlong to_handle(c_audio_processor* processor)
{
    return static_cast<jlong>(reinterpret_cast<std::intptr_t>(processor));
}

jlong JNICALL native_create(JNIEnv* env, jclass)
{
    try {
        return to_handle(new c_audio_processor());
    } catch (const std::exception& ex) {
        throw_runtime_exception(env, ex.what());
    } catch (...) {
        throw_runtime_exception(env, "native audio processor initialization failed");
    }

    return 0;
}

void JNICALL native_destroy(JNIEnv*, jclass, jlong handle)
{
    delete from_handle(handle);
}

jboolean JNICALL native_update(JNIEnv* env, jclass, jlong handle)
{
    c_audio_processor* processor = from_handle(handle);
    if (processor == nullptr) {
        throw_illegal_state(env, "audio processor handle is null");
        return JNI_FALSE;
    }

    try {
        return processor->update() ? JNI_TRUE : JNI_FALSE;
    } catch (const std::exception& ex) {
        throw_runtime_exception(env, ex.what());
    } catch (...) {
        throw_runtime_exception(env, "native audio update failed");
    }

    return JNI_FALSE;
}

jfloatArray JNICALL native_get_bands(JNIEnv* env, jclass, jlong handle)
{
    c_audio_processor* processor = from_handle(handle);
    if (processor == nullptr) {
        throw_illegal_state(env, "audio processor handle is null");
        return nullptr;
    }

    try {
        const auto& bands = processor->get_bands();
        jfloatArray result = env->NewFloatArray(static_cast<jsize>(bands.size()));
        if (result == nullptr) {
            return nullptr;
        }

        env->SetFloatArrayRegion(result, 0, static_cast<jsize>(bands.size()), bands.data());
        return result;
    } catch (const std::exception& ex) {
        throw_runtime_exception(env, ex.what());
    } catch (...) {
        throw_runtime_exception(env, "native band retrieval failed");
    }

    return nullptr;
}

const JNINativeMethod kNativeMethods[] = {
    { const_cast<char*>("nativeCreate"), const_cast<char*>("()J"), reinterpret_cast<void*>(native_create) },
    { const_cast<char*>("nativeDestroy"), const_cast<char*>("(J)V"), reinterpret_cast<void*>(native_destroy) },
    { const_cast<char*>("nativeUpdate"), const_cast<char*>("(J)Z"), reinterpret_cast<void*>(native_update) },
    { const_cast<char*>("nativeGetBands"), const_cast<char*>("(J)[F"), reinterpret_cast<void*>(native_get_bands) },
};
} // namespace

extern "C" JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void*)
{
    JNIEnv* env = nullptr;
    if (vm == nullptr || vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_8) != JNI_OK || env == nullptr) {
        return JNI_ERR;
    }

    jclass bridge_class = env->FindClass(kAudioProcessorNativeClassName);
    if (bridge_class == nullptr) {
        return JNI_ERR;
    }

    if (env->RegisterNatives(
            bridge_class,
            kNativeMethods,
            static_cast<jint>(sizeof(kNativeMethods) / sizeof(kNativeMethods[0]))) != 0) {
        env->DeleteLocalRef(bridge_class);
        return JNI_ERR;
    }

    env->DeleteLocalRef(bridge_class);
    return JNI_VERSION_1_8;
}
