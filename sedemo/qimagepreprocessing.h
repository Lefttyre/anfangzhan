#pragma once
#ifndef QIMAGEPREPROCESSING_H
#define QIMAGEPREPROCESSING_H

#include <QObject>
#include <QThread>
#include "GlobalVariable.h"
#include <opencv2/dnn.hpp>
class QImagePreProcessing : public QThread
{
    Q_OBJECT
public:
    QImagePreProcessing(uint idx=0);
    ~QImagePreProcessing(){}
    virtual void run();
    void imgprepro(std::shared_ptr<GlobalVariable::FrameDecoded>);
private:
    uint id;
    GlobalVariable::FramePreProcessed m_PreProcessFrames;
};

#endif // QIMAGEPREPROCESSING_H
