package com.chess_ar.handtracking.archess;

import android.Manifest;
import android.app.Activity;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.widget.TextView;

import java.security.Permission;

public class MainARActivity extends Activity {

    private static int MY_CAMERA_PERMISSIONS_CODE = 811;

    // Used to load the 'native-lib' library on application startup.
    static {
        System.loadLibrary("native-lib");
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main_ar);

        // Example of a call to a native method
        TextView tv = (TextView) findViewById(R.id.sample_text);
        tv.setText(stringFromJNI());

        if (checkSelfPermission(Manifest.permission.CAMERA)
                != PackageManager.PERMISSION_GRANTED) {
            requestPermissions(
                    new String[]{Manifest.permission.CAMERA},
                    MY_CAMERA_PERMISSIONS_CODE);
        } else {
            gotoOpenGLActivity();
        }
    }

    @Override
    public void onRequestPermissionsResult(int code, String[] permissions, int[] results) {
        // Result 0 has the camera permission
        if (results.length > 0 && results[0] == PackageManager.PERMISSION_GRANTED) {
            gotoOpenGLActivity();
        }
    }

    private void gotoOpenGLActivity() {

    }

    /**
     * A native method that is implemented by the 'native-lib' native library,
     * which is packaged with this application.
     */
    public native String stringFromJNI();
}
