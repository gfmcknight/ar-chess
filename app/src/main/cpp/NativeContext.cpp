//
// Created by gfm13 on 10/13/2018.
//

#include "NativeContext.h"
#include "logging.h"

static const char *pieceFilename[] = {
        [pt_pawn]       = "models/Pawn.obj",
        [pt_rook]       = "models/Rook.obj",
        [pt_bishop]     = "models/Bishop.obj",
        [pt_knight]     = "models/Knight.obj",
        [pt_queen]      = "models/Queen.obj",
        [pt_king]       = "models/King.obj",
};

static const glm::mat4 pieceMatrix[] = {
        [pt_pawn]       = glm::scale(glm::mat4(1.0f), glm::vec3(0.001f)),
        [pt_rook]       = glm::scale(glm::mat4(1.0f), glm::vec3(0.001f)),
        [pt_bishop]     = glm::scale(glm::mat4(1.0f), glm::vec3(0.001f)),
        [pt_knight]     = glm::scale(glm::mat4(1.0f), glm::vec3(0.001f)),
        [pt_queen]      = glm::scale(glm::mat4(1.0f), glm::vec3(0.001f)),
        [pt_king]       = glm::scale(glm::mat4(1.0f), glm::vec3(0.001f)),
};

struct Piece {
    PieceType type;
    int x;
    int y;
    int player;
};

static std::vector<Piece> pieces;

#define BOARD_SIZE 8
// [y][x]
static glm::vec3 boardOffsets[BOARD_SIZE][BOARD_SIZE];


NativeContext::NativeContext(AAssetManager *assetManager, jobject jContext, JNIEnv *env) {
    this->assetManager = assetManager;

}

NativeContext::~NativeContext() {
    if (ar_session_ != nullptr) {
        ArSession_destroy(ar_session_);
        ArFrame_destroy(ar_frame_);
    }
}

void NativeContext::OnPause() {
    LOGI("OnPause()");
    if (ar_session_ != nullptr) {
        ArSession_pause(ar_session_);
    }
}

void NativeContext::OnResume(void* env, void* context, void* activity) {
    LOGI("OnResume()");

    if (ar_session_ == nullptr) {
        ArInstallStatus install_status;
        // If install was not yet requested, that means that we are resuming the
        // activity first time because of explicit user interaction (such as
        // launching the application)
        bool user_requested_install = !install_requested_;

        // === ATTENTION!  ATTENTION!  ATTENTION! ===
        // This method can and will fail in user-facing situations.  Your
        // application must handle these cases at least somewhat gracefully.  See
        // HelloAR Java sample code for reasonable behavior.
        CHECK(ArCoreApk_requestInstall(env, activity, user_requested_install,
                                       &install_status) == AR_SUCCESS);

        switch (install_status) {
            case AR_INSTALL_STATUS_INSTALLED:
                break;
            case AR_INSTALL_STATUS_INSTALL_REQUESTED:
                install_requested_ = true;
                return;
        }

        // === ATTENTION!  ATTENTION!  ATTENTION! ===
        // This method can and will fail in user-facing situations.  Your
        // application must handle these cases at least somewhat gracefully.  See
        // HelloAR Java sample code for reasonable behavior.
        CHECK(ArSession_create(env, context, &ar_session_) == AR_SUCCESS);
        CHECK(ar_session_);

        ArFrame_create(ar_session_, &ar_frame_);
        CHECK(ar_frame_);

        ArSession_setDisplayGeometry(ar_session_, display_rotation_, width_,
                                     height_);
    }

    const ArStatus status = ArSession_resume(ar_session_);
    CHECK(status == AR_SUCCESS);

}


