package com.switchui.core;

import android.view.Surface;

public final class NativeBridge {
    static {
        System.loadLibrary("switchui_native");
    }

    private NativeBridge() {}

    public static native boolean InitEmulator(Surface surface);
    public static native boolean LoadROM(String path);
    public static native void PauseEmulator();
    public static native void StopEmulator();
}
