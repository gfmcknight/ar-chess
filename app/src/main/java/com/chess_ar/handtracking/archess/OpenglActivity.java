package com.chess_ar.handtracking.archess;

import android.app.Activity;
import android.opengl.GLES20;
import android.opengl.GLSurfaceView;
import android.os.Bundle;
import android.os.Handler;
import android.view.GestureDetector;
import android.view.MotionEvent;
import android.view.View;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

public class OpenglActivity extends Activity implements GLSurfaceView.Renderer {
    private static final String TAG = OpenglActivity.class.getSimpleName();

    private GLSurfaceView surfaceView;

    private boolean viewportChanged = false;
    private int viewportWidth;
    private int viewportHeight;

    // Opaque native pointer to the native application instance.
    private long nativeApplication;
    private GestureDetector gestureDetector;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        setContentView(R.layout.activity_opengl);
        surfaceView = (GLSurfaceView) findViewById(R.id.surfaceview);

        // Set up tap listener.
        /*gestureDetector =
                new GestureDetector(
                        this,
                        new GestureDetector.SimpleOnGestureListener() {
                            @Override
                            public boolean onSingleTapUp(final MotionEvent e) {
                                surfaceView.queueEvent(
                                        () -> JniInterface.onTouched(nativeApplication, e.getX(), e.getY()));
                                return true;
                            }

                            @Override
                            public boolean onDown(MotionEvent e) {
                                return true;
                            }
                        });

        surfaceView.setOnTouchListener(
                (View v, MotionEvent event) -> gestureDetector.onTouchEvent(event));*/

        // Set up renderer.
        surfaceView.setPreserveEGLContextOnPause(true);
        surfaceView.setEGLContextClientVersion(2);
        surfaceView.setEGLConfigChooser(8, 8, 8, 8, 16, 0); // Alpha used for plane blending.
        surfaceView.setRenderer(this);
        surfaceView.setRenderMode(GLSurfaceView.RENDERMODE_CONTINUOUSLY);

        JNIInterface.assetManager = getAssets();
        nativeApplication = JNIInterface.createContext(getAssets());

        //planeStatusCheckingHandler = new Handler();
    }

    @Override
    public void onSurfaceCreated(GL10 gl, EGLConfig config) {
        GLES20.glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        JNIInterface.onGlSurfaceCreated(nativeApplication);
    }

    @Override
    public void onSurfaceChanged(GL10 gl, int width, int height) {
        viewportWidth = width;
        viewportHeight = height;
        viewportChanged = true;
    }

    @Override
    public void onDrawFrame(GL10 gl) {
        // Synchronized to avoid racing onDestroy.
        synchronized (this) {
            if (nativeApplication == 0) {
                return;
            }
            if (viewportChanged) {
                int displayRotation = getWindowManager().getDefaultDisplay().getRotation();
                JNIInterface.onDisplayGeometryChanged(
                        nativeApplication, displayRotation, viewportWidth, viewportHeight);
                viewportChanged = false;
            }
            JNIInterface.onGlSurfaceDrawFrame(nativeApplication);
        }
    }

}
