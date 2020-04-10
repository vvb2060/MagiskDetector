package io.github.vvb2060.magiskdetector;

import android.app.Service;
import android.content.Intent;
import android.os.IBinder;

public class RemoteService extends Service {
    private final IRemoteService.Stub mBinder = new IRemoteService.Stub() {
        @Override
        public boolean haveSu() {
            return App.haveSu() == 0;
        }
    };

    @Override
    public IBinder onBind(Intent intent) {
        return mBinder;
    }

}
