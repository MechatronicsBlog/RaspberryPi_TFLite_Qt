#include "tensorflow.h"

#include "tensorflow/lite/kernels/internal/tensor.h"
#include "tensorflow/lite/kernels/internal/tensor_utils.h"

#include "get_top_n.h"
#include "colormanager.h"

#include <QDebug>
#include <QString>
#include <QFile>
#include <QElapsedTimer>
#include <QPointF>
#include <math.h>
#include <auxutils.h>

TensorFlow::TensorFlow(QObject *parent) : QObject(parent)
{
    initialized         = false;
    accelaration        = false;
    verbose             = false;
    numThreads          = 1;
    threshold           = 0.1;
    has_detection_masks = false;
}
TensorFlow::~TensorFlow()
{}

template<class T>
bool formatImageQt(T* out, QImage image, int image_channels, int wanted_height, int wanted_width, int wanted_channels, bool input_floating, bool scale = false)
{
    const float input_mean = 127.5f;
    const float input_std  = 127.5f;

    // Check same number of channels
    if (image_channels != wanted_channels)
    {
        qDebug() << "ERROR: the image has" << image_channels << " channels. Wanted channels:" << wanted_channels;
        return false;
    }

    // Scale image if needed
    if (scale && (image.width() != wanted_width || image.height() != wanted_height))
        image = image.scaled(wanted_height,wanted_width,Qt::IgnoreAspectRatio,Qt::FastTransformation);

    // Number of pixels
    const int numberPixels = image.height()*image.width()*wanted_channels;

    // Pointer to image data
    const uint8_t *output = image.bits();

    // Boolean to [0,1]
    const int inputFloat = input_floating ? 1 : 0;
    const int inputInt   = input_floating ? 0 : 1;

    // Transform to [0,128] ¿?
    for (int i = 0; i < numberPixels; i++)
    {
      out[i] = inputFloat*((output[i] - input_mean) / input_std) + // inputFloat*(output[i]/ 128.f - 1.f) +
               inputInt*(uint8_t)output[i];
      //qDebug() << out[i];
    }

    return true;
}

// -----------------------------------------------------------------------------------------------------------------------
// https://github.com/tensorflow/tensorflow/blob/master/tensorflow/contrib/lite/examples/label_image/bitmap_helpers_impl.h
// -----------------------------------------------------------------------------------------------------------------------
template <class T>
void formatImageTFLite(T* out, const uint8_t* in, int image_height, int image_width, int image_channels, int wanted_height, int wanted_width, int wanted_channels, bool input_floating)
{
   const float input_mean = 127.5f;
   const float input_std  = 127.5f;

  int number_of_pixels = image_height * image_width * image_channels;
  std::unique_ptr<Interpreter> interpreter(new Interpreter);

  int base_index = 0;

  // two inputs: input and new_sizes
  interpreter->AddTensors(2, &base_index);

  // one output
  interpreter->AddTensors(1, &base_index);

  // set input and output tensors
  interpreter->SetInputs({0, 1});
  interpreter->SetOutputs({2});

  // set parameters of tensors
  TfLiteQuantizationParams quant;
  interpreter->SetTensorParametersReadWrite(0, kTfLiteFloat32, "input",    {1, image_height, image_width, image_channels}, quant);
  interpreter->SetTensorParametersReadWrite(1, kTfLiteInt32,   "new_size", {2},quant);
  interpreter->SetTensorParametersReadWrite(2, kTfLiteFloat32, "output",   {1, wanted_height, wanted_width, wanted_channels}, quant);

  ops::builtin::BuiltinOpResolver resolver;
  const TfLiteRegistration *resize_op = resolver.FindOp(BuiltinOperator_RESIZE_BILINEAR,1);
  auto* params = reinterpret_cast<TfLiteResizeBilinearParams*>(malloc(sizeof(TfLiteResizeBilinearParams)));
  params->align_corners = false;
  interpreter->AddNodeWithParameters({0, 1}, {2}, nullptr, 0, params, resize_op, nullptr);
  interpreter->AllocateTensors();


  // fill input image
  // in[] are integers, cannot do memcpy() directly
  auto input = interpreter->typed_tensor<float>(0);
  for (int i = 0; i < number_of_pixels; i++)
    input[i] = in[i];

  // fill new_sizes
  interpreter->typed_tensor<int>(1)[0] = wanted_height;
  interpreter->typed_tensor<int>(1)[1] = wanted_width;

  interpreter->Invoke();

  auto output = interpreter->typed_tensor<float>(2);
  auto output_number_of_pixels = wanted_height * wanted_height * wanted_channels;

  for (int i = 0; i < output_number_of_pixels; i++)
  {
    if (input_floating)
      out[i] = (output[i] - input_mean) / input_std;
    else
      out[i] = (uint8_t)output[i];
  }
}
bool TensorFlow::init(int imgHeight, int imgWidth)
{
    if (!initialized)
        initialized = initTFLite(imgHeight,imgWidth);

    return initialized;
}

