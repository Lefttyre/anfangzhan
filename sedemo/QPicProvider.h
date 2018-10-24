#pragma once
#include <QQuickImageProvider>
#include "opencv2/core/types.hpp"
#include "opencv2/core/core.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"

class QPicProvider : public QObject, public QQuickImageProvider
{
Q_OBJECT
public:

    QPicProvider();
	~QPicProvider(){

	}

	int  isExist(QString id)
	{
		for (int i = 0; i < m_aryMat.size(); i++)
		{
			if (m_aryMat[i].m_strComplexID == id)
			{
				return i;
			}
		}
		return -1;
	}
    QImage requestImage(const QString &id, QSize *size, const QSize& requestedSize);

    void addMat(cv::Mat objMat,int intEntityID,int intEntityType);

    QImage img;
    struct MatData
    {
        int m_intEntityID;
        cv::Mat m_mat;
        int m_intEntityType;
        QString m_strComplexID;
    };
public Q_SLOTS:
    void RefreshDetail(cv::Mat img);

private:
    QImage  detail_;
	QList<MatData> m_aryMat;
    int intMaxSize = 8;
};
