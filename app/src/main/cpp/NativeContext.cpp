//
// Created by gfm13 on 10/13/2018.
//

#include "NativeContext.h"
#include "logging.h"

NativeContext::NativeContext(AAssetManager *assetManager) {
    this->assetManager = assetManager;
}

void NativeContext::OnSurfaceCreated() {
    LOGI("OnSurfaceCreated()");

    /*background_renderer_.InitializeGlContent(assetManager);
    point_cloud_renderer_.InitializeGlContent(assetManager);
    andy_renderer_.InitializeGlContent(assetManager, "models/andy.obj",
                                       "models/andy.png");
    plane_renderer_.InitializeGlContent(assetManager);*/
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

void NativeContext::OnDrawFrame() {
    // Render the scene.
    glClearColor(0.9f, 0.9f, 0.9f, 1.0f);
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

    /*
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
