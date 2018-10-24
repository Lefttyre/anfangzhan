QT += quick widgets
CONFIG += c++11
INCLUDEPATH += /usr/local/include/
INCLUDEPATH +="."
# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    qvideocapture.cpp \
    QPicProvider.cpp \
    GlobalVariable.cpp \
    qdecoder.cpp \
    qinference.cpp \
    main.cpp \
    mot_filter.cpp \
    qtracker.cpp \
    qimagepreprocessing.cpp \
    qpostprocess.cpp

RESOURCES += qml.qrc

# Additional import path used to resolve QML modules in Qt Creator's code model
QML_IMPORT_PATH =

# Additional import path used to resolve QML modules just for Qt Quick Designer
QML_DESIGNER_IMPORT_PATH =

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

HEADERS += \
    qvideocapture.h \
    GlobalVariable.h \
    qdecoder.h \
    qinference.h \
    cudaUtility.h \
    mot_filter.h \
    dependencies/include/deepsort/deepsort.h \
    QPicProvider.h \
    qtracker.h \
    qimagepreprocessing.h \
    qpostprocess.h


unix:!macx: LIBS += -L/usr/local/lib/ -lstdc++fs -lopencv_core

unix:!macx: LIBS += -L/usr/local/lib/ -lopencv_imgcodecs

INCLUDEPATH += /usr/local/include
DEPENDPATH += /usr/local/include

unix:!macx: LIBS += -L/usr/local/lib/ -lopencv_videoio -lopencv_imgproc -lopencv_highgui -lopencv_dnn

unix:!macx: LIBS += -L/usr/local/cuda-9.2/lib64/ -lcudart
unix:!macx: LIBS += -L/usr/local/cuda-9.2/lib64/ -lcudnn
unix:!macx: LIBS += -L/usr/local/cuda-9.2/lib64/ -lcublas

INCLUDEPATH += /usr/local/cuda-9.2/include
DEPENDPATH += /usr/local/cuda-9.2/include

unix:!macx: LIBS += -L/home/${USER}/TensorRT-4.0.1.6/lib/ -lnvparsers
#LIBS +=-l/home/${USER}/TensorRT-4.0.1.6/lib/libnvparsers.so
unix:!macx: LIBS += -L/home/${USER}/TensorRT-4.0.1.6/lib/ -lnvinfer
unix:!macx: LIBS += -L/home/${USER}/TensorRT-4.0.1.6/lib/ -lnvinfer_plugin
unix:!macx: LIBS += -L$$PWD/dependencies/lib -ldeepsort

INCLUDEPATH += /home/${USER}/TensorRT-4.0.1.6/include
DEPENDPATH += /home/${USER}/TensorRT-4.0.1.6/include
INCLUDEPATH += /home/${USER}/TensorRT-4.0.1.6/samples/common
INCLUDEPATH += $$PWD/dependencies/include

DISTFILES += \
    kernel.cu

CUDA_SOURCES +=kernel.cu
CUDA_SDK="/usr/local/cuda-9.2"
CUDA_DIR="/usr/local/cuda-9.2"
SYSTEM_NAME=linux
SYSTEM_TYPE=64
CUDA_ARCH=sm_50
NVCC_OPTIONS= --use_fast_math

INCLUDEPATH +=$$CUDA_DIR/include
QMAKE_LIBDIR +=$$CUDA_DIR/lib64/

CUDA_OBJECTS_DIR= ./

CUDA_LIBS=cudart cudnn cublas cufft
CUDA_INC=$$join(INCLUDEPATH,'" -I"','-I"','"')
NVCC_LIBS = $$join(CUDA_LIBS,' -l','-l', '')

CONFIG(debug, debug|release) {
    cuda_d.input = CUDA_SOURCES
    cuda_d.output = $$CUDA_OBJECTS_DIR/${QMAKE_FILE_BASE}_cuda.o
    cuda_d.commands = $$CUDA_DIR/bin/nvcc -D_DEBUG $$NVCC_OPTIONS $$CUDA_INC $$NVCC_LIBS --machine $$SYSTEM_TYPE -arch=$$CUDA_ARCH -c -o ${QMAKE_FILE_OUT} ${QMAKE_FILE_NAME}
    cuda_d.dependency_type = TYPE_C
    QMAKE_EXTRA_COMPILERS += cuda_d
}
else{
    cuda.input = CUDA_SOURCES
    cuda.output = $$CUDA_OBJECTS_DIR/${QMAKE_FILE_BASE}_cuda.o
    cuda.commands = $$CUDA_DIR/bin/nvcc $$NVCC_OPTIONS $$CUDA_INC $$NVCC_LIBS --machine $$SYSTEM_TYPE -arch=$$CUDA_ARCH -O3 -c -o ${QMAKE_FILE_OUT} ${QMAKE_FILE_NAME}
    cuda.dependency_type = TYPE_C
    QMAKE_EXTRA_COMPILERS += cuda
}
