#ifndef QPOSTPROCESS_H
#define QPOSTPROCESS_H

#include <QObject>
#include "GlobalVariable.h"
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

class QPostProcess : public QObject
{
Q_OBJECT
public:
    explicit QPostProcess();
    virtual ~QPostProcess();

signals:
     void callAfterPostProcess(QVariant dataVar);

public Q_SLOTS:
    void ImagePostProcess(QVariant dataVar);

private:
    DetectionObject m_detection;
    float x_scale_ratio;
    float y_scale_ratio;
    std::vector<std::string> m_labels;
};

#endif // QPOSTPROCESS_H
