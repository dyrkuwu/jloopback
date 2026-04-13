package me.ley.jloopback;

import java.io.IOException;
import java.io.InputStream;
import java.io.UncheckedIOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.StandardCopyOption;

final class NativeLibraryLoader {
    private static final String LIBRARY_NAME = "jloopback";
    private static final String RESOURCE_PATH = "/natives/windows-x86_64/jloopback.dll";
    private static volatile boolean loaded;

    private NativeLibraryLoader() {
    }

    static void load() {
        if (loaded) {
            return;
        }

        synchronized (NativeLibraryLoader.class) {
            if (loaded) {
                return;
            }

            ensureSupportedPlatform();

            Path extractedPath;
            try {
                extractedPath = extractLibrary();
            } catch (IOException ex) {
                throw new UncheckedIOException("failed to extract native library from JAR", ex);
            }

            System.load(extractedPath.toAbsolutePath().toString());
            loaded = true;
        }
    }

    private static void ensureSupportedPlatform() {
        String osName = System.getProperty("os.name", "").toLowerCase();
        String arch = normalizeArch(System.getProperty("os.arch", ""));

        if (!osName.contains("win")) {
            throw new UnsupportedOperationException("embedded native library is only packaged for Windows");
        }

        if (!"x86_64".equals(arch)) {
            throw new UnsupportedOperationException(
                "embedded native library is only packaged for x86_64, detected: " + arch
            );
        }
    }

    private static String normalizeArch(String arch) {
        String normalized = arch.toLowerCase();
        if ("amd64".equals(normalized)) {
            return "x86_64";
        }
        return normalized;
    }

    private static Path extractLibrary() throws IOException {
        try (InputStream input = NativeLibraryLoader.class.getResourceAsStream(RESOURCE_PATH)) {
            if (input == null) {
                throw new IOException("missing native library resource: " + RESOURCE_PATH);
            }

            Path extractedFile = Files.createTempFile(LIBRARY_NAME + "-", ".dll");
            Files.copy(input, extractedFile, StandardCopyOption.REPLACE_EXISTING);
            extractedFile.toFile().deleteOnExit();
            return extractedFile;
        }
    }
}