void TensorFlow::initInput(int imgHeight, int imgWidth)
{
     Q_UNUSED(imgHeight);
     Q_UNUSED(imgWidth);
}

// ------------------------------------------------------------------------------------------------------------------------------
// Adapted from: https://github.com/tensorflow/tensorflow/blob/master/tensorflow/contrib/lite/examples/label_image/label_image.cc
// ------------------------------------------------------------------------------------------------------------------------------
bool TensorFlow::initTFLite(int imgHeight, int imgWidth)
{
    Q_UNUSED(imgHeight);
    Q_UNUSED(imgWidth);

    try{
        // Open model & assign error reporter
        model = AuxUtils::getDefaultModelFilename().trimmed().isEmpty() && AuxUtils::getDefaultLabelsFilename().trimmed().isEmpty() ? nullptr :
                FlatBufferModel::BuildFromFile(filename.toStdString().c_str(),&error_reporter);

        if(model == nullptr)
        {
            qDebug() << "TensorFlow model loading: ERROR";
            return false;
        }

        // Link model & resolver
        InterpreterBuilder builder(*model.get(), resolver);

        // Check interpreter
        if(builder(&interpreter) != kTfLiteOk)
        {
            qDebug() << "Interpreter: ERROR";
            return false;
        }

        // Apply accelaration (Neural Network Android)
        interpreter->UseNNAPI(accelaration);

        if(interpreter->AllocateTensors() != kTfLiteOk)
        {
            qDebug() << "Allocate tensors: ERROR";
            return false;
        }

        // Set kind of network
        kind_network = interpreter->outputs().size()>1 ? knOBJECT_DETECTION : knIMAGE_CLASSIFIER;

        if (verbose)
        {
          int i_size = interpreter->inputs().size();
          int o_size = interpreter->outputs().size();
          int t_size = interpreter->tensors_size();

          qDebug() << "tensors size: "  << t_size;
          qDebug() << "nodes size: "    << interpreter->nodes_size();
          qDebug() << "inputs: "        << i_size;
          qDebug() << "outputs: "       << o_size;

          for (int i = 0; i < i_size; i++)
            qDebug() << "input" << i << "name:" << interpreter->GetInputName(i) << ", type:" << interpreter->tensor(interpreter->inputs()[i])->type;

          for (int i = 0; i < o_size; i++)
            qDebug() << "output" << i << "name:" << interpreter->GetOutputName(i) << ", type:" << interpreter->tensor(interpreter->outputs()[i])->type;

          for (int i = 0; i < t_size; i++)
          {
            if (interpreter->tensor(i)->name)
              qDebug()  << i << ":" << interpreter->tensor(i)->name << ","
                        << interpreter->tensor(i)->bytes << ","
                        << interpreter->tensor(i)->type << ","
                        << interpreter->tensor(i)->params.scale << ","
                        << interpreter->tensor(i)->params.zero_point;
          }
        }

        // Get input dimension from the input tensor metadata
        // Assuming one input only
        int input = interpreter->inputs()[0];
        TfLiteIntArray* dims = interpreter->tensor(input)->dims;

        // Save outputs
        outputs.clear();
        for(unsigned int i=0;i<interpreter->outputs().size();i++)
            outputs.push_back(interpreter->tensor(interpreter->outputs()[i]));

        wanted_height   = dims->data[1];
        wanted_width    = dims->data[2];
        wanted_channels = dims->data[3];

        if (verbose)
        {
            qDebug() << "Wanted height:"   << wanted_height;
            qDebug() << "Wanted width:"    << wanted_width;
            qDebug() << "Wanted channels:" << wanted_channels;
        }

        if (numThreads > 1)
          interpreter->SetNumThreads(numThreads);

        // Read labels
        if (readLabels()) qDebug() << "There are" << labels.count() << "labels.";
        else qDebug() << "There are NO labels";

        qDebug() << "Tensorflow initialization: OK";
        return true;

    }catch(...)
    {
        qDebug() << "Exception loading model";
        return false;
    }
}

