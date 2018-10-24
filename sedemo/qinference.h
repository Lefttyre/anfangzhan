#pragma once
#ifndef QINFERENCE_H
#define QINFERENCE_H

#include <QThread>
#include "GlobalVariable.h"
#include "NvInfer.h"
#include "NvCaffeParser.h"
#include "NvInferPlugin.h"

class QInference : public QThread
{
    Q_OBJECT
public:
    QInference(uint idx = 0, QObject *parent=NULL);
    ~QInference();
    virtual void run();
    void detectFrame(GlobalVariable::FramePreProcessed);
    void init();
    void uninit();
    void doInference(nvinfer1::IExecutionContext &m_iContext, float *inputData, float *detection_out, int batchSize);
signals:
    void callAfterInference(QVariant dataVar);
private:
    nvinfer1::IRuntime* m_iRuntime;
    nvinfer1::ICudaEngine* m_iEngine;
    nvinfer1::IExecutionContext *m_iContext;
    std::vector<void*> m_Bindings;
    cudaStream_t m_CudaStream;
    Inference_Out m_Inference_out;
    float* detection_out;
    uint id;
    int inputIndex0;
    int detection_outIndex0;
};

#endif // QINFERENCE_H
