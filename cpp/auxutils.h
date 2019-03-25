#ifndef UTILS_H
#define UTILS_H

#include <QObject>
#include <QQuickItem>
#include <QImage>
#include <QVideoFrame>
#include <QStandardPaths>
#include <QMutex>
#include <QTimer>

const QString assetsPath = "./assets";
const QString modelName  = "detect.tflite";
const QString labelsName = "labelmap.txt";
const QString RES_CHAR   = "x";

class AuxUtils : public QObject
{
    Q_OBJECT

public slots:
    static QImage drawText(QImage image, QRectF rect, QString text, Qt::AlignmentFlag pos = Qt::AlignBottom,
                           Qt::GlobalColor borderColor = Qt::black,
                           double borderSize = 0.5,
                           Qt::GlobalColor fontColor = Qt::white,
                           QFont font = QFont("Times", 16, QFont::Bold));
    static QImage drawBoxes(QImage image, QRect rect, QStringList captions, QList<double> confidences, QList<QRectF> boxes, double minConfidence,
                            QMap<QString, bool> activeLabels, bool rgb);
    static QImage drawMasks(QImage image, QRect rect, QStringList captions, QList<double> confidences, QList<QRectF> boxes, QList<QImage> masks, double minConfidence, QMap<QString, bool> activeLabels);
    static QString getDefaultModelFilename();
    static QString getDefaultLabelsFilename();
    static QRectF frameMatchImg(QImage img, QSize rectSize);
    static int sp(int pixel, QSizeF size);
    static double dpi(QSizeF size);
    static QString deviceInfo();
    static QString qtVersion();
    static QString getAssetsPath();
    static QImage setOpacity(QImage& image, qreal opacity);
    static bool isBGRvideoFrame(QVideoFrame f);
    static bool isBGRimage(QImage i);
    bool readLabels(QString filename);
    QStringList getLabels();
    int numberThreads();
    static QVariantList networkInterfaces();
    static void setAngleHor(double angle);
    static void setAngleVer(double angle);
    static bool setResolution(QString res);

signals:
    void imageSaved(QString file);

private:
    static QString copyIfNotExistOrUpdate(QString file, QString defFile);
    static QByteArray fileMD5(QString filename);

    // Constant values
    static constexpr int     FONT_PIXEL_SIZE_TEXT = 38;
    static constexpr int     FONT_PIXEL_SIZE_BOX  = 24;
    static constexpr double  MASK_OPACITY         = 0.6;
    static constexpr double  LINE_WIDTH           = 2;
    static constexpr int     FONT_HEIGHT_MARGIN   = 3;
    static constexpr int     FONT_WIDTH_MARGIN    = 6;
    QStringList labels;

public:
    static double   angleHor;
    static double   angleVer;
    static int      width;
    static int      height;

};

#endif // UTILS_H
