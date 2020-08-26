package io.github.vvb2060.magiskdetector;

import android.app.Activity;
import android.content.ComponentName;
import android.content.Intent;
import android.content.ServiceConnection;
import android.content.SharedPreferences;
import android.os.Build;
import android.os.Bundle;
import android.os.IBinder;
import android.os.RemoteException;
import android.os.SystemClock;
import android.util.ArraySet;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;

import androidx.security.crypto.EncryptedSharedPreferences;
import androidx.security.crypto.MasterKey;

import java.io.BufferedReader;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.io.Reader;
import java.nio.charset.StandardCharsets;
import java.security.GeneralSecurityException;
import java.util.Set;

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
        setCard2(Native.haveMagicMount());
        setCard3(Native.findMagiskdSocket());
        setCard4(Native.haveSu() == 0);
        setCard5(Native.testIoctl());
        setCard6(props());
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
                try {
                    OutputStream outputStream = getContentResolver().openOutputStream(data.getData());
                    InputStream inputStream = Runtime.getRuntime().exec("/system/bin/logcat -d -v long").getInputStream();
                    assert outputStream != null;
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

    private int props() {
        Native.getProps();
        SharedPreferences sp;
        try {
            MasterKey masterKey = new MasterKey.Builder(this)
                    .setKeyScheme(MasterKey.KeyScheme.AES256_GCM)
                    .build();
            sp = EncryptedSharedPreferences.create(
                    this,
                    getPackageName(),
                    masterKey,
                    EncryptedSharedPreferences.PrefKeyEncryptionScheme.AES256_SIV,
                    EncryptedSharedPreferences.PrefValueEncryptionScheme.AES256_GCM
            );
        } catch (GeneralSecurityException | IOException e) {
            Log.e(TAG, "Unable to open SharedPreferences.", e);
            return -1;
        }

        String spFingerprint = sp.getString("fingerprint", "");
        String fingerprint = Build.FINGERPRINT;
        Log.i(TAG, "spFingerprint=" + spFingerprint + " \n  fingerprint=" + fingerprint);
        String spBootId = sp.getString("boot_id", "");
        String bootId = getBootId();
        Log.i(TAG, "spBootId=" + spBootId + " \n  bootId=" + bootId);
        if (spFingerprint.equals(fingerprint) && spBootId.length() > 0) {
            if (!spBootId.equals(bootId)) {
                Set<String> props = sp.getStringSet("props", new ArraySet<>());
                return props.equals(Native.properties) ? 0 : 1;
            } else return 2;
        } else {
            SharedPreferences.Editor editor = sp.edit();
            editor.putString("fingerprint", fingerprint);
            editor.putString("boot_id", bootId);
            editor.putStringSet("props", Native.properties);
            editor.apply();
            return 2;
        }
    }

    private String getBootId() {
        String bootId = "";
        try (InputStream is = new FileInputStream("/proc/sys/kernel/random/boot_id")) {
            Reader reader = new InputStreamReader(is, StandardCharsets.UTF_8);
            bootId = new BufferedReader(reader).readLine().trim();
        } catch (IOException e) {
            Log.w(TAG, "Can't read boot_id.", e);
        }
        if (bootId.length() == 0) {
            bootId = String.valueOf((System.currentTimeMillis() - SystemClock.elapsedRealtime()) / 10);
        }
        return bootId;
    }

    private void setCard1(boolean havesu) {
        String text;
        if (havesu) {
            text = getString(R.string.test1_t);
            binding.cardView4.setVisibility(View.VISIBLE);
        } else {
            text = getString(R.string.test1_f);
            binding.cardView4.setVisibility(View.GONE);
        }
        binding.textView.setText(getString(R.string.display, getString(R.string.test1), text));
    }

    private void setCard2(int haveMagicMount) {
        String text;
        switch (haveMagicMount) {
            case 0:
                text = getString(R.string.test2_0);
                break;
            case 1:
                text = getString(R.string.test2_1);
                break;
            default:
                text = getString(R.string.test2_d);
        }
        binding.textView2.setText(getString(R.string.display, getString(R.string.test2), text));
    }

    private void setCard3(int magiskdSocket) {
        String text;
        switch (magiskdSocket) {
            case 0:
                text = getString(R.string.test3_0);
                break;
            case -1:
                text = getString(R.string.test3_1);
                break;
            case -2:
                text = getString(R.string.test3_2);
                break;
            case -3:
                text = getString(R.string.test3_3);
                break;
            default:
                text = getString(R.string.test3_d, magiskdSocket);
        }
        binding.textView3.setText(getString(R.string.display, getString(R.string.test3), text));
    }

    private void setCard4(boolean havesu) {
        String text = havesu ? getString(R.string.test4_t) : getString(R.string.test4_f);
        binding.textView4.setText(getString(R.string.display, getString(R.string.test4), text));
    }

    private void setCard5(int testIoctl) {
        String text;
        switch (testIoctl) {
            case -1:
            default:
                text = getString(R.string.test5_01);
                break;
            case 0:
                text = getString(R.string.test5_0);
                break;
            case 1:
                text = getString(R.string.test5_1);
                break;
            case 2:
                text = getString(R.string.test5_2);
                break;
        }
        binding.textView5.setText(getString(R.string.display, getString(R.string.test5), text));
    }

    private void setCard6(int props) {
        String text;
        switch (props) {
            case -1:
            default:
                text = getString(R.string.test6_01);
                break;
            case 0:
                text = getString(R.string.test6_0);
                break;
            case 1:
                text = getString(R.string.test6_1);
                break;
            case 2:
                text = getString(R.string.test6_2);
                break;
        }
        binding.textView6.setText(getString(R.string.display, getString(R.string.test6), text));
    }

    private void setError() {
        binding.textView.setText(R.string.error);
        binding.cardView2.setVisibility(View.GONE);
        binding.cardView3.setVisibility(View.GONE);
        binding.cardView4.setVisibility(View.GONE);
    }
}
