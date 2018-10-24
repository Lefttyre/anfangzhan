/****************************************************************************
**
** Copyright (C) 2014-2016 Dinu SV.
** (contact: mail@dinusv.com)
** This file is part of Live CV Application.
**
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
****************************************************************************/

#ifndef QVIDEOCAPTURE_H
#define QVIDEOCAPTURE_H
#include <queue>
#include <deque>
#include "GlobalVariable.h"
#include "QQuickItem"
#include "qdecoder.h"

//class QVideoCaptureThread;
class QVideoCapture : public QQuickItem{

    Q_OBJECT

//    Q_PROPERTY(QString file         READ file         WRITE setFile         NOTIFY fileChanged)
//    Q_PROPERTY(qreal   fps          READ fps          WRITE setFps          NOTIFY fpsChanged)

public:
    explicit QVideoCapture(QQuickItem *parent = 0);
    virtual ~QVideoCapture();

	const QString& file() const;
    void setFile(const QString& file);
    Q_INVOKABLE void selectChns(int x, int y);

signals:
    void sig_refresh_detail(cv::Mat img);
    void sig_refresh_fps(int fps);

public slots:
    void setScreenBlack();
    void receiveFrames(QVariant dataVar);
    void refreshChannels();
protected:
    virtual QSGNode *updatePaintNode(QSGNode *node, UpdatePaintNodeData *nodeData);

private:
    void initializeMatSize();

    QVideoCapture(const QVideoCapture& other);
    QVideoCapture& operator= (const QVideoCapture& other);

    QString m_file;
	int m_intTaskID;
    qreal   m_fps;
    //QMat*   m_output=NULL;
    bool    m_linearFilter;
    bool    m_loop;
	IplImage* imgIpl;
	QImage* qimg;
   // QVideoCaptureThread* m_workerThread;
	QList<QThread*> m_aryFrameProducer;
	QList<QString> m_aryFileList;
	bool m_bolPaused;
	int m_intTotalFrame;
	int m_intCurFrame;
	int m_intRate;
	static int m_date;
	QTimer m_timer;
    QThread* timerthread;
	static qint64 timeStamp1;
	//QVideoCaptureThread* m_thread;
	int intTimer = 35;
	QString totalFrameTime;
	QString currentFrameTime;
	int currentTaskType;
    std::vector<QDecoder*> Decoders;
    std::vector<std::deque<cv::Mat>> vec_image_buf_;

    uint selected_chn_ = 0;
};

inline const QString &QVideoCapture::file() const{
    return m_file;
}

#endif // QVIDEOCAPTURE_H
