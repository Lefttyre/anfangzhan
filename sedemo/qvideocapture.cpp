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
#include "qvideocapture.h"
#include <vector>
#include <string>
#include <QTimer>
#include <QThread>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <chrono>
#include <iostream>
#include <functional>
#include "QQuickItem"
#include "QQuickWindow"
#include "opencv2/opencv.hpp"
#include "opencv2/core/core.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui.hpp"
#include "GlobalVariable.h"
#include <QSGSimpleMaterial>
#include "QSGSimpleTextureNode"
#include "opencv2/imgcodecs.hpp"
#include <iostream>
#include <iomanip>

#define LWIN_W 896
#define LWIN_H 1008

std::vector<int> g_vec_fps;
float g_total_fps;

class BigImageWorker {
 public:
  void init() {
    th_ = std::thread(&BigImageWorker::run, this);
  }
  void run() {
      while (running_) {
        std::unique_lock<std::mutex> lock(mux_);
        if (working_) {
            task_();
            working_ = false;
        } else {
            sync_.wait(lock);
        }
        done_.notify_one();
      }
  }
  void work(const std::function<void()> &task) {
    std::unique_lock<std::mutex> lock(mux_);
    working_ = true;
    task_ = task;
    sync_.notify_one();
  }
  void stop() {
    std::unique_lock<std::mutex> lock(mux_);
    running_ = false;
    sync_.notify_one();
    lock.unlock();
    th_.join();
  }
  void sync() {
    std::unique_lock<std::mutex> lock(mux_);
    if (working_)
      done_.wait(lock);
  }
 private:
  std::thread th_;
  std::function<void()> task_;
  bool running_ = true;
  bool working_ = false;
  std::mutex mux_;
  std::condition_variable sync_;
  std::condition_variable done_;
} g_image_worker[channels];  // class BigImageWorker

class TimeCnter {
 public:
    void start() {
        auto now = std::chrono::high_resolution_clock::now();
        last.assign(channels, now);
        g_vec_fps.assign(channels, 0);
        g_total_fps = 0;
        channels_frame_num.assign(channels,0);
    }
    void calc(int id) {
        ++channels_frame_num[id];
        ++frame_num;
        if (channels_frame_num[id] % 10 == 0) {
            auto now = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double, std::milli> diff = now - last[id];
            g_vec_fps[id] = std::ceil(10000/diff.count()+1.5);
            last[id] = now;
	    #if 0
            std::string str = "channel" + std::to_string(id)+ ": " + std::to_string(g_total_fps) + "fps\n";
            std::cout << str;
	    #endif
        }
        if (frame_num % 80 == 0) {
           float fps = 0;
           for (auto it : g_vec_fps) {
               fps += it;
           }
           g_total_fps = fps;
        }
    }

 std::vector<std::chrono::time_point<std::chrono::system_clock>> last;
 uint32_t frame_num;
 std::vector<uint32_t> channels_frame_num;
}fps_cal;  // class TimeCnter

std::vector<std::string> GetVideoList(std::string list_txt) {
    std::vector<std::string> ret;
    std::ifstream ifs(list_txt);
    if (ifs.is_open()) {
        int cnt = channels;
        while(cnt--) {
            std::string path;
            ifs >> path;
            ret.push_back(path);
        }
    }
    return ret;
}

QVideoCapture::QVideoCapture(QQuickItem *parent)
    : QQuickItem(parent)
    , m_file("")
    , m_fps(0)
    , m_linearFilter(true)
    , m_loop(false)
	, qimg(NULL)
    , imgIpl(NULL)
	//, m_thread(0)
{
    qRegisterMetaType<QVariant>("QVariant");
    setFlag(ItemHasContents, true);

    fps_cal.start();

    for (int i = 0; i < channels; ++i) {
        g_image_worker[i].init();
    }
    vec_image_buf_.resize(channels);
    auto files = GetVideoList("filelist");
    //create and start docoders
    for(int i=0; i < channels; i++){
        QDecoder* pDecoder=new QDecoder(i, files[i]);
        Decoders.push_back(pDecoder);
    }

    for(auto& de:Decoders){
        de->start();
    }

    timerthread = new QThread(this);
    timerthread->start();

    m_timer.start(intTimer);
    m_timer.setInterval(400);
    m_timer.moveToThread(timerthread);
    connect(&m_timer, SIGNAL(timeout()), this, SLOT(refreshChannels()));

    delete imgIpl;
    imgIpl = NULL;

    delete qimg;
    qimg = NULL;

    update();

}

qint64 QVideoCapture::timeStamp1;

void QVideoCapture::setFile(const QString &file){
    //实际设定ID 待改正
    delete imgIpl;
    imgIpl = NULL;

    delete qimg;
    qimg = NULL;

    update();
}

QSGNode *QVideoCapture::updatePaintNode(QSGNode *node, QQuickItem::UpdatePaintNodeData * x){
	if (!qimg ||!imgIpl)
	{
		return QQuickItem::updatePaintNode(node, x);
	}
	QSGSimpleTextureNode *texture = static_cast<QSGSimpleTextureNode*>(node);
	if (texture == NULL) {
		texture = new QSGSimpleTextureNode();
	}

	QSGTexture *t = window()->createTextureFromImage(*qimg);


	if (t) {
		QSGTexture *tt = texture->texture();
		if (tt) {
			tt->deleteLater();
		}
        QRectF rect;
        //rect.setRect(boundingRect().width() / 2 - intOrignWidth / 2, boundingRect().height()/2 - intOrignWidth*dblRate/2, intOrignWidth, intOrignWidth*dblRate);
        texture->setRect(boundingRect());
		texture->setTexture(t);
	}

	return texture;
	
}


