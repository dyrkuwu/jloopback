package me.ley.jloopback;

import java.lang.ref.Cleaner;
import java.util.Objects;

public final class AudioProcessor implements AutoCloseable {
    private static final Cleaner CLEANER = Cleaner.create();

    private final NativeState nativeState;
    private final Cleaner.Cleanable cleanable;

    public AudioProcessor() {
        long handle = AudioProcessorNative.nativeCreate();
        if (handle == 0L) {
            throw new IllegalStateException("nativeCreate returned a null handle");
        }

        nativeState = new NativeState(handle);
        cleanable = CLEANER.register(this, nativeState);
    }

    public boolean update() {
        return AudioProcessorNative.nativeUpdate(requireOpenHandle());
    }

    public float[] getBands() {
        float[] bands = AudioProcessorNative.nativeGetBands(requireOpenHandle());
        return Objects.requireNonNullElseGet(bands, () -> new float[0]);
    }

    public float[] updateAndGetBands() {
        update();
        return getBands();
    }

    public boolean isOpen() {
        return nativeState.isOpen();
    }

    @Override
    public void close() {
        cleanable.clean();
    }

    private long requireOpenHandle() {
        long currentHandle = nativeState.getHandle();
        if (currentHandle == 0L) {
            throw new IllegalStateException("AudioProcessor is already closed");
        }

        return currentHandle;
    }

    private static final class NativeState implements Runnable {
        private long handle;

        private NativeState(long handle) {
            this.handle = handle;
        }

        private synchronized long getHandle() {
            return handle;
        }

        private synchronized boolean isOpen() {
            return handle != 0L;
        }

        @Override
        public synchronized void run() {
            if (handle == 0L) {
                return;
            }

            AudioProcessorNative.nativeDestroy(handle);
            handle = 0L;
        }
    }
}
