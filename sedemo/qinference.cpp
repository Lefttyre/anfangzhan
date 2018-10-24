#include "qinference.h"
#include <cassert>
#include <fstream>
#include <sstream>
#include <iostream>
#include <cmath>
#include <sys/stat.h>
#include <cmath>
#include <time.h>
#include <cuda_runtime_api.h>
#include <cudnn.h>
#include <cublas_v2.h>
#include <memory>
#include <cstring>
#include <algorithm>
#include <map>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include<opencv2/dnn.hpp>
#include <experimental/filesystem>
#include "NvCaffeParser.h"
#include "NvInferPlugin.h"
#include "common.h"
#include "GlobalVariable.h"
#define SHOW_PERF false

static Logger gLogger;
using namespace nvinfer1;
using namespace nvcaffeparser1;
using namespace plugin;

const char* INPUT_BLOB_NAME0 = "data";
const char* OUTPUT_BLOB_NAME0 = "detection_out";

void cudaSoftmax(int n, int channels,  float* x, float*y, uint batchSize);

std::string locateFile(const std::string& input)
{
    std::vector<std::string> dirs{"data/", "ssd/"};
    return locateFile(input, dirs);
}

bool fileExists(const std::string fileName)
{
    if (!std::experimental::filesystem::exists(std::experimental::filesystem::path(fileName)))
    {
        std::cout << "File does not exist : " << fileName << std::endl;
        return false;
    }
    return true;
}

struct PPM
{
    std::string magic, fileName;
    int h, w, max;
    uint8_t buffer[INPUT_C*INPUT_H*INPUT_W];
};

// simple PPM (portable pixel map) reader
void readPPMFile(const std::string& filename, PPM& ppm)
{
    ppm.fileName = filename;
    std::ifstream infile(locateFile(filename), std::ifstream::binary);
    infile >> ppm.magic >> ppm.w >> ppm.h >> ppm.max;
    infile.seekg(1, infile.cur);
    infile.read(reinterpret_cast<char*>(ppm.buffer), ppm.w * ppm.h * 3);
}

void caffeToGIEModel(const std::string& deployFile,			// name for caffe prototxt
    const std::string& modelFile,			// name for model
    const std::vector<std::string>& detection_outs,		// network detection_outs
    unsigned int maxBatchSize,				// batch size - NB must be at least as large as the batch we want to run with)
    nvcaffeparser1::IPluginFactory* pluginFactory,	// factory for plugin layers
    IHostMemory **gieModelStream)			// detection_out stream for the GIE model
{
    // create the builder
    IBuilder* builder = createInferBuilder(gLogger);

    // parse the caffe model to populate the network, then set the detection_outs
    INetworkDefinition* network = builder->createNetwork();
    ICaffeParser* parser = createCaffeParser();
    parser->setPluginFactory(pluginFactory);

    std::cout << "Begin parsing model..." << std::endl;

    const IBlobNameToTensor* blobNameToTensor = parser->parse(deployFile.c_str(),
        modelFile.c_str(),
        *network,
        DataType::kFLOAT);
    std::cout << "End parsing model..." << std::endl;
    // specify which tensors are detection_outs
    for (auto& s : detection_outs)
        network->markOutput(*blobNameToTensor->find(s.c_str()));

    for (int i = 0, n = network->getNbOutputs(); i < n; i++)
    {
        DimsCHW dims = static_cast<DimsCHW&&>(network->getOutput(i)->getDimensions());
        std::cout << "Output \"" << network->getOutput(i)->getName() << "\": " << dims.c() << "x" << dims.h() << "x" << dims.w() << std::endl;
    }

    // Build the engine
    builder->setMaxBatchSize(maxBatchSize);
    builder->setMaxWorkspaceSize(1024 << 20);	// we need about 6MB of scratch space for the plugin layer for batch size 5
        builder->setHalf2Mode(true);

    std::cout << "Begin building engine..." << std::endl;
    ICudaEngine* engine = builder->buildCudaEngine(*network);
    assert(engine);
    std::cout << "End building engine..." << std::endl;

    // we don't need the network any more, and we can destroy the parser
    network->destroy();
    parser->destroy();

    // serialize the engine, then close everything down
    (*gieModelStream) = engine->serialize();

    engine->destroy();
    builder->destroy();
    shutdownProtobufLibrary();
}

