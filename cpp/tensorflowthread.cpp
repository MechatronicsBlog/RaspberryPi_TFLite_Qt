#include "tensorflowthread.h"
#include "auxutils.h"

void WorkerTF::setTf(TensorFlow *value)
{
    tf = value;
}

void WorkerTF::setImgTF(const QImage &value)
{
    imgTF     = value;
    videoMode = false;
}

void WorkerTF::setVideoInfo(QString s, QString d, bool sAim, bool sInfTime, QMap<QString,bool> aLabels)
{
    source = s;
    destination = d;
    showAim = sAim;
    showInfTime = sInfTime;
    activeLabels = aLabels;
    videoMode = true;
}

void WorkerTF::work()
{   
    if (!videoMode)
    {
        tf->run(imgTF);
        emit finished();
        emit results(tf->getKindNetwork(),tf->getResults(),tf->getConfidence(),tf->getBoxes(),tf->getMasks(),tf->getInferenceTime());
    }
}

QImage WorkerTF::processImage(QImage img)
{
    // Data
    double        minConf       = tf->getThreshold();
    int           inferenceTime = tf->getInferenceTime();
    QStringList   results       = tf->getResults();
    QList<double> confidence    = tf->getConfidence();
    QList<QRectF> boxes         = tf->getBoxes();
    QList<QImage> masks         = tf->getMasks();

    // Draw masks on image
    if (!masks.isEmpty())
        img = AuxUtils::drawMasks(img,img.rect(),results,confidence,boxes,masks,minConf,activeLabels);

    // Draw boxes on image
    img = AuxUtils::drawBoxes(img,img.rect(),results,confidence,boxes,minConf,activeLabels,true);

    // Show inference time
    if (showInfTime)
    {
        QString text = QString::number(inferenceTime) + " ms";
        img = AuxUtils::drawText(img,img.rect(),text);
    }

    return img;
}

TensorFlowThread::TensorFlowThread()
{
    threadTF.setObjectName("TensorFlow thread");
    worker.moveToThread(&threadTF);
    QObject::connect(&worker, SIGNAL(results(int,  QStringList, QList<double>, QList<QRectF>, QList<QImage>, int)), this, SLOT(propagateResults(int, QStringList, QList<double>, QList<QRectF>,  QList<QImage>, int)));
    QObject::connect(&worker, SIGNAL(numFrame(int)), this, SLOT(propagateNumFrame(int)));
    QObject::connect(&worker, SIGNAL(numFrames(int)), this, SLOT(propagateNumFrames(int)));
    QObject::connect(&worker, SIGNAL(finished()),  &threadTF, SLOT(quit()));
    QObject::connect(&threadTF, SIGNAL(started()), &worker,   SLOT(work()));
}

void TensorFlowThread::setTf(TensorFlow *value)
{
    worker.setTf(value);
}

void TensorFlowThread::run(QImage imgTF)
{    
    worker.setImgTF(imgTF);
    threadTF.start();
}

void TensorFlowThread::run(QString source, QString destination, bool showAim, bool showInfTime, QMap<QString,bool> activeLabels)
{
    worker.setVideoInfo(source,destination,showAim,showInfTime,activeLabels);
    threadTF.start();
}

void TensorFlowThread::propagateResults(int network, QStringList captions, QList<double> confidences, QList<QRectF> boxes, QList<QImage> masks, int infTime)
{
    emit results(network,captions,confidences,boxes,masks,infTime);
}

void TensorFlowThread::propagateNumFrame(int n)
{
    emit numFrame(n);
}

void TensorFlowThread::propagateNumFrames(int n)
{
    emit numFrames(n);
}
