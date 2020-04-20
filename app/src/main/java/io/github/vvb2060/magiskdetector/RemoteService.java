package io.github.vvb2060.magiskdetector;

import android.app.Service;
import android.content.Intent;
import android.os.IBinder;
import android.system.Os;

public class RemoteService extends Service {
    private final IRemoteService.Stub mBinder = new IRemoteService.Stub() {
        @Override
        public boolean haveSu() {
            return Native.haveSu() == 0;
        }
    };

    @Override
    public IBinder onBind(Intent intent) {
        int appId = Os.getuid() % 100000;
        if (appId >= 90000) return mBinder;
        else return null;
    }

}
