package io.github.vvb2060.magiskdetector;

import android.annotation.TargetApi;
import android.content.pm.ApplicationInfo;
import android.os.Build;

import androidx.annotation.NonNull;

@TargetApi(Build.VERSION_CODES.Q)
public class AppZygote implements android.app.ZygotePreload {

    @Override
    public void doPreload(@NonNull ApplicationInfo appInfo) {
        System.loadLibrary("vvb2060");
    }
}
