package com.chess_ar.handtracking.archess;

import android.app.Activity;
import android.content.Context;
import android.content.res.AssetManager;

public class JNIInterface {
    private static final String TAG = "JniInterface";
    static AssetManager assetManager;

    public native static long createContext(AssetManager assetManager, Context ctx);
    public native static void deleteContext(long id);

    public static native void onPause(long nativeApplication);

    public static native void onResume(long nativeApplication, Context context, Activity activity);

    /** Allocate OpenGL resources for rendering. */
    public static native void onGlSurfaceCreated(long nativeApplication);

    /**
     * Called on the OpenGL thread before onGlSurfaceDrawFrame when the view port width, height, or
     * display rotation may have changed.
     */
    public static native void onDisplayGeometryChanged(
            long nativeApplication, int displayRotation, int width, int height);

    /** Main render loop, called on the OpenGL thread. */
    public static native void onGlSurfaceDrawFrame(long nativeApplication);
}
