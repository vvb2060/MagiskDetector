package io.github.vvb2060.magiskdetector;

import android.util.ArraySet;

import java.util.Set;

public class Native {
    static final Set<String> properties = new ArraySet<>();

    static {
        System.loadLibrary("vvb2060");
    }

    static native int haveSu();

    static native int haveMagicMount();

    static native int findMagiskdSocket();

    static native int testIoctl();

    static native void getProps();

    static void add(String name) {
        properties.add(name);
    }

    static void clear() {
        properties.clear();
    }
}
