#ifndef QDECODER_H
#define QDECODER_H

#include <QObject>
#include <QThread>
#include <string>
#include "opencv2/videoio.hpp"

class QDecoder : public QThread
{
    Q_OBJECT
public:
    QDecoder(uint id, std::string path);
    ~QDecoder();
    void run();
    cv::Mat image;
    cv::VideoCapture* capture;
    uint stream_id;
    std::string video_path;
};

#endif // QDECODER_H