void NativeContext::OnSurfaceCreated() {
    LOGI("OnSurfaceCreated()");

    background_renderer_.InitializeGlContent(assetManager);
    /*point_cloud_renderer_.InitializeGlContent(assetManager);
    andy_renderer_.InitializeGlContent(assetManager, "models/andy.obj",
                                       "models/andy.png");
    plane_renderer_.InitializeGlContent(assetManager);*/

    for (int t = 0; t < (int)pt_MAX; t++) {
        pieceRenderers[t].InitializeGlContent(assetManager, pieceFilename[t], "models/Wood.png");
    }

    for (int y = 0; y < BOARD_SIZE; y++) {
        for (int x = 0; x < BOARD_SIZE; x++) {
            boardOffsets[y][x] = glm::vec3(
                    ((float)x - (BOARD_SIZE - 1.0f) / 2.0f) * 30.f,
                    0.f,
                    ((float)y - (BOARD_SIZE - 1.0f) / 2.0f) * 30.f - 50.f
            );
        }
    }

    static const PieceType row[] = {
            pt_rook, pt_knight, pt_bishop, pt_queen, pt_king, pt_bishop, pt_knight, pt_rook
    };
    for (int x = 0; x < sizeof(row) / sizeof(row[0]); x++) {
        pieces.push_back((Piece){.type = pt_pawn, .x = x, .y = 1, .player = 0});
        pieces.push_back((Piece){.type = pt_pawn, .x = x, .y = 6, .player = 1});
        pieces.push_back((Piece){.type = row[x], .x = x, .y = 0, .player = 0});
        pieces.push_back((Piece){.type = row[x], .x = x, .y = 7, .player = 1});
    }
}

void NativeContext::OnDisplayGeometryChanged(int display_rotation,
                                                  int width, int height) {
    LOGI("OnSurfaceChanged(%d, %d)", width, height);
    glViewport(0, 0, width, height);
    display_rotation_ = display_rotation;
    width_ = width;
    height_ = height;
    if (ar_session_ != nullptr) {
        ArSession_setDisplayGeometry(ar_session_, display_rotation, width, height);
    }
}

void NativeContext::RenderBoard(glm::mat4 projection_mat, glm::mat4 view_mat, float color_correction[4]) {

}