// --------------------------------------------------------------------------------------
// Code from: https://github.com/YijinLiu/tf-cpu/blob/master/benchmark/obj_detect_lite.cc
// --------------------------------------------------------------------------------------
template<typename T>
T* TensorData(TfLiteTensor* tensor, int batch_index);

template<>
float* TensorData(TfLiteTensor* tensor, int batch_index) {
    int nelems = 1;
    for (int i = 1; i < tensor->dims->size; i++) nelems *= tensor->dims->data[i];
    switch (tensor->type) {
        case kTfLiteFloat32:
            return tensor->data.f + nelems * batch_index;
        default:
            qDebug() << "Should not reach here!";
    }
    return nullptr;
}

template<>
uint8_t* TensorData(TfLiteTensor* tensor, int batch_index) {
    int nelems = 1;
    for (int i = 1; i < tensor->dims->size; i++) nelems *= tensor->dims->data[i];
    switch (tensor->type) {
        case kTfLiteUInt8:
            return tensor->data.uint8 + nelems * batch_index;
        default:
            qDebug() << "Should not reach here!";
    }
    return nullptr;
}

int TensorFlow::getKindNetwork()
{
    return kind_network;
}

double TensorFlow::getThreshold() const
{
    return threshold;
}

void TensorFlow::setThreshold(double value)
{
    threshold = value;
}

bool TensorFlow::setInputs(QImage image)
{
    return setInputsTFLite(image);
}

bool TensorFlow::setInputsTFLite(QImage image)
{
    // Get inputs
    std::vector<int> inputs = interpreter->inputs();

    // Set inputs
    for(unsigned int i=0;i<interpreter->inputs().size();i++)
    {
        int input = inputs[i];

        // Convert input
        switch (interpreter->tensor(input)->type)
        {
            case kTfLiteFloat32:
            {
                formatImageTFLite<float>(interpreter->typed_tensor<float>(input),image.bits(), image.height(),
                                         image.width(), img_channels, wanted_height, wanted_width,wanted_channels, true);
                //formatImageQt<float>(interpreter->typed_tensor<float>(input),image,img_channels,
                //                     wanted_height,wanted_width,wanted_channels,true,true);
                break;
            }
            case kTfLiteUInt8:
            {
                formatImageTFLite<uint8_t>(interpreter->typed_tensor<uint8_t>(input),image.bits(),
                                           img_height, img_width, img_channels, wanted_height,
                                           wanted_width, wanted_channels, false);

                //formatImageQt<uint8_t>(interpreter->typed_tensor<uint8_t>(input),image,img_channels,
                //                       wanted_height,wanted_width,wanted_channels,false);
                break;
            }
            default:
            {
                qDebug() << "Cannot handle input type" << interpreter->tensor(input)->type << "yet";
                return false;
            }
        }
    }

    return true;
}

bool TensorFlow::inference()
{
    return inferenceTFLite();
}

bool TensorFlow::inferenceTFLite()
{
    // Invoke interpreter
    if (interpreter->Invoke() != kTfLiteOk)
    {
        qDebug() << "Failed to invoke interpreter";
        return false;
    }
    return true;
}
bool TensorFlow::getClassfierOutputs(std::vector<std::pair<float, int>> *top_results)
{
    return getClassfierOutputsTFLite(top_results);
}

