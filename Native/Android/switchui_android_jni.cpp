#include "jit_memory_android.h"
#include "../Renderer/vulkan_backend_android.h"

#include <android/log.h>
#include <android/native_window_jni.h>
#include <jni.h>

#include <memory>
#include <mutex>
#include <string>

namespace {

constexpr const char* kTag = "SwitchUI-JNI";

class EmulatorCore {
public:
    bool Init(ANativeWindow* window) {
        if (!switchui::renderer::InitVulkanForAndroid(window, &vk_)) {
            __android_log_print(ANDROID_LOG_ERROR, kTag, "InitVulkanForAndroid failed");
            return false;
        }

        jit_ = switchui::android::AllocateJitMemory(64 * 1024 * 1024);
        if (!jit_.rw || !jit_.rx) {
            __android_log_print(ANDROID_LOG_ERROR, kTag, "AllocateJitMemory failed");
            switchui::renderer::DestroyVulkanContext(&vk_);
            return false;
        }
        return true;
    }

    bool LoadRom(const std::string& path) {
        if (path.empty()) {
            return false;
        }
        rom_path_ = path;
        __android_log_print(ANDROID_LOG_INFO, kTag, "ROM loaded: %s", rom_path_.c_str());
        return true;
    }

    void Pause() {
        paused_ = true;
    }

    void Stop() {
        paused_ = true;
        switchui::android::FreeJitMemory(&jit_);
        switchui::renderer::DestroyVulkanContext(&vk_);
        rom_path_.clear();
    }

private:
    bool paused_ = false;
    std::string rom_path_;
    switchui::renderer::VulkanContext vk_{};
    switchui::android::JitMemoryBlock jit_{};
};

std::mutex g_mutex;
std::unique_ptr<EmulatorCore> g_emulator;

} // namespace

extern "C" JNIEXPORT jboolean JNICALL
Java_com_switchui_core_NativeBridge_InitEmulator(JNIEnv* env, jobject /*thiz*/, jobject surface) {
    std::lock_guard<std::mutex> lock(g_mutex);

    ANativeWindow* window = ANativeWindow_fromSurface(env, surface);
    if (!window) {
        return JNI_FALSE;
    }

    g_emulator = std::make_unique<EmulatorCore>();
    const bool ok = g_emulator->Init(window);
    ANativeWindow_release(window);

    return ok ? JNI_TRUE : JNI_FALSE;
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_switchui_core_NativeBridge_LoadROM(JNIEnv* env, jobject /*thiz*/, jstring path) {
    std::lock_guard<std::mutex> lock(g_mutex);
    if (!g_emulator || !path) {
        return JNI_FALSE;
    }

    const char* utf = env->GetStringUTFChars(path, nullptr);
    const std::string rom_path = utf ? utf : "";
    if (utf) {
        env->ReleaseStringUTFChars(path, utf);
    }
    return g_emulator->LoadRom(rom_path) ? JNI_TRUE : JNI_FALSE;
}

extern "C" JNIEXPORT void JNICALL
Java_com_switchui_core_NativeBridge_PauseEmulator(JNIEnv* /*env*/, jobject /*thiz*/) {
    std::lock_guard<std::mutex> lock(g_mutex);
    if (g_emulator) {
        g_emulator->Pause();
    }
}

extern "C" JNIEXPORT void JNICALL
Java_com_switchui_core_NativeBridge_StopEmulator(JNIEnv* /*env*/, jobject /*thiz*/) {
    std::lock_guard<std::mutex> lock(g_mutex);
    if (g_emulator) {
        g_emulator->Stop();
        g_emulator.reset();
    }
}
