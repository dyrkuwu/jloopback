package me.ley.jloopback;

final class AudioProcessorNative {
    static {
        NativeLibraryLoader.load();
    }

    private AudioProcessorNative() {
    }

    static native long nativeCreate();

    static native void nativeDestroy(long handle);

    static native boolean nativeUpdate(long handle);

    static native float[] nativeGetBands(long handle);
}