void QInference::doInference(IExecutionContext& context, float* inputData, float* detection_out, int batchSize) {
#if SHOW_PERF
    float total = 0, ms;
    cudaEvent_t start, end;
    CHECK(cudaEventCreateWithFlags(&start, cudaEventBlockingSync));
    CHECK(cudaEventCreateWithFlags(&end, cudaEventBlockingSync));
#endif
    // DMA the input to the GPU,  execute the batch asynchronously, and DMA it back:
    CHECK(cudaMemcpyAsync(m_Bindings[inputIndex0], inputData, batchSize * INPUT_C * INPUT_H * INPUT_W * sizeof(float), cudaMemcpyHostToDevice, m_CudaStream));

#if SHOW_PERF
    cudaEventRecord(start, m_CudaStream);
#endif

    context.enqueue(batchSize, m_Bindings.data(), m_CudaStream, nullptr);

#if SHOW_PERF
    cudaEventRecord(end, m_CudaStream);
    cudaEventSynchronize(end);
    cudaEventElapsedTime(&ms, start, end);
    total += ms;
    std::cout << " runs  " << total << " ms." << std::endl;
#endif
    CHECK(cudaMemcpyAsync(detection_out, m_Bindings[detection_outIndex0], batchSize * OUTPUT_DETECTION_OUT * sizeof(float), cudaMemcpyDeviceToHost, m_CudaStream));
    cudaStreamSynchronize(m_CudaStream);

    // release the stream and the buffers
#if SHOW_PERF
    cudaEventDestroy(start);
    cudaEventDestroy(end);
#endif
}


using namespace nvinfer1;
using namespace nvcaffeparser1;
using namespace plugin;
void cudaSoftmax(int n, int channels,  float* x, float*y, uint batchSize);

//Softmax layer.TensorRT softmax only support cross channel
class SoftmaxPlugin : public IPlugin
{
    //You need to implement it when softmax parameter axis is 2.
public:
    int initialize() override { return 0; }
    inline void terminate() override {}

    SoftmaxPlugin(){}
    SoftmaxPlugin( const void* buffer, size_t size)
    {
        assert(size == sizeof(mCopySize));
        mCopySize = *reinterpret_cast<const size_t*>(buffer);
    }
    inline int getNbOutputs() const override
    {
        //@TODO:  As the number of outputs are only 1, because there is only layer in top.
        return 1;
    }
    Dims getOutputDimensions(int index, const Dims* inputs, int nbInputDims) override
    {
        assert(nbInputDims == 1);
        assert(index == 0);
        assert(inputs[index].nbDims == 3);
//        assert((inputs[0].d[0])*(inputs[0].d[1]) % OutC == 0);

        // @TODO: Understood this.
        return DimsCHW( inputs[0].d[0] , inputs[0].d[1] , inputs[0].d[2] );
    }

    size_t getWorkspaceSize(int) const override
    {
        // @TODO: 1 is the batch size.
        return mCopySize*1;
    }

    int enqueue(int batchSize, const void*const *inputs, void** outputs, void* workspace, cudaStream_t stream) override
    {
#if SHOW_PERF
        //std::cout<<"flatten enqueue:"<<batchSize<<";"<< mCopySize<<std::endl;
#endif
//        CHECK(cudaMemcpyAsync(outputs[0],inputs[0],batchSize*mCopySize*sizeof(float),cudaMemcpyDeviceToDevice,stream));

        cudaSoftmax( 8732*21, 21, (float *) *inputs, static_cast<float *>(*outputs),batchSize);

        return 0;
    }

