#ifndef TENSORFLOWTHREAD_H
#define TENSORFLOWTHREAD_H

#include <QObject>
#include <QImage>
#include <QRectF>
#include <QThread>
#include <QMap>
#include "tensorflow.h"

class WorkerTF: public QObject
{
    Q_OBJECT

        QImage imgTF;
        TensorFlow *tf;
        QString source;
        QString destination;
        bool videoMode;
        QMap<QString,bool> activeLabels;
        bool showAim;
        bool showInfTime;

    public:
        void setImgTF(const QImage &value);
        void setTf(TensorFlow *value);
        void setVideoInfo(QString s, QString d, bool sAim, bool sInfTime, QMap<QString,bool> aLabels);

    public slots:
        void work();

    private:
        QImage processImage(QImage img);

    signals:
        void results(int network, QStringList captions, QList<double> confidences, QList<QRectF> boxes,  QList<QImage> masks, int infTime);
        void finished();
        void numFrame(int n);
        void numFrames(int n);
};

class TensorFlowThread: public QObject
{
    Q_OBJECT

    public:
        TensorFlowThread();
        void setTf(TensorFlow *value);

        void run(QImage imgTF);
        void run(QString source, QString destination, bool showAim, bool showInfTime, QMap<QString,bool> activeLabels);

    signals:
        void results(int network, QStringList captions, QList<double> confidences, QList<QRectF> boxes, QList<QImage> masks, int infTime);
        void numFrame(int n);
        void numFrames(int n);

    public slots:
        void propagateResults(int network, QStringList captions, QList<double> confidences, QList<QRectF> boxes, QList<QImage> masks, int infTime);
        void propagateNumFrame(int n);
        void propagateNumFrames(int n);

    private:
        QThread threadTF;
        WorkerTF worker;
};

#endif // TENSORFLOWTHREAD_H
