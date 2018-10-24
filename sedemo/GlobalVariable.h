#pragma once
#include "QtCore"
#include "QObject"
#include "opencv2/core/core.hpp"
#include "QVector"
#include "QMutex"
#include <vector>
#include <QMetaType>
#include <QVariant>
#include <memory>
#ifndef CAMBRICON_SDK_API
#define CAMBRICON_SDK_API __declspec(dllexport)
#else
#define CAMBRICON_SDK_API __declspec(dllimport)
#endif
#define USE_CUDNN
typedef  struct {
    uint m_intChn;
    uint m_intPos;
} _imageidx;


typedef struct {
    uint channel_num;
    uint frame_id;
    uint image_id; //the index in one batch
    float confidence;
    float xmin;
    float ymin;
    float xmax;
    float ymax;
    uint cls_id;
    std::string label;
} bbox;

class DetectionObject {
public:
    DetectionObject(){}
    ~DetectionObject(){}
    DetectionObject(const DetectionObject &other){
        for(int i=0; i<other.batch_images.size();i++){
            batch_images.push_back(other.batch_images[i].clone());
            batch_images_for_tracker.push_back(other.batch_images[i].clone());
        }
       objects = other.objects;
       imgidx = other.imgidx;
    }
    std::vector<std::vector<bbox>> objects;
    std::vector<cv::Mat> batch_images;
    std::vector<cv::Mat> batch_images_for_tracker;
    std::vector<_imageidx>  imgidx;
};
Q_DECLARE_METATYPE(DetectionObject)

class Inference_Out{
public:
    Inference_Out(){}
    ~Inference_Out(){}
    Inference_Out(const Inference_Out &other){
        for(int i=0; i<other.batch_images.size();i++){
            batch_images.push_back(other.batch_images[i].clone());
        }
        imgidx = other.imgidx;
        inference_out = other.inference_out;
    }
    std::vector<cv::Mat> batch_images;
    std::vector<_imageidx> imgidx;
    std::vector<float> inference_out;
} ;
Q_DECLARE_METATYPE(Inference_Out);

static const int INPUT_C = 3;
static const int INPUT_H = 300;
static const int INPUT_W = 300;
static const uint channels = 32;
static const uint s_batchSize = 1;
static const int threads = 32;
static const int outputsize=1400;
static const int OUTPUT_DETECTION_OUT = outputsize;
class GlobalVariable :public QObject
{
    Q_OBJECT

public:
	GlobalVariable();
	~GlobalVariable();
    struct FrameDecoded
	{
        ~FrameDecoded()
		{
			//delete m_data;
		}
        std::shared_ptr<cv::Mat> m_mat;
        uint m_intChn;
        uint m_intPos;
	};	
    struct FramePreProcessed{
        std::vector<cv::Mat> batch_images;
        std::vector<_imageidx> imgidx;
        cv::Mat blobimages;
    };

    static std::vector<std::shared_ptr<QSemaphore>> s_semFilterFrees;
    static std::vector<std::shared_ptr<QSemaphore>> s_semFilterUseds;
    static std::vector<QList<std::shared_ptr<GlobalVariable::FrameDecoded>>> s_aryProducedFrameses;

    static std::vector<std::shared_ptr<QSemaphore>> s_semPreProFrees;
    static std::vector<std::shared_ptr<QSemaphore>> s_semPreProUseds;
    static std::vector<QList<GlobalVariable::FramePreProcessed>> s_aryPreProcessedFramesess;
};