    size_t getSerializationSize() override
    {
        return sizeof(mCopySize);
    }
    void serialize(void* buffer) override
    {
        *reinterpret_cast<size_t*>(buffer) = mCopySize;
    }
    void configure(const Dims*inputs, int nbInputs, const Dims* outputs, int nbOutputs, int)	override
    {
        mCopySize = inputs[0].d[0] * inputs[0].d[1] * inputs[0].d[2] * sizeof(float);
    }

protected:
    size_t mCopySize;

};

class FlattenLayer: public IPlugin
{

protected:

  DimsCHW dimBottom;

  int _size;

public:

  FlattenLayer()
  {
  }

  FlattenLayer(const void* buffer, size_t size)
  {
    assert(size == 3 * sizeof(int));

    const int* d = reinterpret_cast<const int*>(buffer);

    _size = d[0] * d[1] * d[2];

    dimBottom = DimsCHW
        { d[0], d[1], d[2] };

  }

  inline int getNbOutputs() const override
  {
    return 1;
  }
  ;

  Dims getOutputDimensions(int index, const Dims* inputs, int nbInputDims) override
  {

    assert(1 == nbInputDims);
    assert(0 == index);
    assert(3 == inputs[index].nbDims);

    _size = inputs[0].d[0] * inputs[0].d[1] * inputs[0].d[2];

    return DimsCHW(_size, 1, 1);

  }

  int initialize() override
  {

    return 0;

  }

  inline void terminate() override
  {

  }

  inline size_t getWorkspaceSize(int) const override
  {
    return 0;
  }

  int enqueue(int batchSize, const void* const *inputs, void** detection_outs, void*, cudaStream_t stream) override
  {

    CHECK(cudaMemcpyAsync(detection_outs[0], inputs[0], batchSize * _size * sizeof(float), cudaMemcpyDeviceToDevice, stream));

    return 0;

  }

  size_t getSerializationSize() override
  {

    return 3 * sizeof(int);

  }

  void serialize(void* buffer) override
  {

    int* d = reinterpret_cast<int*>(buffer);

    d[0] = dimBottom.c();
    d[1] = dimBottom.h();
    d[2] = dimBottom.w();

  }

  void configure(const Dims*inputs, int nbInputs, const Dims* detection_outs, int nbOutputs, int) override
  {

    dimBottom = DimsCHW(inputs[0].d[0], inputs[0].d[1], inputs[0].d[2]);

  }

};


template<int OutC>
class ReshapeLayer: public IPlugin
{
public:
  ReshapeLayer()
  {
  }
  ReshapeLayer(const void* buffer, size_t size)
  {
    assert(size == sizeof(mCopySize));
    mCopySize = *reinterpret_cast<const size_t*>(buffer);
  }

  int getNbOutputs() const override
  {
    return 1;
  }
  Dims getOutputDimensions(int index, const Dims* inputs, int nbInputDims) override
  {
    return DimsCHW(inputs[0].d[0], inputs[0].d[1], inputs[0].d[2]);
    assert(nbInputDims == 1);
    assert(index == 0);
    assert(inputs[index].nbDims == 3);
    assert((inputs[0].d[0]) * (inputs[0].d[1]) % OutC == 0);
    return DimsCHW(OutC, inputs[0].d[0] * inputs[0].d[1] / OutC, inputs[0].d[2]);
  }

  int initialize() override
  {
    return 0;
  }

  void terminate() override
  {
  }

  size_t getWorkspaceSize(int) const override
  {
    return 0;
  }

  // currently it is not possible for a plugin to execute "in place". Therefore we memcpy the data from the input to the detection_out buffer
  int enqueue(int batchSize, const void* const *inputs, void** detection_outs, void*, cudaStream_t stream) override
  {
    CHECK(cudaMemcpyAsync(detection_outs[0], inputs[0], mCopySize * batchSize, cudaMemcpyDeviceToDevice, stream));
    return 0;
  }

