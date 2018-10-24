#include <memory>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include "QQmlEngine"
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <qqml.h>
#include "qvideocapture.h"
#include "qdecoder.h"
#include <QVector>
#include "qinference.h"
#include "QPicProvider.h"
#include "qtracker.h"
#include "qimagepreprocessing.h"
#include "qpostprocess.h"
std::shared_ptr<QThread> g_pthread;
std::vector<std::shared_ptr<QThread>> g_postprocessed;
//std::shared_ptr<QThread> g_tracker_pthread;
int main(int argc, char *argv[])
{
    for(int i=0; i<threads;i++){
        GlobalVariable::s_semFilterFrees.push_back(std::make_shared<QSemaphore>(1));
        GlobalVariable::s_semFilterUseds.push_back(std::make_shared<QSemaphore>(0));
        GlobalVariable::s_semPreProFrees.push_back(std::make_shared<QSemaphore>(1));
        GlobalVariable::s_semPreProUseds.push_back(std::make_shared<QSemaphore>(0));
    }

    #if (QT_VERSION >= QT_VERSION_CHECK(5, 6, 0))
    	QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    #endif

    QGuiApplication app(argc, argv);
    qmlRegisterType<QVideoCapture>("CambriconControl", 1, 0, "VideoCapture");

    g_pthread = std::make_shared<QThread>();
   // std::shared_ptr<QThread> g_tracker_pthread = std::make_shared<QThread>();
    QThread* tracker_pthread = new QThread();
    QPicProvider* pPicRrovider=new QPicProvider();

    QQmlApplicationEngine engine;
    engine.addImageProvider("detailimg", pPicRrovider);

    engine.load(QUrl(QStringLiteral("qrc:/main.qml")));
    if (engine.rootObjects().isEmpty())
        return -1;
    std::vector<std::shared_ptr<QImagePreProcessing>> pobjImagePres;
    for(uint i=0; i<threads; i++){
        pobjImagePres.push_back(std::make_shared<QImagePreProcessing>(i));
        pobjImagePres.at(i)->start();
    }
    std::vector<std::shared_ptr<QInference>> pobjInferences;
    for(uint i=0; i<threads; i++ ){
        pobjInferences.push_back(std::make_shared<QInference>(i));
        pobjInferences.at(i)->init();
        pobjInferences.at(i)->start();
    }

    std::vector<std::shared_ptr<QPostProcess>> pobjPostProcesses;
    for(uint i=0; i<threads;i++){
        g_postprocessed.push_back(std::make_shared<QThread>());
        pobjPostProcesses.push_back(std::make_shared<QPostProcess>());
        pobjPostProcesses[i]->moveToThread(g_postprocessed[i].get());
        g_postprocessed[i]->start();
    }
    QList<QObject*> aryRoots = engine.rootObjects();
    QVideoCapture* objVideoView = nullptr;
    QTracker* pobjTracker = new QTracker();
    pobjTracker->pPicRrovider = pPicRrovider;

    if (aryRoots.size() > 0)
    {
        QObject* objRoot = aryRoots[0];
        for(int i=0;i<1;i++)
        {
            QObject* objTask = objRoot->findChild<QObject*>("avCompoent0");
            QObject* objItem1 = objRoot->findChild<QObject*>("videoView0");
            objVideoView = qobject_cast<QVideoCapture*>(objItem1);
            engine.rootContext()->setContextProperty("fps_updater", objVideoView);

            for(uint i = 0; i<threads; i++){
                QObject::connect(pobjInferences[i].get(), SIGNAL(callAfterInference(QVariant)),pobjPostProcesses[i].get(),SLOT(ImagePostProcess(QVariant)));
            }

            for(uint i = 0; i<threads; i++){
                QObject::connect(pobjPostProcesses[i].get(), SIGNAL(callAfterPostProcess(QVariant)),pobjTracker,SLOT(tracker(QVariant)));
                QObject::connect(pobjPostProcesses[i].get(), SIGNAL(callAfterPostProcess(QVariant)),objVideoView,SLOT(receiveFrames(QVariant)));
            }

            QObject::connect(objVideoView, SIGNAL(sig_refresh_detail(cv::Mat)), pPicRrovider, SLOT(RefreshDetail(cv::Mat)));
            //g_pthread->start();
        }
        //objVideoView->moveToThread(g_pthread.get()); //because has parent, it can't move succefully.
        pobjTracker->moveToThread(tracker_pthread);
        tracker_pthread->start();
    }
    return app.exec();
}
