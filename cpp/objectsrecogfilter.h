#ifndef OBJECTSRECOGFILTER_H
#define OBJECTSRECOGFILTER_H

#include <QVideoFilterRunnable>
#include <QElapsedTimer>
#include <QThread>
#include <QMutex>
#include <QTimer>

#include "tensorflow.h"
#include "tensorflowthread.h"

// https://stackoverflow.com/questions/43106069/how-to-convert-qvideoframe-with-yuv-data-to-qvideoframe-with-rgba32-data-in

class ObjectsRecogFilterRunable;

class ObjectsRecogFilter : public QAbstractVideoFilter
{
    Q_OBJECT

    Q_PROPERTY(double cameraOrientation READ getCameraOrientation WRITE setCameraOrientation)
    Q_PROPERTY(double videoOrientation READ getVideoOrientation WRITE setVideoOrientation)
    Q_PROPERTY(double minConfidence READ getMinConfidence WRITE setMinConfidence)
    Q_PROPERTY(QSize contentSize READ getContentSize WRITE setContentSize)
    Q_PROPERTY(bool acceleration READ getAcceleration WRITE setAcceleration)
    Q_PROPERTY(int nThreads READ getNThreads WRITE setNThreads)
    Q_PROPERTY(bool showInfTime READ getShowInfTime WRITE setShowInfTime)
    Q_PROPERTY(double angle READ getAngle NOTIFY angleChanged)

public slots:
    void init(int imgHeight, int imgWidth);
    void initInput(int imgHeight, int imgWidth);
    QMap<QString,bool> getActiveLabels();
    bool getActiveLabel(QString key);
    void setActiveLabel(QString key, bool value);

public:
        ObjectsRecogFilter();
        QVideoFilterRunnable *createFilterRunnable();
        void setCameraOrientation(double o);
        void setVideoOrientation(double o);
        double getCameraOrientation();
        double getVideoOrientation();
        double getMinConfidence() const;
        void setMinConfidence(double value);
        bool getRunning();
        void releaseRunning();
        QSize getContentSize() const;
        void setContentSize(const QSize &value);
        bool getAcceleration() const;
        void setAcceleration(bool value);
        int getNThreads() const;
        void setNThreads(int value);
        bool getShowInfTime() const;
        void setShowInfTime(bool value);
        void setFrameSize(QSize size);
        void setFrameRate(int fps);
        double getAngle() const;
        void setAngle(const double value);
        double getImgHeight();
        double getImgWidth();
        bool getInitialized() const;
        void setInitialized(bool value);

private:
        ObjectsRecogFilterRunable *rfr;
        TensorFlow tf;
        TensorFlowThread tft;
        double camOrientation;
        double vidOrientation;
        double minConf;
        bool   acc;
        int    nThr;
        bool   infTime;
        QMutex mutex;
        bool   running;
        QSize  videoSize;
        QMap<QString,bool> activeLabels;
        void setRunning(bool val);
        double ang;
        bool initialized;

    signals:
        void runTensorFlow(QImage imgTF);
        void focusDataChanged();
        void focusedChanged();
        void angleChanged();
        void errorXChanged();
        void errorYChanged();

    private slots:
        void TensorFlowExecution(QImage imgTF);
        void processResults(int network, QStringList res, QList<double> conf, QList<QRectF> boxes, QList<QImage> masks, int inftime);
};

class ObjectsRecogFilterRunable : public QVideoFilterRunnable
{
    public:
        ObjectsRecogFilterRunable(ObjectsRecogFilter *filter, QStringList res);
        QVideoFrame run(QVideoFrame *input, const QVideoSurfaceFormat &surfaceFormat, RunFlags flags);
        void setResults(int net, QStringList res, QList<double> conf, QList<QRectF> box, QList<QImage> mask, int inftime);

    private:
        ObjectsRecogFilter *m_filter;
        int           network;
        QStringList   results;
        QList<double> confidence;
        QList<QRectF> boxes;
        QList<QImage> masks;
        int           inferenceTime;
        QElapsedTimer timer;

};

#endif // OBJECTSRECOGFILTER_H