bool TensorFlow::getClassfierOutputsTFLite(std::vector<std::pair<float, int>> *top_results)
{
    const int    output_size = 1000;
    const size_t num_results = 5;

    // Assume one output
    if (interpreter->outputs().size()>0)
    {
        int output = interpreter->outputs()[0];

        switch (interpreter->tensor(output)->type)
        {
            case kTfLiteFloat32:
            {
                tflite::label_image::get_top_n<float>(interpreter->typed_output_tensor<float>(0), output_size,
                                                      num_results, threshold, top_results, true);
                break;
            }
            case kTfLiteUInt8:
            {
                tflite::label_image::get_top_n<uint8_t>(interpreter->typed_output_tensor<uint8_t>(0),
                                                        output_size, num_results, threshold, top_results,false);
                break;
            }
            default:
            {
                qDebug() << "Cannot handle output type" << interpreter->tensor(output)->type << "yet";
                return false;
            }
        }
        return true;
    }
    return false;
}


bool TensorFlow::getObjectOutputs(QStringList &captions, QList<double> &confidences, QList<QRectF> &locations, QList<QImage> &images)
{
    return getObjectOutputsTFLite(captions,confidences,locations,images);
}

bool TensorFlow::getObjectOutputsTFLite(QStringList &captions, QList<double> &confidences, QList<QRectF> &locations, QList<QImage> &masks)
{
    if (outputs.size() >= 4)
    {
        const int    num_detections    = *TensorData<float>(outputs[3], 0);
        const float* detection_classes =  TensorData<float>(outputs[1], 0);
        const float* detection_scores  =  TensorData<float>(outputs[2], 0);
        const float* detection_boxes   =  TensorData<float>(outputs[0], 0);
        const float* detection_masks   =  !has_detection_masks || outputs.size()<5 ? nullptr : TensorData<float>(outputs[4], 0);
        ColorManager cm;

        for (int i=0; i<num_detections; i++)
        {
            // Get class
            const int cls = detection_classes[i] + 1;

            // Ignore first one
            if (cls == 0) continue;

            // Get score
            float score = detection_scores[i];

            // Check minimum score
            if (score < getThreshold()) break;

            // Get class label
            const QString label = getLabel(cls);

            // Get coordinates
            const float top    = detection_boxes[4 * i]     * img_height;
            const float left   = detection_boxes[4 * i + 1] * img_width;
            const float bottom = detection_boxes[4 * i + 2] * img_height;
            const float right  = detection_boxes[4 * i + 3] * img_width;

            // Save coordinates
            QRectF box(left,top,right-left,bottom-top);

            // Get masks
            // WARNING: Under development
            // https://github.com/matterport/Mask_RCNN/issues/222
            if (detection_masks != nullptr)
            {
                const int dim1 = outputs[4]->dims->data[2];
                const int dim2 = outputs[4]->dims->data[3];
                QImage mask(dim1,dim2,QImage::Format_ARGB32_Premultiplied);

                // Set binary mask [dim1,dim2]
                for(int j=0;j<mask.height();j++)
                    for(int k=0;k<mask.width();k++)
                        mask.setPixel(k,j,detection_masks[i*dim1*dim2 + j*dim2 + k]>= MASK_THRESHOLD ?
                                      cm.getColor(label).rgba() : QColor(Qt::transparent).rgba());

                // Billinear interpolation
                // https://chu24688.tian.yam.com/posts/44797337
                //QImage maskScaled = ColorManager::billinearInterpolation(mask,box.height(),box.width());

                // Scale mask to box size
                QImage maskScaled = mask.scaled(box.width(),box.height(),Qt::IgnoreAspectRatio,Qt::FastTransformation);

                // Border detection
                //QTransform trans(-1,0,1,-2,0,2,-1,0,1);
                //maskScaled = ColorManager::applyTransformation(maskScaled,trans);

                // Append to masks
                masks.append(maskScaled);
            }

            // Save remaining data
            captions.append(label);
            confidences.append(score);
            locations.append(box);
        }

        return true;
    }
    return false;
}

