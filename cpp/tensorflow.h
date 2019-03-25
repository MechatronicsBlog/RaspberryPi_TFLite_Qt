#ifndef TENSORFLOW_H
#define TENSORFLOW_H

#include <QObject>
#include <QRectF>
#include <QImage>

#include "tensorflow/lite/error_reporter.h"
#include "tensorflow/lite/interpreter.h"
#include "tensorflow/lite/model.h"
#include "tensorflow/lite/graph_info.h"
#include "tensorflow/lite/kernels/register.h"

using namespace tflite;

class TensorFlow : public QObject
{
    Q_OBJECT
public:
    explicit TensorFlow(QObject *parent = nullptr);
    ~TensorFlow();

    static const int knIMAGE_CLASSIFIER = 1;
    static const int knOBJECT_DETECTION = 2;
    static const int DEF_BOX_DISTANCE   = 10;

signals:

public slots:
    bool init(int imgHeight, int imgWidth);
    bool run(QImage img);
    QString getFilename() const;
    void setFilename(const QString &value);
    QString getLabelsFilename() const;
    void setLabelsFilename(const QString &value);
    bool getAccelaration() const;
    void setAccelaration(bool value);
    bool getVerbose() const;
    void setVerbose(bool value);
    int getNumThreads() const;
    void setNumThreads(int value);
    int getHeight() const;
    int getWidth() const;
    int getChannels() const;
    QString getLabel(int index);
    QString getResultCaption(int index);
    double  getResultConfidence(int index);
    QStringList getResults();
    QList<double> getConfidence();
    QList<QRectF> getBoxes();
    QList<QImage> getMasks();
    int getInferenceTime();
    int getKindNetwork();
    double getThreshold() const;
    void setThreshold(double value);
    void initInput(int imgHeight, int imgWidth);
    bool initTFLite(int imgHeight, int imgWidth);
    bool setInputsTFLite(QImage image);
    bool inferenceTFLite();
    bool getClassfierOutputsTFLite(std::vector<std::pair<float, int>> *top_results);
    bool getObjectOutputsTFLite(QStringList &captions, QList<double> &confidences, QList<QRectF> &locations, QList<QImage> &masks);

private:
    // Configuration constants
    const double MASK_THRESHOLD = 0.3;

    // Output names
    const QString num_detections    = "num_detections";
    const QString detection_classes = "detection_classes";
    const QString detection_scores  = "detection_scores";
    const QString detection_boxes   = "detection_boxes";
    const QString detection_masks   = "detection_masks";

    // Network configuration
    bool has_detection_masks;

    // Threshold
    double threshold;

    // Image properties
    const QImage::Format format = QImage::Format_RGB888;
    const int numChannels = 3;

    // Kind of network in model
    int kind_network;

    // Model filename
    QString filename;

    // Labels & box priors filename & data
    QString labelsFilename;
    QStringList labels;

    // Results
    QStringList   rCaption;
    QList<double> rConfidence;
    QList<QRectF> rBox;
    QList<QImage> rMasks;
    int inferenceTime;

    // Initialized
    bool initialized;

    // Accelaration
    bool accelaration;

    // Verbose
    bool verbose;

    // Number of threads
    int numThreads;

    // Image configuration
    int wanted_height, wanted_width, wanted_channels;
    int img_height, img_width, img_channels;

    // Model
    std::unique_ptr<FlatBufferModel> model;
    // Resolver
    tflite::ops::builtin::BuiltinOpResolver resolver;
    // Interpreter
    std::unique_ptr<Interpreter> interpreter;
    // Error reporter
    StderrReporter error_reporter;
    // Outputs
    std::vector<TfLiteTensor*> outputs;

    // Private functions
    bool readLabels();
    bool setInputs(QImage image);
    bool inference();
    bool getClassfierOutputs(std::vector<std::pair<float, int>> *top_results);
    bool getObjectOutputs(QStringList &captions, QList<double> &confidences, QList<QRectF> &locations, QList<QImage> &images);
};

#endif // TENSORFLOW_H
