#include <objectsrecogfilter.h>
#include <QStandardPaths>
#include <QPainter>
#include <QDebug>
#include <QThread>
#include <QMutexLocker>

#include "auxutils.h"
#include "private/qvideoframe_p.h"

// WARNING: same TensorFlow initialization repeated in ObjectRecogFilter and TensorFlowQML constructors
ObjectsRecogFilter::ObjectsRecogFilter()
{
    connect(this, SIGNAL(runTensorFlow(QImage)), this, SLOT(TensorFlowExecution(QImage)));
    connect(&tft,SIGNAL(results(int, QStringList, QList<double>, QList<QRectF>, QList<QImage>, int)),this,SLOT(processResults(int, QStringList, QList<double>, QList<QRectF>, QList<QImage>, int)));

    tf.setFilename(AuxUtils::getDefaultModelFilename());
    tf.setLabelsFilename(AuxUtils::getDefaultLabelsFilename());
    tf.setAccelaration(true);
    tf.setNumThreads(QThread::idealThreadCount());

    releaseRunning();
    initialized = false;
}

void ObjectsRecogFilter::init(int imgHeight, int imgWidth)
{
    initialized = tf.init(imgHeight,imgWidth);
    tft.setTf(&tf);
}

void ObjectsRecogFilter::initInput(int imgHeight, int imgWidth)
{
    tf.initInput(imgHeight,imgWidth);
}

void ObjectsRecogFilter::TensorFlowExecution(QImage imgTF)
{
    tf.setAccelaration(getAcceleration());
    tf.setNumThreads(getNThreads());
    tft.run(imgTF);
}

void ObjectsRecogFilter::processResults(int network, QStringList res, QList<double> conf, QList<QRectF> boxes, QList<QImage> masks, int inftime)
{
    rfr->setResults(network,res,conf,boxes,masks,inftime);
    releaseRunning();
}

void ObjectsRecogFilter::setCameraOrientation(double o)
{
    camOrientation = o;
}

void ObjectsRecogFilter::setVideoOrientation(double o)
{
    vidOrientation = o;
}

double ObjectsRecogFilter::getCameraOrientation()
{
    return camOrientation;
}

double ObjectsRecogFilter::getVideoOrientation()
{
    return vidOrientation;
}

bool ObjectsRecogFilter::getRunning()
{
    QMutexLocker locker(&mutex);

    bool val = running;
    if (!val) setRunning(true);

    return !val;
}

void ObjectsRecogFilter::setRunning(bool val)
{
    running = val;
}

bool ObjectsRecogFilter::getInitialized() const
{
    return initialized;
}

void ObjectsRecogFilter::setInitialized(bool value)
{
    initialized = value;
}

void ObjectsRecogFilter::releaseRunning()
{
    QMutexLocker locker(&mutex);

    setRunning(false);
}

QSize ObjectsRecogFilter::getContentSize() const
{
    return videoSize;
}

void ObjectsRecogFilter::setContentSize(const QSize &value)
{
    videoSize = value;
}

bool ObjectsRecogFilter::getAcceleration() const
{
    return acc;
}

void ObjectsRecogFilter::setAcceleration(bool value)
{
    acc = value;
}

int ObjectsRecogFilter::getNThreads() const
{
    return nThr;
}

void ObjectsRecogFilter::setNThreads(int value)
{
    nThr = value;
}

bool ObjectsRecogFilter::getShowInfTime() const
{
    return infTime;
}

void ObjectsRecogFilter::setShowInfTime(bool value)
{
    infTime = value;
}

double ObjectsRecogFilter::getMinConfidence() const
{
    return minConf;
}

void ObjectsRecogFilter::setMinConfidence(double value)
{
    minConf = value;
    tf.setThreshold(minConf);
}

ObjectsRecogFilterRunable::ObjectsRecogFilterRunable(ObjectsRecogFilter *filter, QStringList res)
{
    m_filter   = filter;
    results    = res;
}

void ObjectsRecogFilterRunable::setResults(int net, QStringList res, QList<double> conf, QList<QRectF> box, QList<QImage> mask, int inftime)
{
    network       = net;
    results       = res;
    confidence    = conf;
    boxes         = box;
    masks         = mask;
    inferenceTime = inftime;
}

void ObjectsRecogFilter::setActiveLabel(QString key, bool value)
{
    activeLabels[key] = value;
}

QMap<QString,bool> ObjectsRecogFilter::getActiveLabels()
{
    return activeLabels;
}

bool ObjectsRecogFilter::getActiveLabel(QString key)
{
    return activeLabels.value(key,false);
}

double ObjectsRecogFilter::getAngle() const
{
    return ang;
}

void ObjectsRecogFilter::setAngle(const double value)
{
    ang = value;    
    emit angleChanged();
}

double ObjectsRecogFilter::getImgHeight()
{
    return tf.getHeight();
}

double ObjectsRecogFilter::getImgWidth()
{
    return tf.getWidth();
}

