package io.github.vvb2060.magiskdetector;

import android.app.Application;

public class App extends Application {
    static {
        System.loadLibrary("vvb2060");
    }

    static native int haveSu();

    static native int haveMagicMount();

    static native int findMagiskdSocket();
}
