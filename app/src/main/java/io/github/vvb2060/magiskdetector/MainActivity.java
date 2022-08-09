package io.github.vvb2060.magiskdetector;

import android.app.Activity;
import android.content.ComponentName;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.Bundle;
import android.os.IBinder;
import android.os.RemoteException;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;

import java.io.IOException;

import io.github.vvb2060.magiskdetector.databinding.ActivityMainBinding;

public class MainActivity extends Activity {

    private static final String TAG = "MagiskDetector";
    private ActivityMainBinding binding;
    private final ServiceConnection connection = new ServiceConnection() {
        @Override
        public void onServiceConnected(ComponentName name, IBinder binder) {
            IRemoteService service = IRemoteService.Stub.asInterface(binder);
            try {
                setCard1(service.haveSu());
                setCard2(service.haveMagicMount());
                setCard3(service.haveMagiskHide());
            } catch (RemoteException e) {
                Log.e(TAG, "RemoteException", e);
            }
        }

        @Override
        public void onServiceDisconnected(ComponentName name) {
            binding.textView.setText(R.string.error);
        }

        @Override
        public void onNullBinding(ComponentName name) {
            setError();
        }
    };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        binding = ActivityMainBinding.inflate(getLayoutInflater());
        setContentView(binding.getRoot());
    }

    @Override
    protected void onStart() {
        super.onStart();
        Intent intent = new Intent(getApplicationContext(), RemoteService.class);
        if (!bindService(intent, connection, BIND_AUTO_CREATE)) setError();
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        getMenuInflater().inflate(R.menu.menu, menu);
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        if (item.getItemId() == R.id.logcat) {
            Intent intent = new Intent(Intent.ACTION_CREATE_DOCUMENT)
                    .addCategory(Intent.CATEGORY_OPENABLE)
                    .setType("text/plain")
                    .putExtra(Intent.EXTRA_TITLE, TAG);
            startActivityForResult(intent, 42);
            return true;
        } else return super.onOptionsItemSelected(item);
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        if (resultCode == Activity.RESULT_OK) {
            if (requestCode == 42 && data.getData() != null) {
                try (var outputStream = getContentResolver().openOutputStream(data.getData());
                     var inputStream = Runtime.getRuntime().exec("/system/bin/logcat -d -v long").getInputStream()) {
                    byte[] buffer = new byte[8 * 1024];
                    int bytes;
                    while ((bytes = inputStream.read(buffer)) >= 0)
                        outputStream.write(buffer, 0, bytes);
                } catch (IOException e) {
                    Log.e(TAG, "Unable to save log.", e);
                }
            }
        } else super.onActivityResult(requestCode, resultCode, data);
    }

    @Override
    protected void onStop() {
        super.onStop();
        unbindService(connection);
    }

    private void setCard1(int havesu) {
        String text;
        switch (havesu) {
            case 1:
                text = getString(R.string.test1_1);
                break;
            case 0:
                text = getString(R.string.test1_0);
                break;
            default:
                text = getString(R.string.test_e);
        }
        binding.textView.setText(getString(R.string.display, getString(R.string.test1), text));
    }

    private void setCard2(int haveMagicMount) {
        String text;
        if (haveMagicMount == 0) {
            text = getString(R.string.test2_0);
        } else if (haveMagicMount >= 1) {
            text = getString(R.string.test2_1, haveMagicMount);
        } else {
            text = getString(R.string.test_e);
        }
        binding.textView2.setText(getString(R.string.display, getString(R.string.test2), text));
    }

    private void setCard3(int magiskdHide) {
        String text;
        if (magiskdHide == 0) {
            text = getString(R.string.test3_0);
        } else if (magiskdHide >= 1) {
            text = getString(R.string.test3_1, magiskdHide);
        } else {
            text = getString(R.string.test_e);
        }
        binding.textView3.setText(getString(R.string.display, getString(R.string.test3), text));
    }

    private void setError() {
        binding.textView.setText(R.string.error);
        binding.cardView2.setVisibility(View.GONE);
        binding.cardView3.setVisibility(View.GONE);
    }
}