QImage rotateImage(QImage img, double rotation)
{
    QPoint center = img.rect().center();
    QMatrix matrix;
    matrix.translate(center.x(), center.y());
    matrix.rotate(rotation);

    return img.transformed(matrix);
}

QVideoFrame ObjectsRecogFilterRunable::run(QVideoFrame *input, const QVideoSurfaceFormat &surfaceFormat, RunFlags flags)
{
    Q_UNUSED(surfaceFormat);
    Q_UNUSED(flags);

    QImage img;
    bool mirrorHorizontal;
    bool mirrorVertical = false;

    if(input->isValid())
    {       
        // Get image from video frame, we need to convert it
        // for unsupported QImage formats, i.e Format_YUV420P
        //
        // When input has an unsupported format the QImage
        // default format is ARGB32
        //
        // NOTE: BGR images are not properly managed by qt_imageFromVideoFrame
        //
        bool BGRVideoFrame = AuxUtils::isBGRvideoFrame(*input);
        if (BGRVideoFrame)
        {
            input->map(QAbstractVideoBuffer::ReadOnly);
            img = QImage(input->bits(),input->width(),input->height(),QImage::Format_ARGB32).copy();
            input->unmap();
            // WARNING: Mirror only for Android? How to check if this has to be done?
            // surfaceFormat.isMirrored() == false for Android
            mirrorVertical = true;
        }
        else img = qt_imageFromVideoFrame(*input);

        // Check if mirroring is needed
        if (!mirrorVertical) mirrorVertical = surfaceFormat.isMirrored();
        mirrorHorizontal = surfaceFormat.scanLineDirection() == QVideoSurfaceFormat::BottomToTop;
        img = img.mirrored(mirrorHorizontal,mirrorVertical);

        // Check img is valid
        if (img.format() != QImage::Format_Invalid)
        {
            // Take into account the rotation
            img = rotateImage(img,-m_filter->getVideoOrientation());

            // If not initialized, intialize with image size
            if (!m_filter->getInitialized())
                m_filter->init(img.height(),img.width());
            else if (m_filter->getImgHeight() != img.height() ||
                     m_filter->getImgWidth()  != img.width())
                // If image size changed, initialize input tensor
                m_filter->initInput(img.height(),img.width());

            // Get a mutex for creating a thread to execute TensorFlow
            if (m_filter->getRunning())
            {
                //img.save("/home/pi/imageTF.png");
                emit m_filter->runTensorFlow(img);
            }

            // Image classification network
            if (network == TensorFlow::knIMAGE_CLASSIFIER)
            {
                // Get current TensorFlow outputs
                QString objStr = results.count()>0    ? results.first()    : "";
                double  objCon = confidence.count()>0 ? confidence.first() : -1;

                // Check if there are results, the label is active & the minimum confidence level is reached
                if (objStr.length()>0 && objCon >= m_filter->getMinConfidence() && m_filter->getActiveLabel(objStr))
                {
                    // Formatting of confidence value
                    QString confVal = QString::number(objCon * 100, 'f', 2) + " %";


                    // Content size
                    QRectF srcRect = AuxUtils::frameMatchImg(img,m_filter->getContentSize());

                    // Text
                    QString text = objStr + '\n' + confVal;

                    // Show inference time
                    if (m_filter->getShowInfTime())
                        text = text + '\n' + QString::number(inferenceTime) + " ms";

                    img = AuxUtils::drawText(img,srcRect,text);
                }
            }
            // Object detection network
            else if (network == TensorFlow::knOBJECT_DETECTION)
            {
                QRectF  srcRect;
                bool    showInfTime  = m_filter->getShowInfTime();

                // Calculate source rect if needed
                if (showInfTime)
                    srcRect = AuxUtils::frameMatchImg(img,m_filter->getContentSize());

                // Draw masks on image
                if (!masks.isEmpty())
                    img = AuxUtils::drawMasks(img,img.rect(),results,confidence,boxes,masks,m_filter->getMinConfidence(),m_filter->getActiveLabels());

                // Draw boxes on image
                img = AuxUtils::drawBoxes(img,img.rect(),results,confidence,boxes,m_filter->getMinConfidence(),
                                          m_filter->getActiveLabels(),!BGRVideoFrame);

                // Show inference time
                if (showInfTime)
                {
                    QString text = QString::number(inferenceTime) + " ms";
                    img = AuxUtils::drawText(img,srcRect,text);
                }

            }

            // Restore rotation
            img = rotateImage(img,m_filter->getVideoOrientation());
        }

        // NOTE: for BGR images loaded as RGB
        if (BGRVideoFrame) img = img.rgbSwapped();

        // Return video frame from img
        return  QVideoFrame(img);
    }

    return *input;
}

QVideoFilterRunnable *ObjectsRecogFilter::createFilterRunnable()
{
    rfr = new ObjectsRecogFilterRunable(this,tf.getResults());
    return rfr;
}