QVideoCapture::~QVideoCapture(){
    for(auto de: Decoders){
        delete de;
    }
    for (int i = 0; i < channels; ++i) {
        g_image_worker[i].stop();
    }
    vec_image_buf_.clear();
}

void QVideoCapture::setScreenBlack()
{
	if (!qimg)
	{
		return;
	}
    qimg->fill("black");
	if (imgIpl)
	{
		if (imgIpl->imageData)
		{
			cvReleaseImage(&imgIpl);
			delete imgIpl;
			imgIpl = NULL;
		}
	}

    update();
}

void QVideoCapture::receiveFrames(QVariant dataVar){

    DetectionObject askData = dataVar.value<DetectionObject>();
    uint image_numbers = askData.batch_images.size();

    for(uint p=0; p<image_numbers; p++){
        uint channel = askData.imgidx[p].m_intChn;
    	fps_cal.calc(channel);
        emit sig_refresh_fps(g_total_fps);

        // put label and rect
        cv::Mat temp ; //= askData.batch_images[p];
        if(vec_image_buf_[channel].size()>8){

            temp = vec_image_buf_[channel].at(vec_image_buf_[channel].size()-4);
}else{
            temp = askData.batch_images[p];
        }

        for (auto &obj : askData.objects[p]){
            cv::Point2f lt = cv::Point2f(obj.xmin, obj.ymin);
            cv::Point2f rb = cv::Point2f(obj.xmax, obj.ymax);
            auto color = obj.cls_id == 15 ? CV_RGB(0x33, 0xff, 0xcc) : CV_RGB(101, 189, 237);
            cv::rectangle(temp, lt, rb, color, 5);
            std::stringstream ss;
            ss.precision(2);
            ss << obj.confidence;
            std::string str;
            ss >> str;
            str =  obj.label + " " + str;
            auto textsize = cv::getTextSize(str, 1, 2.0, 2, nullptr);
            cv::rectangle(temp, cvRect(obj.xmin, obj.ymax-textsize.height, textsize.width, textsize.height+2), color, -1);
            cv::putText(temp, str, cv::Point(obj.xmin, obj.ymax), 1, 2.0, cv::Scalar(0, 0, 0), 2);
        }

        std::string text = "CHN-" + std::to_string(channel);
        int x = temp.cols / 2;
        int y=  temp.rows ;
        cv::putText(temp, text.c_str(), cv::Point(x, y-20), 1, 5.0, cv::Scalar(255, 0, 0), 3);

        if (vec_image_buf_[channel].size() > 100) {
            vec_image_buf_[channel].front().release();
            vec_image_buf_[channel].pop_front();
        }
        vec_image_buf_[channel].push_back(askData.batch_images[p]);

        if (!imgIpl) {
            imgIpl = cvCreateImageHeader(cvSize(LWIN_W, LWIN_H), 8, 3);
            if (!qimg)
            {
                qimg = new QImage(QSize(LWIN_W, LWIN_H), QImage::Format_RGB888);
                imgIpl->imageData = (char*)qimg->bits();
            }
        }
    }
}

void QVideoCapture::refreshChannels() {
    std::vector<bool> flush;
    flush.assign(channels, false);
    for (int i = 0; i < channels; ++i) {
	if (vec_image_buf_[i].empty())
	    continue;
        //zoom image
        if (i == selected_chn_) {
            cv::Mat detail = vec_image_buf_[i].front();
            emit sig_refresh_detail(detail);
        }
    	int x, y, pix_x, pix_y;
        y = i / 4;
        x = i % 4;
        pix_x = LWIN_W / 4 * x;
        pix_y = LWIN_H / 8 * y;
        std::function<void()> task = [this, pix_x, pix_y, i]() {
            cv::Mat objMat(LWIN_H/8, LWIN_W/4, CV_8UC3);
            if (vec_image_buf_[i].empty()) return;
            auto srcMat = vec_image_buf_[i].front();
            cv::resize(srcMat, objMat, CvSize(LWIN_W/4, LWIN_H/8));
            std::string chan_fps = "FPS:" + std::to_string(g_vec_fps[i]);
            cv::putText(objMat,chan_fps,cv::Point(5, 18), 1, 1.5, cv::Scalar(0, 255, 0), 2);
            vec_image_buf_[i].front().release();
            vec_image_buf_[i].pop_front();
            cv::Mat dest = cv::cvarrToMat(imgIpl);
            objMat.copyTo(dest(cvRect(pix_x, pix_y, LWIN_W/4, LWIN_H/8)));
            if (selected_chn_ == i)
                 cv::rectangle(dest, cvRect(pix_x, pix_y, LWIN_W/4, LWIN_H/8), cv::Scalar(255, 0, 0), 2);
        };
        g_image_worker[i].work(task);
	flush[i] = true;
    }
    for (int i = 0; i < channels; ++i) {
        if (flush[i]) g_image_worker[i].sync();
    }
    update();
}

void QVideoCapture::selectChns(int x, int y) {
    int select_chn = static_cast<int>(8.0 * y / height()) * 4 + static_cast<int>(x / width() * 4);
    if (select_chn > 31) select_chn = 31;
    qDebug() << "selected: " << select_chn;
    selected_chn_ = select_chn;
}