  size_t getSerializationSize() override
  {
    return sizeof(mCopySize);
  }

  void serialize(void* buffer) override
  {
    *reinterpret_cast<size_t*>(buffer) = mCopySize;
  }

  void configure(const Dims*inputs, int nbInputs, const Dims* detection_outs, int nbOutputs, int) override
  {
    mCopySize = inputs[0].d[0] * inputs[0].d[1] * inputs[0].d[2] * sizeof(float);
  }

protected:
  size_t mCopySize;
};


struct Profiler : public IProfiler
{
    typedef std::pair<std::string, float> Record;
    std::vector<Record> mProfile;

    virtual void reportLayerTime(const char* layerName, float ms)
    {
        auto record = std::find_if(mProfile.begin(), mProfile.end(), [&](const Record& r){ return r.first == layerName; });

        if (record == mProfile.end()) mProfile.push_back(std::make_pair(layerName, ms));
        else record->second += ms;
    }

    void printLayerTimes(const int TIMING_ITERATIONS)
    {
        float totalTime = 0;
        for (size_t i = 0; i < mProfile.size(); i++)
        {
            printf("%-40.40s %4.3fms\n", mProfile[i].first.c_str(), mProfile[i].second / TIMING_ITERATIONS);
            totalTime += mProfile[i].second;
        }
        printf("Time over all layers: %4.3f\n", totalTime / TIMING_ITERATIONS);
    }
};

class PluginFactory : public nvinfer1::IPluginFactory, public nvcaffeparser1::IPluginFactory {
public:

        // caffe parser plugin implementation
        bool isPlugin(const char* name) override
        {
        return (!strcmp(name, "res3d_relu_mbox_loc_perm")
            || !strcmp(name, "res3d_relu_mbox_loc_flat")
            || !strcmp(name, "res3d_relu_mbox_conf_perm")
            || !strcmp(name, "res3d_relu_mbox_conf_flat")
            || !strcmp(name, "res3d_relu_mbox_priorbox")
            || !strcmp(name, "res4f_relu_mbox_loc_perm")
            || !strcmp(name, "res4f_relu_mbox_loc_flat")
            || !strcmp(name, "res4f_relu_mbox_conf_perm")
            || !strcmp(name, "res4f_relu_mbox_conf_flat")
            || !strcmp(name, "res4f_relu_mbox_priorbox")
            || !strcmp(name, "pool5_mbox_loc_perm")
            || !strcmp(name, "pool5_mbox_loc_flat")
            || !strcmp(name, "pool5_mbox_conf_perm")
            || !strcmp(name, "pool5_mbox_conf_flat")
            || !strcmp(name, "pool5_mbox_priorbox")
            || !strcmp(name, "mbox_loc")
            || !strcmp(name, "mbox_conf")
            || !strcmp(name, "mbox_priorbox")
            || !strcmp(name, "mbox_conf_reshape")
            || !strcmp(name, "mbox_conf_softmax")
            || !strcmp(name, "mbox_conf_flatten")
            || !strcmp(name, "detection_out")
        );

        }

