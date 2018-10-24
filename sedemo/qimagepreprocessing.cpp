#include "qimagepreprocessing.h"
static float pixelMean[3]{ 104.0f,117.0f,123.0f }; // also in BGR order

QImagePreProcessing::QImagePreProcessing(uint idx):id(idx)
{

}

void QImagePreProcessing::run(){
    while(true){
        GlobalVariable::s_semFilterUseds[id]->acquire();
        if(GlobalVariable::s_aryProducedFrameses[id].empty()){
            GlobalVariable::s_semFilterFrees[id]->release();
            continue;
        }

        auto objFrameDecoded = GlobalVariable::s_aryProducedFrameses[id].takeFirst();
        m_PreProcessFrames.batch_images.push_back(*(objFrameDecoded->m_mat));
        m_PreProcessFrames.imgidx.push_back({objFrameDecoded->m_intChn,objFrameDecoded->m_intPos});
        if(m_PreProcessFrames.batch_images.size() < s_batchSize){
            GlobalVariable::s_semFilterFrees[id]->release();
            continue;
        }
        imgprepro(objFrameDecoded);
        GlobalVariable::s_semFilterFrees[id]->release();
    }
}

void QImagePreProcessing::imgprepro(std::shared_ptr<GlobalVariable::FrameDecoded>){
    m_PreProcessFrames.blobimages = cv::dnn::blobFromImages(m_PreProcessFrames.batch_images, 1.0, cv::Size(INPUT_H,INPUT_W), cv::Scalar(pixelMean[0],pixelMean[1],pixelMean[2]), false, false);
    GlobalVariable::s_semPreProFrees[id]->acquire();
    GlobalVariable::s_aryPreProcessedFramesess[id].push_back(m_PreProcessFrames);
    GlobalVariable::s_semPreProUseds[id]->release();
    m_PreProcessFrames.batch_images.clear();
    m_PreProcessFrames.imgidx.clear();
}
