#include "QPicProvider.h"
#include <QReadWriteLock>
static QReadWriteLock lock;
QPicProvider::QPicProvider() : QQuickImageProvider(QQuickImageProvider::Image){

}

void QPicProvider::RefreshDetail(cv::Mat img) {
    QImage t(reinterpret_cast<uchar*>(img.data), img.cols, img.rows, img.step, QImage::Format_RGB888);
    lock.lockForWrite();
    detail_ = t.copy();
    lock.unlock();
}


void QPicProvider::addMat(cv::Mat objMat,int intEntityID,int intEntityType)
{
    MatData newMatData;
    newMatData.m_intEntityID = intEntityID;
    newMatData.m_intEntityType = intEntityType;
    newMatData.m_mat = objMat;
    newMatData.m_strComplexID = QString::number(intEntityType) + "_" + QString::number(intEntityID);
    lock.lockForWrite();
    if (m_aryMat.size() >= intMaxSize)
    {
        MatData objMatData = m_aryMat.takeLast();
        objMatData.m_mat.release();
        m_aryMat.push_front(newMatData);
    }
    else
    {
        m_aryMat.push_front(newMatData);
    }
    lock.unlock();
}

QImage QPicProvider::requestImage(const QString &id, QSize *size, const QSize& requestedSize){

    if (id == "detail"){
        lock.lockForRead();
        QImage img = detail_;
        lock.unlock();
        return img;
    }else{
        int intId = id.toInt();
        if(intId < m_aryMat.size()){
             lock.lockForRead();
             cv::Mat* matTmp = &m_aryMat[intId].m_mat;
             const uchar* imgData = (const uchar*)matTmp->data;
             img = QImage(imgData, matTmp->cols, matTmp->rows, matTmp->step, QImage::Format_RGB888);
             lock.unlock();
             return img;
        }else{
             return   QImage();
        }
    }

/*
    int intId = isExist(id);
    if (intId != -1)
    {
        cv::Mat* matTmp = &m_aryMat[intId].m_mat;
        const uchar* imgData = (const uchar*)matTmp->data;
        img = QImage(imgData, matTmp->cols, matTmp->rows, matTmp->step, QImage::Format_RGB888);
        return img;
    }
    else
    {
        QList<QString> aryList = id.split("_");
        if (aryList.size() == 2)
        {
            //EntityType
            int intEntityType = aryList[0].toInt();
            int intEntityId = aryList[1].toInt();
            CvRect imgRect;
            cv::Mat* objMat = new cv::Mat();
            char pPath[400];
            *objMat = cv::imread(pPath);

            if (!objMat->data)
            {
                return QImage();
            }
            double scale = 2.5;
            cv::Size dsize = cv::Size(objMat->cols*scale, objMat->rows*scale);
            cv::Mat* scaleImg = new cv::Mat(dsize, CV_32S);
            cv::resize(*objMat, *scaleImg, dsize);
            cv::Mat *cutImg = new cv::Mat();
            (*scaleImg)(imgRect).copyTo(*cutImg);
            cv::cvtColor(*cutImg, *cutImg, CV_BGR2RGB);
            addMat(*cutImg, intEntityId, intEntityType);

            const uchar* imgData = (const uchar*)cutImg->data;
            img = QImage(imgData, cutImg->cols, cutImg->rows, cutImg->step, QImage::Format_RGB888);
            objMat->release();
            return img;
        }
        else
        {
            return QImage();
        }
    }
*/
}