        virtual IPlugin* createPlugin(const char* layerName, const Weights* weights, int nbWeights) override
        {
                // there's no way to pass parameters through from the model definition, so we have to define it here explicitly
                if(!strcmp(layerName, "conv4_3_norm")){

            //INvPlugin *   plugin::createSSDNormalizePlugin (const Weights *scales, bool acrossSpatial, bool channelShared, float eps)

            _nvPlugins[layerName] = plugin::createSSDNormalizePlugin(weights,false,false,1e-10);

            return _nvPlugins.at(layerName);

                }else if(!strcmp(layerName, "res3d_relu_mbox_loc_perm")
            ||  !strcmp(layerName, "res3d_relu_mbox_conf_perm")
            ||  !strcmp(layerName,"res4f_relu_mbox_loc_perm")
            ||  !strcmp(layerName,"res4f_relu_mbox_conf_perm")
            ||  !strcmp(layerName,"pool5_mbox_loc_perm")
            || !strcmp(layerName,"pool5_mbox_conf_perm")
        ){

            _nvPlugins[layerName] = plugin::createSSDPermutePlugin(Quadruple({0,2,3,1}));

            return _nvPlugins.at(layerName);

                } else if(!strcmp(layerName,"res3d_relu_mbox_priorbox")){

            plugin::PriorBoxParameters params = {0};
            float minSize[1] = {30.0f};
            //float maxSize[1] = {60.0f};
            float aspectRatios[1] = {2.0f};
            params.minSize = (float*)minSize;
            //params.maxSize = (float*)maxSize;
            params.aspectRatios = (float*)aspectRatios;
            params.numMinSize = 1;
            //params.numMaxSize = 1;
            params.numAspectRatios = 1;
            params.flip = true;
            params.clip = true;
            params.variance[0] = 0.10000000149;
            params.variance[1] = 0.10000000149;
            params.variance[2] = 0.20000000298;
            params.variance[3] = 0.20000000298;
           // params.imgH = 0;
           // params.imgW = 0;
           // params.stepH = 8.0f;
           // params.stepW = 8.0f;
            params.offset = 0.5f;
            _nvPlugins[layerName] = plugin::createSSDPriorBoxPlugin(params);

            return _nvPlugins.at(layerName);
        }else if(!strcmp(layerName,"res4f_relu_mbox_priorbox")){

            plugin::PriorBoxParameters params = {0};
            float minSize[1] = {60.0f};
            float maxSize[1] = {285.0f};
            float aspectRatios[2] = {2.0f, 3.0f};
            params.minSize = (float*)minSize;
            params.maxSize = (float*)maxSize;
            params.aspectRatios = (float*)aspectRatios;
            params.numMinSize = 1;
            params.numMaxSize = 1;
            params.numAspectRatios = 2;
            params.flip = true;
            params.clip = true;
            params.variance[0] = 0.10000000149;
            params.variance[1] = 0.10000000149;
            params.variance[2] = 0.20000000298;
            params.variance[3] = 0.20000000298;
           // params.imgH = 0;
           // params.imgW = 0;
           // params.stepH = 16.0f;
            //params.stepW = 16.0f;
            params.offset = 0.5f;
            _nvPlugins[layerName] = plugin::createSSDPriorBoxPlugin(params);

            return _nvPlugins.at(layerName);
        }else if(!strcmp(layerName,"pool5_mbox_priorbox")){
            plugin::PriorBoxParameters params = {0};
            float minSize[1] = {285.0f};
            float maxSize[1] = {510.0f};
            float aspectRatios[2] = {2.0f, 3.0f};
            params.minSize = (float*)minSize;
            params.maxSize = (float*)maxSize;
            params.aspectRatios = (float*)aspectRatios;
            params.numMinSize = 1;
            params.numMaxSize = 1;
            params.numAspectRatios = 2;
            params.flip = true;
            params.clip = true;
            params.variance[0] = 0.10000000149;
            params.variance[1] = 0.10000000149;
            params.variance[2] = 0.20000000298;
            params.variance[3] = 0.20000000298;
            //params.imgH = 0;
            //params.imgW = 0;
            //params.stepH = 32.0f;
           // params.stepW = 32.0f;
            params.offset = 0.5f;
            _nvPlugins[layerName] = plugin::createSSDPriorBoxPlugin(params);

            return _nvPlugins.at(layerName);
        }else if(!strcmp(layerName,"detection_out")
            ||!strcmp(layerName,"detection_out2")
        ){
            plugin::DetectionOutputParameters params = {0};
            params.numClasses = 21;
            params.shareLocation = true;
            params.varianceEncodedInTarget = false;
            params.backgroundLabelId = 0;
            params.keepTopK = 200;
            params.codeType = plugin::CodeTypeSSD::CENTER_SIZE;
            params.nmsThreshold = 0.45;
            params.topK = 400;
            params.confidenceThreshold = 0.01;
        params.inputOrder[0]=0;
        params.inputOrder[1]=1;
        params.inputOrder[2]=2;
            _nvPlugins[layerName] = plugin::createSSDDetectionOutputPlugin (params);
            return _nvPlugins.at(layerName);
        }else if (
            !strcmp(layerName, "res3d_relu_mbox_loc_flat")
            ||!strcmp(layerName,"res3d_relu_mbox_conf_flat")
            ||!strcmp(layerName,"res4f_relu_mbox_loc_flat")
            ||!strcmp(layerName,"res4f_relu_mbox_conf_flat")
            ||!strcmp(layerName,"pool5_mbox_conf_flat")
            ||!strcmp(layerName,"pool5_mbox_loc_flat")
            ||!strcmp(layerName,"mbox_conf_flatten")
        ){
            _nvPlugins[layerName] = (plugin::INvPlugin*)(new FlattenLayer());
            return _nvPlugins.at(layerName);
        }else if(!strcmp(layerName,"mbox_conf_reshape")){
            _nvPlugins[layerName] = (plugin::INvPlugin*)new ReshapeLayer<21>();
            return _nvPlugins.at(layerName);
        }else if(!strcmp(layerName,"mbox_conf_softmax")){
            _nvPlugins[layerName] = (plugin::INvPlugin*)new SoftmaxPlugin();
            return _nvPlugins.at(layerName);
        }else if(!strcmp(layerName,"mbox_loc")){
            _nvPlugins[layerName] = plugin::createConcatPlugin (1,false);
            return _nvPlugins.at(layerName);
        }else if(!strcmp(layerName,"mbox_conf")){
            _nvPlugins[layerName] = plugin::createConcatPlugin (1,false);
            return _nvPlugins.at(layerName);
        }else if(!strcmp(layerName,"mbox_priorbox")){
            _nvPlugins[layerName] = plugin::createConcatPlugin (2,false);
            return _nvPlugins.at(layerName);
    }else {
        cout<<" error "<<layerName<<endl;
            assert(0);
            return nullptr;
        }
    }

