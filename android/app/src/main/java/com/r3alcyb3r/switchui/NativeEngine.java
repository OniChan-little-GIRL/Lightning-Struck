package com.r3alcyb3r.switchui;

import android.view.Surface;

public class NativeEngine {
    static {
        System.loadLibrary("switchui_native");
    }

    public native void initEmulator(Surface surface);
    public native void loadROM(String path);
    public native void pauseEmulator();
    public native void stopEmulator();
}
