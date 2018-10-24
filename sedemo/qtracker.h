#pragma once
#ifndef QTRACKER_H
#define QTRACKER_H
#include <iostream>
#include <vector>
#include <memory>
#include <QObject>
#include "mot_filter.h"
#include "QPicProvider.h"
#include "GlobalVariable.h"

class QTracker : public QObject {
Q_OBJECT
public:
    explicit QTracker();
    virtual ~QTracker(){

    }
    QPicProvider* pPicRrovider;
    std::vector<std::string> m_labels;
public Q_SLOTS:
    void tracker(QVariant dataVar);
private:
    void TrackAndObjectOutput(cv::Mat& image_origin, const DS_DetectObjects &detect_objects, const uint64_t &frame_id, const uint32_t &channel_id);
    std::vector<std::map<uint64_t, cv::Mat>> cache_frames_;
    int cache_frame_count_ = 0;
    std::vector<std::shared_ptr<MotFilter>> track_channels_;
};

#endif // QTRACKER_H