    // deserialization plugin implementation
    IPlugin* createPlugin(const char* layerName, const void* serialData, size_t serialLength) override {
        if(!strcmp(layerName, "conv4_3_norm"))
        {
            _nvPlugins[layerName] = plugin::createSSDNormalizePlugin(serialData, serialLength);
            return _nvPlugins.at(layerName);
        }else if( !strcmp(layerName, "res3d_relu_mbox_loc_perm")
            ||  !strcmp(layerName, "res3d_relu_mbox_conf_perm")
            ||  !strcmp(layerName,"res4f_relu_mbox_loc_perm")
            ||  !strcmp(layerName,"res4f_relu_mbox_conf_perm")
            ||  !strcmp(layerName,"pool5_mbox_loc_perm")
            || !strcmp(layerName,"pool5_mbox_conf_perm")
        ){
            _nvPlugins[layerName] = plugin::createSSDPermutePlugin(serialData, serialLength);
            return _nvPlugins.at(layerName);

        }else if(!strcmp(layerName,"res3d_relu_mbox_priorbox")
            || !strcmp(layerName,"res4f_relu_mbox_priorbox")
            || !strcmp(layerName,"pool5_mbox_priorbox")
        ){
            _nvPlugins[layerName] = plugin::createSSDPriorBoxPlugin (serialData, serialLength);
            return _nvPlugins.at(layerName);
        }else if(!strcmp(layerName,"detection_out")
            || !strcmp(layerName,"detection_out2")
            ){
            _nvPlugins[layerName] = plugin::createSSDDetectionOutputPlugin (serialData, serialLength);
            return _nvPlugins.at(layerName);
        }else if(!strcmp(layerName,"mbox_conf_reshape")){
            _nvPlugins[layerName] = (plugin::INvPlugin*)new ReshapeLayer<21>(serialData, serialLength);
            return _nvPlugins.at(layerName);
        }else if(!strcmp(layerName,"mbox_conf_softmax")){
            _nvPlugins[layerName] = (plugin::INvPlugin*)new SoftmaxPlugin(serialData, serialLength);
            return _nvPlugins.at(layerName);
        }else if (
            !strcmp(layerName, "res3d_relu_mbox_loc_flat")
            ||!strcmp(layerName,"res3d_relu_mbox_conf_flat")
            ||!strcmp(layerName,"res4f_relu_mbox_loc_flat")
            ||!strcmp(layerName,"res4f_relu_mbox_conf_flat")
            ||!strcmp(layerName,"pool5_mbox_conf_flat")
            ||!strcmp(layerName,"pool5_mbox_loc_flat")
            ||!strcmp(layerName,"mbox_conf_flatten")
        ){
            _nvPlugins[layerName] = (plugin::INvPlugin*)(new FlattenLayer(serialData, serialLength));
            return _nvPlugins.at(layerName);
        }else if(!strcmp(layerName,"mbox_loc")){
            _nvPlugins[layerName] = plugin::createConcatPlugin (serialData, serialLength);
            return _nvPlugins.at(layerName);
        }else if(!strcmp(layerName,"mbox_conf")){
            _nvPlugins[layerName] = plugin::createConcatPlugin (serialData, serialLength);
            return _nvPlugins.at(layerName);
        }else if(!strcmp(layerName,"mbox_priorbox")){
            _nvPlugins[layerName] = plugin::createConcatPlugin (serialData, serialLength);
            return _nvPlugins.at(layerName);
        }else{
            assert(0);
            return nullptr;
        }
    }


