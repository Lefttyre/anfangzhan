#include "qpostprocess.h"
#include <fstream>

QPostProcess::QPostProcess()
{
 qRegisterMetaType<QVariant>("QVariant");
    x_scale_ratio = 0;
    y_scale_ratio = 0;
    m_detection.batch_images_for_tracker.resize(s_batchSize);
    m_detection.batch_images.resize(s_batchSize);

    std::string label_file = "../ssd/labelmap_voc.txt";

    std::ifstream labels(label_file);
    std::string line;
    while (std::getline(labels, line)) {
        m_labels.push_back(std::string(line));
    }
    labels.close();
}

QPostProcess::~QPostProcess(){

}

void QPostProcess::ImagePostProcess(QVariant dataVar){

    uint batchsize = s_batchSize;
    Inference_Out askData = dataVar.value<Inference_Out>();

    x_scale_ratio = x_scale_ratio ? x_scale_ratio : 1.0*INPUT_W / askData.batch_images[0].cols;
    y_scale_ratio = y_scale_ratio ? y_scale_ratio : 1.0*INPUT_H / askData.batch_images[0].rows;
    std::vector<std::vector<bbox>>vec_objects(batchsize);
    m_detection.objects.clear();
    float *detection_out = askData.inference_out.data();
    auto& imgidx = askData.imgidx;
    auto& batch_images = askData.batch_images;
    for (uint p = 0; p < batchsize; ++p) {
        float* det = detection_out + p * OUTPUT_DETECTION_OUT;

        for (int k=0; k<8; k++){
            if(det[7*k+1] < 0) continue;
            bbox tmp;
            if (det[7*k+2] > 0.46f){
                float xmin = 300 * det[7*k + 3]/x_scale_ratio;
                float ymin = 300 * det[7*k + 4]/y_scale_ratio;
                float xmax = 300 * det[7*k + 5]/x_scale_ratio;
                float ymax = 300 * det[7*k + 6]/y_scale_ratio;
                tmp.channel_num = imgidx[p].m_intChn;
                tmp.cls_id = static_cast<uint>(det[7*k+1]);
                tmp.confidence = det[7*k+2];
                tmp.frame_id = imgidx[p].m_intPos;
                tmp.image_id = static_cast<uint>(det[7*k]);
                tmp.label = m_labels[det[7*k+1] - 1];
                tmp.xmax = xmax;
                tmp.xmin = xmin;
                tmp.ymax = ymax;
                tmp.ymin = ymin;
                vec_objects[p].push_back(tmp);
            }
        }
        cv::Mat framergb;
        cv::cvtColor(batch_images[p], framergb, CV_BGR2RGB);
        batch_images[p] = framergb;
        m_detection.batch_images_for_tracker[p] = framergb.clone();
       // m_detection.batch_images[p] = framergb.clone();
    } //for (uint p = 0; p < batchsize; ++p)
    m_detection.imgidx = imgidx;
    m_detection.batch_images = batch_images;
    m_detection.objects = vec_objects;

    QVariant v;
    v.setValue(m_detection);

    emit(callAfterPostProcess(v));
    return ;
}