void NativeContext::RenderPieces(glm::mat4 projection_mat, glm::mat4 view_mat, float color_correction[4]) {
    float c[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    /*glm::mat4 model_base = glm::scale(glm::mat4(1.0f), glm::vec3(0.001f));
    for (int t = 0; t < (int)pt_MAX; t++) {
        glm::mat4 model_mat = glm::translate(model_base, glm::vec3(50.f * t, 50.f * t, 50.f * t));
        pieceRenderers[t].Draw(projection_mat, view_mat, model_mat, color_correction, c);
    }*/

    for (auto &p : pieces) {
        glm::mat4 model_mat = glm::translate(pieceMatrix[p.type], boardOffsets[p.y][p.x]);
        pieceRenderers[p.type].Draw(projection_mat, view_mat, model_mat, color_correction, c);
    }
}

void NativeContext::OnDrawFrame() {
    // Render the scene.
    glClearColor(0.1f, 0.1f, 0.9f, 1.0f);
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    if (ar_session_ == nullptr) return;

    ArSession_setCameraTextureName(ar_session_,
                                   background_renderer_.GetTextureId());

    // Update session to get current frame and render camera background.
    if (ArSession_update(ar_session_, ar_frame_) != AR_SUCCESS) {
        LOGE("HelloArApplication::OnDrawFrame ArSession_update error");
    }

    ArCamera* ar_camera;
    ArFrame_acquireCamera(ar_session_, ar_frame_, &ar_camera);

    glm::mat4 view_mat;
    glm::mat4 projection_mat;
    ArCamera_getViewMatrix(ar_session_, ar_camera, glm::value_ptr(view_mat));
    ArCamera_getProjectionMatrix(ar_session_, ar_camera,
            /*near=*/0.1f, /*far=*/100.f,
                                 glm::value_ptr(projection_mat));

    ArTrackingState camera_tracking_state;
    ArCamera_getTrackingState(ar_session_, ar_camera, &camera_tracking_state);
    ArCamera_release(ar_camera);

    background_renderer_.Draw(ar_session_, ar_frame_);

    // If the camera isn't tracking don't bother rendering other objects.
    if (camera_tracking_state != AR_TRACKING_STATE_TRACKING) {
        return;
    }

    // Get light estimation value.
    ArLightEstimate* ar_light_estimate;
    ArLightEstimateState ar_light_estimate_state;
    ArLightEstimate_create(ar_session_, &ar_light_estimate);

    ArFrame_getLightEstimate(ar_session_, ar_frame_, ar_light_estimate);
    ArLightEstimate_getState(ar_session_, ar_light_estimate,
                             &ar_light_estimate_state);

    // Set light intensity to default. Intensity value ranges from 0.0f to 1.0f.
    // The first three components are color scaling factors.
    // The last one is the average pixel intensity in gamma space.
    float color_correction[4] = {1.f, 1.f, 1.f, 1.f};
    if (ar_light_estimate_state == AR_LIGHT_ESTIMATE_STATE_VALID) {
        ArLightEstimate_getColorCorrection(ar_session_, ar_light_estimate,
                                           color_correction);
    }

    ArLightEstimate_destroy(ar_light_estimate);
    ar_light_estimate = nullptr;

    RenderBoard(projection_mat, view_mat, color_correction);
    RenderPieces(projection_mat, view_mat, color_correction);

    /*
    // Render Andy objects.
    glm::mat4 model_mat(1.0f);
    for (const auto& colored_anchor : anchors_) {
        ArTrackingState tracking_state = AR_TRACKING_STATE_STOPPED;
        ArAnchor_getTrackingState(ar_session_, colored_anchor.anchor,
                                  &tracking_state);
        if (tracking_state == AR_TRACKING_STATE_TRACKING) {
            // Render object only if the tracking state is AR_TRACKING_STATE_TRACKING.
            util::GetTransformMatrixFromAnchor(*colored_anchor.anchor, ar_session_,
                                               &model_mat);
            andy_renderer_.Draw(projection_mat, view_mat, model_mat, color_correction,
                                colored_anchor.color);
        }
    }

    // Update and render planes.
    ArTrackableList* plane_list = nullptr;
    ArTrackableList_create(ar_session_, &plane_list);
    CHECK(plane_list != nullptr);

    ArTrackableType plane_tracked_type = AR_TRACKABLE_PLANE;
    ArSession_getAllTrackables(ar_session_, plane_tracked_type, plane_list);

    int32_t plane_list_size = 0;
    ArTrackableList_getSize(ar_session_, plane_list, &plane_list_size);
    plane_count_ = plane_list_size;

    for (int i = 0; i < plane_list_size; ++i) {
        ArTrackable* ar_trackable = nullptr;
        ArTrackableList_acquireItem(ar_session_, plane_list, i, &ar_trackable);
        ArPlane* ar_plane = ArAsPlane(ar_trackable);
        ArTrackingState out_tracking_state;
        ArTrackable_getTrackingState(ar_session_, ar_trackable,
                                     &out_tracking_state);

        ArPlane* subsume_plane;
        ArPlane_acquireSubsumedBy(ar_session_, ar_plane, &subsume_plane);
        if (subsume_plane != nullptr) {
            ArTrackable_release(ArAsTrackable(subsume_plane));
            continue;
        }

        if (ArTrackingState::AR_TRACKING_STATE_TRACKING != out_tracking_state) {
            continue;
        }

        ArTrackingState plane_tracking_state;
        ArTrackable_getTrackingState(ar_session_, ArAsTrackable(ar_plane),
                                     &plane_tracking_state);
        if (plane_tracking_state == AR_TRACKING_STATE_TRACKING) {
            const auto iter = plane_color_map_.find(ar_plane);
            glm::vec3 color;
            if (iter != plane_color_map_.end()) {
                color = iter->second;

                // If this is an already observed trackable release it so it doesn't
                // leave an additional reference dangling.
                ArTrackable_release(ar_trackable);
            } else {
                // The first plane is always white.
                if (!first_plane_has_been_found_) {
                    first_plane_has_been_found_ = true;
                    color = kWhite;
                } else {
                    color = GetRandomPlaneColor();
                }
                plane_color_map_.insert({ar_plane, color});
            }

            plane_renderer_.Draw(projection_mat, view_mat, *ar_session_, *ar_plane,
                                 color);
        }
    }

    ArTrackableList_destroy(plane_list);
    plane_list = nullptr;

    // Update and render point cloud.
    ArPointCloud* ar_point_cloud = nullptr;
    ArStatus point_cloud_status =
            ArFrame_acquirePointCloud(ar_session_, ar_frame_, &ar_point_cloud);
    if (point_cloud_status == AR_SUCCESS) {
        point_cloud_renderer_.Draw(projection_mat * view_mat, ar_session_,
                                   ar_point_cloud);
        ArPointCloud_release(ar_point_cloud);
    }*/
}