    void destroyPlugin()
    {
        for (auto it=_nvPlugins.begin(); it!=_nvPlugins.end(); ++it){
            it->second->destroy();
            _nvPlugins.erase(it);
        }
    }


private:

        std::map<std::string, plugin::INvPlugin*> _nvPlugins;
};

void QInference::init()
{
    cudaSetDevice(0);
    PluginFactory pluginFactory;

    std::string prototxt = "../ssd/deploy.prototxt";
    std::string modelfile = "../ssd/ResNet34slim2_VOC0712_SSD_300x300_iter_60000.caffemodel";
    std::string modelpath =  "../ssd/";

    std::string planFilePath =  modelpath + "ResNet34SSDBatch" + std::to_string(s_batchSize) + ".engine";

    if(!fileExists(planFilePath)){
        IHostMemory *trtModelStream{ nullptr };
        caffeToGIEModel(prototxt, modelfile,
            std::vector < std::string > { OUTPUT_BLOB_NAME0},
            s_batchSize, &pluginFactory, &trtModelStream);
        assert(trtModelStream != nullptr);

        // write data to output file
        std::cout << "Serialized plan file cached at location : " << planFilePath << std::endl;
        std::stringstream gieModelStream;
        gieModelStream.seekg(0, gieModelStream.beg);
        gieModelStream.write(static_cast<const char*>(trtModelStream->data()), trtModelStream->size());
        std::ofstream outFile;
        outFile.open(planFilePath);
        outFile << gieModelStream.rdbuf();
        outFile.close();

        // Deserialize the engine.
        std::cout << "*** deserializing" << std::endl;
        nvinfer1::IRuntime* runtime = nvinfer1::createInferRuntime(gLogger);
        assert(runtime != nullptr);
        m_iEngine = runtime->deserializeCudaEngine(trtModelStream->data(), trtModelStream->size(), &pluginFactory);
        trtModelStream->destroy();
        runtime->destroy();
    } else {
        std::cout << "Using previously generated plan file located at " << planFilePath << std::endl;
        // reading the model in memory
        std::stringstream ModelStream;
        ModelStream.seekg(0, ModelStream.beg);
        std::ifstream cache(planFilePath);
        assert(cache.good());
        ModelStream << cache.rdbuf();
        cache.close();

        // calculating model size
        ModelStream.seekg(0, std::ios::end);
        const int modelSize = ModelStream.tellg();
        ModelStream.seekg(0, std::ios::beg);
        void* modelMem = malloc(modelSize);
        ModelStream.read((char*) modelMem, modelSize);
        nvinfer1::IRuntime* runtime = nvinfer1::createInferRuntime(gLogger);
        assert(runtime != nullptr);
        m_iEngine = runtime->deserializeCudaEngine(modelMem, modelSize, &pluginFactory);
        free(modelMem);
        runtime->destroy();
        std::cout << "Loading Complete!" << std::endl;
    }

    assert(m_iEngine != nullptr);
    m_iContext = m_iEngine->createExecutionContext();
    assert(m_iContext != nullptr);
    CHECK(cudaStreamCreate(&m_CudaStream));
    m_Bindings.resize(m_iEngine->getNbBindings(),nullptr);
    inputIndex0 = m_iEngine->getBindingIndex(INPUT_BLOB_NAME0),
    detection_outIndex0 = m_iEngine->getBindingIndex(OUTPUT_BLOB_NAME0);
    // create GPU buffers and a stream
    CHECK(cudaMalloc(&m_Bindings[inputIndex0], s_batchSize * INPUT_C * INPUT_H * INPUT_W * sizeof(float)));   // data
    CHECK(cudaMalloc(&m_Bindings[detection_outIndex0], s_batchSize * OUTPUT_DETECTION_OUT * sizeof(float))); // bbox_pred
}

