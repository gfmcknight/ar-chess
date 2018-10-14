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
        [pt_pawn]       = glm::scale(glm::mat4(1.0f), glm::vec3(1.0f)),
        [pt_rook]       = glm::scale(glm::mat4(1.0f), glm::vec3(1.0f)),
        [pt_bishop]     = glm::scale(glm::mat4(1.0f), glm::vec3(1.0f)),
        [pt_knight]     = glm::scale(glm::mat4(1.0f), glm::vec3(1.0f)),
        [pt_queen]      = glm::scale(glm::mat4(1.0f), glm::vec3(1.0)),
        [pt_king]       = glm::scale(glm::mat4(1.0f), glm::vec3(1.0))
};

struct Piece {
    PieceType type;
    int x;
    int y;
    int player;
};

static std::vector<Piece> pieces;

static uint8_t filter[FILTER_WIDTH * FILTER_HEIGHT * 4];

// [y][x]
static glm::vec3 boardOffsets[BOARD_SIZE][BOARD_SIZE];

static const glm::vec3 kWhite = {255, 255, 255};

NativeContext::NativeContext(AAssetManager *assetManager, jobject jContext, JNIEnv *env) {
    this->assetManager = assetManager;
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            board[i][j] = pt_MAX;
            if (j == 1 || j == 6) {
                board[i][j] = pt_pawn;
            }
        }
    }

    board[0][0] = pt_rook;
    board[1][0] = pt_knight;
    board[2][0] = pt_bishop;
    board[3][0] = pt_queen;
    board[4][0] = pt_king;
    board[5][0] = pt_bishop;
    board[6][0] = pt_knight;
    board[7][0] = pt_rook;

    board[0][7] = pt_rook;
    board[1][7] = pt_knight;
    board[2][7] = pt_bishop;
    board[3][7] = pt_queen;
    board[4][7] = pt_king;
    board[5][7] = pt_bishop;
    board[6][7] = pt_knight;
    board[7][7] = pt_rook;
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
                                       "models/andy.png");*/
    plane_renderer_.InitializeGlContent(assetManager);

    for (int t = 0; t < (int)pt_MAX; t++) {
        pieceRenderers[t].InitializeGlContent(assetManager, pieceFilename[t], "models/Wood.png");
        //pieceRenderers[t].RecenterVertices();
    }

    for (int y = 0; y < BOARD_SIZE; y++) {
        for (int x = 0; x < BOARD_SIZE; x++) {
            boardOffsets[y][x] = glm::vec3(
                    ((float)x - (BOARD_SIZE - 1.0f) / 2.0f) * 0.03f,
                    0.f,
                    ((float)y - (BOARD_SIZE - 1.0f) / 2.0f) * 0.03f - 0.05f
            );
        }
    }

    boardRenderer.InitializeGlContent(assetManager,"models/Board.obj", "models/Board.png");

    static const PieceType row[] = {
            pt_rook, pt_knight, pt_bishop, pt_queen, pt_king, pt_bishop, pt_knight, pt_rook
    };
    for (int x = 0; x < sizeof(row) / sizeof(row[0]); x++) {
        pieces.push_back((Piece){.type = pt_pawn, .x = x, .y = 1, .player = 0});
        pieces.push_back((Piece){.type = pt_pawn, .x = x, .y = 6, .player = 1});
        pieces.push_back((Piece){.type = row[x], .x = x, .y = 0, .player = 0});
        pieces.push_back((Piece){.type = row[x], .x = x, .y = 7, .player = 1});
    }

    last_board_model_ = glm::mat4(1.0f);
    last_board_view_ = glm::mat4(1.0f);
    last_board_projection_ = glm::mat4(1.0f);
    initialized_ = true;
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

void NativeContext::RenderBoard(glm::mat4 &projection_mat, glm::mat4 &view_mat,
                                glm::mat4 &model_mat, float color_correction[4], uint8_t *filter) {
    float c[] = {1.0f, 1.0f, 1.0f, 1.0f};
    glm::mat4 matrix = glm::translate(glm::scale(glm::mat4(1.0f), glm::vec3(0.00245)), glm::vec3(0, 0, -22.0));
    matrix = model_mat * matrix;
    boardRenderer.Draw(projection_mat, view_mat, matrix, color_correction, c,
                       FILTER_WIDTH, FILTER_HEIGHT, filter);
}

