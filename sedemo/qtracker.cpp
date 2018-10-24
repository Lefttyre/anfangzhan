#include "qtracker.h"
#include <fstream>
QTracker::QTracker()
{
    cache_frames_.resize(channels);
    std::string label_file = "../ssd/labelmap_voc.txt";

    std::ifstream labels(label_file);
    std::string line;
    while (std::getline(labels, line)) {
        m_labels.push_back(std::string(line));
    }
    labels.close();
    //create trackers
    for (uint i = 0; i < channels; ++i) {
      track_channels_.push_back(std::make_shared<MotFilter>());
    }

}

void QTracker::tracker(QVariant dataVar){
    // tranform to object for tracker needed
    DetectionObject askData;
    askData = dataVar.value<DetectionObject>();
    uint image_numbers = askData.batch_images_for_tracker.size();

    for(uint p=0; p<image_numbers; p++){
        cv::Mat trackMat = askData.batch_images_for_tracker[p];
        DS_DetectObjects detect_objects;
        for(auto& obj:askData.objects[p]){
            DS_Rect rect;
            DS_DetectObject detect_object;
            rect.x = static_cast<int>(obj.xmin);
            rect.y = static_cast<int>(obj.ymin);
            rect.width = static_cast<int>(obj.xmax - obj.xmin);
            rect.height = static_cast<int>(obj.ymax - obj.ymin);
            detect_object.rect = rect;
            detect_object.confidence = obj.confidence;
            detect_object.class_id = obj.cls_id;
            detect_objects.push_back(detect_object);
        }

        TrackAndObjectOutput(trackMat,detect_objects,askData.imgidx[p].m_intPos,askData.imgidx[p].m_intChn);
    }
}

void QTracker::TrackAndObjectOutput(cv::Mat& image_origin, const DS_DetectObjects &detect_objects, const uint64_t &frame_id, const uint32_t &channel_id){
    auto pmot_filter = track_channels_[channel_id];
    std::vector<MotObject> shot_objects;
    bool cache_frame;
    bool release_frame;
    uint64_t release_frame_id;

    pmot_filter->Update(detect_objects, frame_id, shot_objects, cache_frame, release_frame, release_frame_id);

    if(cache_frame){
        cache_frames_[channel_id].insert(std::pair<uint64_t, cv::Mat>(frame_id, image_origin));
        cache_frame_count_ ++;
    }

    for(auto& shot_object:shot_objects)
    {
        std::map<uint64_t, cv::Mat>::iterator iter;
        cv::Mat object_frame;
        iter = cache_frames_[channel_id].find(shot_object.frame_id);
        if(iter != cache_frames_[channel_id].end())
        {
            object_frame = iter->second;
        }
        else
        {
            continue;
        }
        if(shot_object.x < 0) shot_object.x = 0;
        if(shot_object.y < 0) shot_object.y = 0;
        if(shot_object.width < 0) shot_object.width = 0;
        if(shot_object.height < 0) shot_object.height = 0;
        if(shot_object.x + shot_object.width > object_frame.cols) shot_object.width = object_frame.cols -shot_object.x;
        if(shot_object.y + shot_object.height > object_frame.rows) shot_object.height = object_frame.rows -shot_object.y;
        cv::Rect cut_rect(shot_object.x,shot_object.y,shot_object.width,shot_object.height);
        cv::Mat cut_img=object_frame(cut_rect);
        pPicRrovider->addMat(cut_img, channel_id,shot_object.track_id);

        std::string object_class = m_labels[shot_object.class_id];
        char msg[20];
        snprintf(msg, sizeof(msg), "%s", object_class.c_str());
//        char *object_msg = msg;
//        if(objcb_ != nullptr)
//        {
//          (*objcb_)(channel_id, cut_img, object_msg);
//        }
    } //  for(auto& shot_object:shot_objects)

    if(release_frame){
        std::map<uint64_t, cv::Mat>::iterator iter_release;
        iter_release = cache_frames_[channel_id].find(release_frame_id);
        if(iter_release != cache_frames_[channel_id].end())
        {
            cache_frames_[channel_id].erase(iter_release);
            cache_frame_count_ --;
        }
    } //  if(release_frame)
}