void QInference::uninit()
{
    m_iContext->destroy();
    m_iEngine->destroy();
    m_iRuntime->destroy();
}

void QInference::detectFrame(GlobalVariable::FramePreProcessed objFramePrePro)
{
    uint batchsize = s_batchSize;

    memset(detection_out, batchsize*OUTPUT_DETECTION_OUT*sizeof (float),0);
    // run inference
    doInference(*m_iContext, (float*)objFramePrePro.blobimages.data, detection_out, batchsize);

    memcpy(m_Inference_out.inference_out.data(),detection_out,batchsize * OUTPUT_DETECTION_OUT*sizeof (float));

    QVariant v;
    m_Inference_out.batch_images = objFramePrePro.batch_images;
    m_Inference_out.imgidx = objFramePrePro.imgidx;
    v.setValue(m_Inference_out);

    emit(callAfterInference(v));
    return ;
}

QInference::QInference(uint idx, QObject *parent): id(idx),QThread (parent) {
   qRegisterMetaType<QVariant>("QVariant");
   // host memory for detection_outs
   m_CudaStream = nullptr;
   detection_out = new float[s_batchSize * OUTPUT_DETECTION_OUT];
   m_Inference_out.inference_out.resize(s_batchSize * OUTPUT_DETECTION_OUT);
   m_Inference_out.batch_images.resize(s_batchSize);
}

QInference::~QInference()
{
    for(auto bindings: m_Bindings){
        cudaFree(bindings);
    }
    cudaStreamDestroy(m_CudaStream);
    delete[] detection_out;
}

void QInference::run()
{
    while (true)
    {
        GlobalVariable::s_semPreProUseds[id]->acquire();
        if(GlobalVariable::s_aryPreProcessedFramesess[id].empty()) {
            GlobalVariable::s_semPreProFrees[id]->release();
            continue;
        }
        auto objFramePrePro= GlobalVariable::s_aryPreProcessedFramesess[id].takeFirst();
        GlobalVariable::s_semPreProFrees[id]->release();
        detectFrame(objFramePrePro);
    }


    // create a GIE model from the caffe model and serialize it to a stream

}
