//
// Created by gfm13 on 10/13/2018.
//

#ifndef ARCHESS_NATIVECONTEXT_H
#define ARCHESS_NATIVECONTEXT_H

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <media/NdkImage.h>
#include <android/asset_manager.h>

#include <unordered_map>

#include "arcore_c_api.h"
#include "background_renderer.h"
#include "obj_renderer.h"
#include "plane_renderer.h"
#include "glm.h"

#define BOARD_SIZE 8

enum PieceType {
    pt_pawn,
    pt_rook,
    pt_bishop,
    pt_knight,
    pt_queen,
    pt_king,
    pt_MAX
};

static const int FILTER_WIDTH = 480;
static const int FILTER_HEIGHT = 270;

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

    // Returns true if any planes have been detected.  Used for hiding the
    // "searching for planes" snackbar.
    bool HasDetectedPlanes() const { return plane_count_ > 0; }

    // OnTouched is called on the OpenGL thread after the user touches the screen.
    // @param x: x position on the screen (pixels).
    // @param y: y position on the screen (pixels).
    void OnTouched(float x, float y);

private:
    AAssetManager* assetManager;
    ArSession* ar_session_ = nullptr;
    ArFrame* ar_frame_ = nullptr;
    std::vector<ArAnchor *> anchors_;

    bool initialized_ = false;
    bool install_requested_ = false;
    int width_ = 1;
    int height_ = 1;
    int display_rotation_ = 0;
    int32_t plane_count_ = 0;

    PieceType board[BOARD_SIZE][BOARD_SIZE];

    // The first plane is always rendered in white, if this is true then a plane
    // at some point has been found.
    bool first_plane_has_been_found_ = false;

    // Stores the randomly-selected color each plane is drawn with
    std::unordered_map<ArPlane*, glm::vec3> plane_color_map_;

    //PointCloudRenderer point_cloud_renderer_;
    BackgroundRenderer background_renderer_;
    hello_ar::PlaneRenderer plane_renderer_;
    //ObjRenderer andy_renderer_;

    hello_ar::ObjRenderer pieceRenderers[pt_MAX];
    hello_ar::ObjRenderer boardRenderer;

    void RenderBoard(glm::mat4 &projection_mat, glm::mat4 &view_mat,
                     glm::mat4 &model_mat, float color_correction[4], uint8_t *filter);
    void RenderPieces(glm::mat4 &projection_mat, glm::mat4 &view_mat,
                      glm::mat4 &model_mat, float color_correction[4], uint8_t *filter);

    uint8_t * getFilterTexture(const glm::mat4 &projection_mat) const;
};


#endif //ARCHESS_NATIVECONTEXT_H
