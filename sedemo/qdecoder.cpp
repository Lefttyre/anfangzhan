#include "qdecoder.h"
#include <iostream>
#include <GlobalVariable.h>
#include "opencv2/imgcodecs/imgcodecs.hpp"
#include <opencv2/opencv.hpp>


QDecoder::QDecoder(uint id, std::string path)
{
    stream_id = id;
    video_path = path;
}

QDecoder::~QDecoder()
{
    capture->release();
}
void QDecoder::run()
{
    capture = new cv::VideoCapture();
    std::string strStdMediaFile = video_path;

    bool result=capture->open(strStdMediaFile);
    int frames=capture->get(CV_CAP_PROP_FRAME_COUNT);
    unsigned int  intFrameCnt = 0;
    do{
        if (intFrameCnt==frames-1)
        {
            capture->set(CV_CAP_PROP_POS_FRAMES,0);
            intFrameCnt=0;
        }

        if (!capture->read(image)) {
            std::cout << "read the empty frame" << std::endl;
            continue;
        }
        auto objFrameDecoded=std::make_shared<GlobalVariable::FrameDecoded>();
        objFrameDecoded->m_mat = std::make_shared<cv::Mat>(image);
        //*objFrameDecoded->m_mat = image.clone();
        objFrameDecoded->m_intChn = stream_id;
        objFrameDecoded->m_intPos = intFrameCnt;
        uint idx = stream_id % threads;
        GlobalVariable::s_semFilterFrees[idx]->acquire();
        GlobalVariable::s_aryProducedFrameses[idx].push_back(objFrameDecoded);
        GlobalVariable::s_semFilterUseds[idx]->release();
        intFrameCnt++;
     } while (true);
    return;
}
