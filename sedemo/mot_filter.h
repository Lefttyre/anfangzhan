#ifndef MOT_FILTER_H_
#define MOT_FILTER_H_

#include <cstdint>
#include <list>
#include <map>
#include <mutex>
#include <unordered_map>
#include <vector>
#include <deepsort/deepsort.h>

typedef struct
{
    int track_id;
    uint64_t frame_id;
    int x;
    int y;
    int width;
    int height;
    int class_id;
    float confidence;
    float max_k;
}MotObject;

//struct Box{
//  int id;  // useless
//  int clas_ret;  // classification result
//  float confidence;  // confidence threshold
//  int xmin, ymin, xmax, ymax;
//};

//struct Result {
//    int channel;
//    uint64_t frame;
//    std::vector<Box> boxes;
//};


class MotFilter {
 public:
    MotFilter();
    MotFilter(uint32_t height, uint32_t width);
    ~MotFilter();
    void Update(const DS_DetectObjects &frame_result,const uint64_t &track_frame_id,
                std::vector<MotObject> &shot_objects, bool &cache_frame,
                bool &release_frame, uint64_t &release_frame_id);
    DS_Tracker ds_tracker_;
    std::map<int, MotObject> objects_;
    uint64_t max_k_age_;
    std::vector<int> cache_frames_;
    int height_;
    int width_;
};

#endif //  MOT_FILTER_H_
