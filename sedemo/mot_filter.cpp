#include <string.h>
#include "mot_filter.h"

MotFilter::MotFilter() {
    ds_tracker_ = DS_Create();
    max_k_age_ = 30;
}

MotFilter::MotFilter(uint32_t height, uint32_t width): height_(height),
    width_(width) {
    ds_tracker_ = DS_Create();
    max_k_age_ = 30;
}

MotFilter::~MotFilter() {
    DS_Delete(ds_tracker_);
    objects_.clear();
    cache_frames_.clear();
}

void MotFilter::Update(const DS_DetectObjects &frame_result,
                       const uint64_t &track_frame_id,
                        std::vector<MotObject>& shot_objects,
                        bool& cache_frame, bool& release_frame,
                        uint64_t& release_frame_id) {
    DS_DetectObjects ds_detect_objects = frame_result;
    //DS_DetectObject ds_detect_object;
    DS_TrackObjects ds_track_objects;

//    for (auto it : frame_result.boxes) {
//        ds_detect_object.class_id = it.clas_ret;
//        ds_detect_object.confidence = it.confidence;
//        ds_detect_object.rect.x = it.xmin;
//        ds_detect_object.rect.y = it.ymin;
//        ds_detect_object.rect.width = it.xmax - it.xmin;
//        ds_detect_object.rect.height = it.ymax - it.ymin;
//        ds_detect_objects.push_back(ds_detect_object);
//    }

    DS_Update(ds_tracker_, ds_detect_objects, ds_track_objects);
    cache_frame = false;
    release_frame = false;

    for (auto it : ds_track_objects) {
        MotObject temp_object;
        temp_object.frame_id = track_frame_id;
        temp_object.track_id = it.track_id;
        temp_object.class_id = it.class_id;
        temp_object.confidence = it.confidence;
        temp_object.x = it.rect.x;
        temp_object.y = it.rect.y;
        temp_object.width = it.rect.width;
        temp_object.height = it.rect.height;
        //temp_object.max_k = temp_object.confidence * temp_object.width *
        //                    temp_object.height;

        temp_object.max_k = temp_object.width * temp_object.height;

        auto tracker_iter = objects_.find(it.track_id);

        if (tracker_iter == objects_.end()) {
            objects_.insert(std::pair<int, MotObject>(
                            it.track_id, temp_object));
            cache_frame = true;
        } else {
            if (temp_object.x < 0 || temp_object.y < 0
                    || temp_object.x + temp_object.width > width_
                    || temp_object.y + temp_object.height > height_) {
                continue;
            }

            if (temp_object.max_k > tracker_iter->second.max_k) {
                memcpy(&(tracker_iter->second), &temp_object,
                       sizeof(MotObject));
                cache_frame = true;
            }
        }
    }

    shot_objects.clear();

    for (auto tracker_iter = objects_.begin(); tracker_iter != objects_.end();
            tracker_iter++) {
        if ((track_frame_id - tracker_iter->second.frame_id) > max_k_age_) {
            // shot
            shot_objects.push_back(tracker_iter->second);
            objects_.erase(tracker_iter);
        }
    }

    if (cache_frame) {
        cache_frames_.push_back(track_frame_id);
    }

    if (!cache_frames_.empty()) {
        std::vector<int>::iterator cache_frame_iter;
        cache_frame_iter = cache_frames_.begin();

        if ((track_frame_id - (*cache_frame_iter)) > max_k_age_) {
            release_frame = true;
            release_frame_id = (*cache_frame_iter);
            cache_frames_.erase(cache_frame_iter);
        }
    }
}