// ---------------------------------------------------------------------------------------------------------------
// Adapted from: https://github.com/tensorflow/tensorflow/tree/master/tensorflow/contrib/lite/examples/label_image
// ---------------------------------------------------------------------------------------------------------------
bool TensorFlow::run(QImage img)
{
    QElapsedTimer timer;

    if (initialized)
    {
        // Start timer
        //timer.start();

        // Transform image format & copy data
        QImage image = img.format() == format ? img : img.convertToFormat(format);

        // Store original image properties
        img_width    = image.width();
        img_height   = image.height();
        img_channels = numChannels;

        // Set inputs
        if (!setInputs(image)) return false;

        // Perform inference
        timer.start();
        if (!inference()) return false;
        inferenceTime = timer.elapsed();

        // -------------------------------------
        // Outputs depend on the kind of network
        // -------------------------------------
        rCaption.clear();
        rConfidence.clear();
        rBox.clear();
        rMasks.clear();
        //inferenceTime = 0;

        // Image classifier
        if (kind_network == knIMAGE_CLASSIFIER)
        {
            std::vector<std::pair<float, int>> top_results;

            if (!getClassfierOutputs(&top_results)) return false;

            for (const auto& result : top_results)
            {
                rConfidence.append(result.first);
                rCaption.append(getLabel(result.second));
                if (verbose) qDebug() << rConfidence.last() << ":" << rCaption.last();
            }
        }
        // Object detection
        else if (kind_network == knOBJECT_DETECTION)
        {
            if (!getObjectOutputs(rCaption,rConfidence,rBox,rMasks)) return false;
        }

        //inferenceTime = timer.elapsed();
        if (verbose) qDebug() << "Elapsed time: " << inferenceTime << "milliseconds";

        return true;
    }

    return false;
}

// WARNING: function repeated in AuxUtils
bool TensorFlow::readLabels()
{   
    if (!labelsFilename.trimmed().isEmpty())
    {
        QFile textFile(labelsFilename);

        if (textFile.exists())
        {
            QByteArray line;

            labels.clear();
            textFile.open(QIODevice::ReadOnly);

            line = textFile.readLine().trimmed();
            while(!line.isEmpty()) // !textFile.atEnd() &&
            {
                labels.append(line);
                line = textFile.readLine().trimmed();
            }

            textFile.close();
        }
        return true;
    }
    return false;
}

QString TensorFlow::getLabel(int index)
{
    if(index>=0 && index<labels.count())
        return labels[index];
    return "";
}

QString TensorFlow::getResultCaption(int index)
{
    if (index>=0 && index<rCaption.count())
        return rCaption[index];
    return "";
}

QStringList TensorFlow::getResults()
{
    return rCaption;
}

QList<double> TensorFlow::getConfidence()
{
    return rConfidence;
}

QList<QRectF> TensorFlow::getBoxes()
{
    return rBox;
}

QList<QImage> TensorFlow::getMasks()
{
    return rMasks;
}

int TensorFlow::getInferenceTime()
{
    return inferenceTime;
}

double TensorFlow::getResultConfidence(int index)
{
    if (index>=0 && index<rConfidence.count())
        return rConfidence[index];
    return -1;
}

bool TensorFlow::getAccelaration() const
{
    return accelaration;
}

void TensorFlow::setAccelaration(bool value)
{
    accelaration = value;
}

int TensorFlow::getNumThreads() const
{
    return numThreads;
}

void TensorFlow::setNumThreads(int value)
{
    numThreads = value;
}

int TensorFlow::getChannels() const
{
    return wanted_channels;
}

QString TensorFlow::getLabelsFilename() const
{
    return labelsFilename;
}

void TensorFlow::setLabelsFilename(const QString &value)
{
    labelsFilename = value;
}

int TensorFlow::getWidth() const
{
    return wanted_width;
}

int TensorFlow::getHeight() const
{
    return wanted_height;
}

bool TensorFlow::getVerbose() const
{
    return verbose;
}

void TensorFlow::setVerbose(bool value)
{
    verbose = value;
}

QString TensorFlow::getFilename() const
{
    return filename;
}

void TensorFlow::setFilename(const QString &value)
{
    filename = value;
    filename = filename.replace("file://","");
}
