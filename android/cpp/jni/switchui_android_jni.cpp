#include <jni.h>
#include <string>
#include <android/log.h>
#include <android/native_window_jni.h>
#include "../gpu/vulkan_context.hpp"
#include "../memory/jit_memory.hpp"

#define LOG_TAG "SwitchUI_JNI"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

static SwitchUI::VulkanContext* g_VulkanContext = nullptr;

extern "C" {

JNIEXPORT void JNICALL
Java_com_r3alcyb3r_switchui_NativeEngine_InitEmulator(JNIEnv* env, jobject thiz, jobject surface) {
    LOGI("Initializing SwitchUI Native Engine...");

    ANativeWindow* window = ANativeWindow_fromSurface(env, surface);
    if (!window) {
        LOGE("Could not get ANativeWindow from surface");
        return;
    }

    g_VulkanContext = new SwitchUI::VulkanContext();
    if (!g_VulkanContext->Initialize(window)) {
        LOGE("Failed to initialize Vulkan Context");
    }

    LOGI("Emulator Engine Initialized.");
}

JNIEXPORT void JNICALL
Java_com_r3alcyb3r_switchui_NativeEngine_LoadROM(JNIEnv* env, jobject thiz, jstring path) {
    const char* nativePath = env->GetStringUTFChars(path, nullptr);
    LOGI("Loading ROM from: %s", nativePath);

    // Core ROM loading logic would go here

    env->ReleaseStringUTFChars(path, nativePath);
}

JNIEXPORT void JNICALL
Java_com_r3alcyb3r_switchui_NativeEngine_PauseEmulator(JNIEnv* env, jobject thiz) {
    LOGI("Emulator Paused");
}

JNIEXPORT void JNICALL
Java_com_r3alcyb3r_switchui_NativeEngine_StopEmulator(JNIEnv* env, jobject thiz) {
    LOGI("Stopping Emulator...");
    if (g_VulkanContext) {
        g_VulkanContext->Shutdown();
        delete g_VulkanContext;
        g_VulkanContext = nullptr;
    }
    LOGI("Emulator Stopped.");
}

}