void NativeContext::RenderPieces(glm::mat4 &projection_mat, glm::mat4 &view_mat,
                                 glm::mat4 &model_mat, float color_correction[4], uint8_t *filter) {
    float c[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    /*glm::mat4 model_base = glm::scale(glm::mat4(1.0f), glm::vec3(0.001f));
    for (int t = 0; t < (int)pt_MAX; t++) {
        glm::mat4 model_mat = glm::translate(model_base, glm::vec3(50.f * t, 50.f * t, 50.f * t));
        pieceRenderers[t].Draw(projection_mat, view_mat, model_mat, color_correction, c);
    }*/

    last_board_model_ = model_mat;
    last_board_view_ = view_mat;
    last_board_projection_ = projection_mat;

    for (int x = 0; x < BOARD_SIZE; x++) {
        for (int y = 0; y < BOARD_SIZE; y++) {
            if (board[x][y] == pt_MAX) continue;
            glm::mat4 matrix = glm::translate(pieceMatrix[board[x][y]], boardOffsets[y][x]);
            matrix = model_mat * matrix;
            pieceRenderers[board[x][y]].Draw(projection_mat, view_mat, matrix, color_correction, c,
                                             FILTER_WIDTH, FILTER_HEIGHT, filter);
        }
    }
}

void NativeContext::OnDrawFrame() {
    if (! initialized_) {
        return;
    }

    // Render the scene.
    glClearColor(0.1f, 0.1f, 0.9f, 1.0f);
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    if (ar_session_ == nullptr) return;

    /*
    // Acquire the image from the camera
    ArImage* cameraImage = nullptr;
    const AImage* ndkCameraImage = nullptr;
    ArFrame_acquireCameraImage(ar_session_, ar_frame_, &cameraImage);
    ArImage_getNdkImage(cameraImage, &ndkCameraImage);
    int format;
    AImage_getFormat(ndkCameraImage, &format);

    uint8_t* dataptr = nullptr;
    int datalength  = 0;
    AImage_getPlaneData(ndkCameraImage, 0, &dataptr, &datalength);

    ArImage_release(cameraImage);
     */

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

    if (anchors_.size()) {
        ArTrackingState tracking_state = AR_TRACKING_STATE_STOPPED;
        ArAnchor_getTrackingState(ar_session_, anchors_[0],
                                  &tracking_state);
        if (tracking_state == AR_TRACKING_STATE_TRACKING) {
            // Render object only if the tracking state is AR_TRACKING_STATE_TRACKING.
            glm::mat4 model_mat(1.0f);
            util::GetTransformMatrixFromAnchor(*(anchors_[0]), ar_session_,
                                               &model_mat);
            uint8_t* filter = getFilterTexture(projection_mat);

            RenderBoard(projection_mat, view_mat, model_mat, color_correction, filter);
            RenderPieces(projection_mat, view_mat, model_mat, color_correction, filter);
        }
    }

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
    }*/

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
                    color = kWhite /*GetRandomPlaneColor()*/;
                }
                plane_color_map_.insert({ar_plane, color});
            }

            plane_renderer_.Draw(projection_mat, view_mat, *ar_session_, *ar_plane,
                                 color);
        }
    }

    ArTrackableList_destroy(plane_list);
    plane_list = nullptr;

    /*
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

uint8_t * NativeContext::getFilterTexture(const glm::mat4 &projection_mat) const {
    util::ScopedArPose pose(ar_session_);
    ArAnchor_getPose(ar_session_, anchors_[0], pose.GetArPose());
    float rawPose[7];
    ArPose_getPoseRaw(ar_session_, pose.GetArPose(), rawPose);

    ArPointCloud* pointCloud;
    const float* cloudData = nullptr;
    int numberOfPoints = 0;

    ArFrame_acquirePointCloud(ar_session_, ar_frame_, &pointCloud);
    ArPointCloud_getData(ar_session_, pointCloud, &cloudData);
    ArPointCloud_getNumberOfPoints(ar_session_, pointCloud, &numberOfPoints);

    memset(filter, 0, FILTER_WIDTH * FILTER_HEIGHT * 4 * sizeof(uint8_t));

    for (int i = 0; i < numberOfPoints; i += 4) {
        if (*(cloudData + i + 1) >= rawPose[6]) {
            const glm::vec3 point3d = glm::vec3(
                           *(cloudData + i),
                           *(cloudData + i + 1),
                           *(cloudData + i + 2));

            glm::vec3 point2d = glm::project(point3d, glm::mat4(1.0f), projection_mat, glm::vec4(0, 0, FILTER_WIDTH, FILTER_HEIGHT));
            int x = (int)point2d.x;
            int y = (int)point2d.y;
            if (x < 0 || x >= FILTER_WIDTH || y < 0 || y >= FILTER_HEIGHT) continue;
            filter[4 * (x + y * FILTER_WIDTH)    ] = 0XFF;
            filter[4 * (x + y * FILTER_WIDTH) + 1] = 0XFF;
            filter[4 * (x + y * FILTER_WIDTH) + 2] = 0XFF;
            filter[4 * (x + y * FILTER_WIDTH) + 3] = 0XFF;
        }
    }

    ArPointCloud_release(pointCloud);
    return filter;
}

void NativeContext::SquareTouched(int boardX, int boardY, float screenX, float screenY) {
    LOGI("%s %d %d %f %f\n", "We did it!!!!!", boardX, boardY, screenX, screenY);
}

void NativeContext::OnTouched(float x, float y) {
    if (ar_frame_ == nullptr || ar_session_ == nullptr || !initialized_) {
        return;
    }

    if (anchors_.size()) {
        // We have a board

        glm::mat4 PV = last_board_projection_ * last_board_view_;
        int width = 1080, height = 1920;
        glm::vec4 viewport(0.f, 0.f, (float)width, (float)height);

        float bestDistance2 = MAXFLOAT;
        float bestScreenX = 0, bestScreenY = 0;
        int bestBoardX = 0, bestBoardY = 0;
        std::string line;
        for (int bx = 0; bx < BOARD_SIZE; bx++) {
            line = "" + std::to_string(bx) + "\t: ";
            for (int by = 0; by < BOARD_SIZE; by++) {
                glm::mat4 matrix = glm::translate(pieceMatrix[board[bx][by]],
                        boardOffsets[by][bx] + glm::vec3(0, 0, -0.022f));
                matrix = last_board_model_ * matrix;
                glm::vec3 screenCoord = glm::project(glm::vec3(0.0f), matrix, PV, viewport);
                line += "(" + std::to_string(screenCoord.x) + ", " + std::to_string(screenCoord.y) + ")\t";
                float distanceX = x - screenCoord.x;
                float distanceY = y - screenCoord.y;
                float distance2 = distanceX * distanceX + distanceY * distanceY;
                if (bestDistance2 > distance2) {
                    bestDistance2 = distance2;
                    bestBoardX = bx;
                    bestBoardY = by;
                    bestScreenX = screenCoord.x;
                    bestScreenY = screenCoord.y;
                }
            }
            LOGI("did it %s\n", line.c_str());
        }

        if (sqrt(bestDistance2) < 0.4 * (float)(width + height) / 2.0) {
            SquareTouched(bestBoardX, bestBoardY, x, y);
            return;
        } else {
            LOGI("We did it almost (%d, %d) best distance %f from:\n\ttouch (%f, %f) to square (%f, %f)\n",
                bestBoardX, bestBoardY, sqrt(bestDistance2), x, y, bestScreenX, bestScreenY);
            return;
        }
    }

    ArHitResultList* hit_result_list = nullptr;
    ArHitResultList_create(ar_session_, &hit_result_list);
    CHECK(hit_result_list);
    ArFrame_hitTest(ar_session_, ar_frame_, x, y, hit_result_list);

    int32_t hit_result_list_size = 0;
    ArHitResultList_getSize(ar_session_, hit_result_list,
                            &hit_result_list_size);

    // The hitTest method sorts the resulting list by distance from the camera,
    // increasing.  The first hit result will usually be the most relevant when
    // responding to user input.

    ArHitResult* ar_hit_result = nullptr;
    ArTrackableType trackable_type = AR_TRACKABLE_NOT_VALID;
    for (int32_t i = 0; i < hit_result_list_size; ++i) {
        ArHitResult* ar_hit = nullptr;
        ArHitResult_create(ar_session_, &ar_hit);
        ArHitResultList_getItem(ar_session_, hit_result_list, i, ar_hit);

        if (ar_hit == nullptr) {
            LOGE("HelloArApplication::OnTouched ArHitResultList_getItem error");
            return;
        }

        ArTrackable* ar_trackable = nullptr;
        ArHitResult_acquireTrackable(ar_session_, ar_hit, &ar_trackable);
        ArTrackableType ar_trackable_type = AR_TRACKABLE_NOT_VALID;
        ArTrackable_getType(ar_session_, ar_trackable, &ar_trackable_type);
        // Creates an anchor if a plane or an oriented point was hit.
        if (AR_TRACKABLE_PLANE == ar_trackable_type) {
            ArPose* hit_pose = nullptr;
            ArPose_create(ar_session_, nullptr, &hit_pose);
            ArHitResult_getHitPose(ar_session_, ar_hit, hit_pose);
            int32_t in_polygon = 0;
            ArPlane* ar_plane = ArAsPlane(ar_trackable);
            ArPlane_isPoseInPolygon(ar_session_, ar_plane, hit_pose, &in_polygon);

            // Use hit pose and camera pose to check if hittest is from the
            // back of the plane, if it is, no need to create the anchor.
            ArPose* camera_pose = nullptr;
            ArPose_create(ar_session_, nullptr, &camera_pose);
            ArCamera* ar_camera;
            ArFrame_acquireCamera(ar_session_, ar_frame_, &ar_camera);
            ArCamera_getPose(ar_session_, ar_camera, camera_pose);
            ArCamera_release(ar_camera);
            float normal_distance_to_plane = util::CalculateDistanceToPlane(
                    *ar_session_, *hit_pose, *camera_pose);

            ArPose_destroy(hit_pose);
            ArPose_destroy(camera_pose);

            if (!in_polygon || normal_distance_to_plane < 0) {
                continue;
            }

            ar_hit_result = ar_hit;
            trackable_type = ar_trackable_type;
            break;
        } else if (AR_TRACKABLE_POINT == ar_trackable_type) {
            ArPoint* ar_point = ArAsPoint(ar_trackable);
            ArPointOrientationMode mode;
            ArPoint_getOrientationMode(ar_session_, ar_point, &mode);
            if (AR_POINT_ORIENTATION_ESTIMATED_SURFACE_NORMAL == mode) {
                ar_hit_result = ar_hit;
                trackable_type = ar_trackable_type;
                break;
            }
        }
    }

    if (ar_hit_result) {
        // Note that the application is responsible for releasing the anchor
        // pointer after using it. Call ArAnchor_release(anchor) to release.
        ArAnchor* anchor = nullptr;
        if (ArHitResult_acquireNewAnchor(ar_session_, ar_hit_result, &anchor) !=
            AR_SUCCESS) {
            LOGE(
                    "HelloArApplication::OnTouched ArHitResult_acquireNewAnchor error");
            return;
        }

        ArTrackingState tracking_state = AR_TRACKING_STATE_STOPPED;
        ArAnchor_getTrackingState(ar_session_, anchor, &tracking_state);
        if (tracking_state != AR_TRACKING_STATE_TRACKING) {
            ArAnchor_release(anchor);
            return;
        }

        if (anchors_.size() >= 1) {
            ArAnchor_release(anchors_[0]);
            anchors_.erase(anchors_.begin());
        }

        // Assign a color to the object for rendering based on the trackable type
        // this anchor attached to. For AR_TRACKABLE_POINT, it's blue color, and
        // for AR_TRACKABLE_PLANE, it's green color.
        anchors_.push_back(anchor);

        ArHitResult_destroy(ar_hit_result);
        ar_hit_result = nullptr;

        ArHitResultList_destroy(hit_result_list);
        hit_result_list = nullptr;
    }
}
