//
// Created by gfm13 on 10/13/2018.
//

#ifndef ARCHESS_NATIVECONTEXT_H
#define ARCHESS_NATIVECONTEXT_H

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <android/asset_manager.h>

#include "arcore_c_api.h"
#include "background_renderer.h"
#include "obj_renderer.h"
#include "glm.h"

enum PieceType {
    pt_pawn,
    pt_rook,
    pt_bishop,
    pt_knight,
    pt_queen,
    pt_king,
    pt_MAX
};

class NativeContext {
public:
    NativeContext(AAssetManager* assetManager, jobject jContext, JNIEnv *env);
    ~NativeContext();

    // OnPause is called on the UI thread from the Activity's onPause method.
    void OnPause();

    // OnResume is called on the UI thread from the Activity's onResume method.
    void OnResume(void* env, void* context, void* activity);

    // OnSurfaceCreated is called on the OpenGL thread when GLSurfaceView
    // is created.
    void OnSurfaceCreated();

    // OnDisplayGeometryChanged is called on the OpenGL thread when the
    // render surface size or display rotation changes.
    //
    // @param display_rotation: current display rotation.
    // @param width: width of the changed surface view.
    // @param height: height of the changed surface view.
    void OnDisplayGeometryChanged(int display_rotation, int width, int height);

    // OnDrawFrame is called on the OpenGL thread to render the next frame.
    void OnDrawFrame();

private:
    AAssetManager* assetManager;
    ArSession* ar_session_ = nullptr;
    ArFrame* ar_frame_ = nullptr;

    bool install_requested_ = false;
    int width_ = 1;
    int height_ = 1;
    int display_rotation_ = 0;

    //PointCloudRenderer point_cloud_renderer_;
    BackgroundRenderer background_renderer_;
    //PlaneRenderer plane_renderer_;
    //ObjRenderer andy_renderer_;

    hello_ar::ObjRenderer pieceRenderers[pt_MAX];

    void RenderBoard(glm::mat4 projection_mat, glm::mat4 view_mat, float color_correction[4]);
    void RenderPieces(glm::mat4 projection_mat, glm::mat4 view_mat, float color_correction[4]);
};


#endif //ARCHESS_NATIVECONTEXT_H
