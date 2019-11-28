#include "ImgWindowWorker.h"
#include "tbb/parallel_for.h"

ImgWindowWorker::ImgWindowWorker(int imageW, int imageH)
    : _imageW(imageW), _imageH(imageH) {

  std::string kernel_path = "../src/opencl/opencl_kernel.cl";
  _clOperator = new CLOperator(kernel_path);

  // allocate memory on CPU for the image
  _clOperator->SetBufferOutput(imageW, imageH);
  _cpu_output = new cl_float3[imageW * imageH];

  _scene = new Scene();
  _clOperator->SetScene(_scene);

  _cam = new Camera();
  _cam->foyer = {0.0f, 50.0f, 90.0f};
  _cam->fov = 60.0f * PI / 180.0f;
  _clOperator->SetCamera(_cam);

  _pixelBuffer = new unsigned char[imageW * imageH * 4];
}

ImgWindowWorker::~ImgWindowWorker() {
  delete _cpu_output;
  delete _clOperator;
  delete _scene;
  delete _cam;
  delete _pixelBuffer;
}

void ImgWindowWorker::startProcess() {

  std::cout << "ImgWindowWorker START" << std::endl;

  cl_float3 vec_displ = {0.0f, -0.3f, -0.5f};
  for (int i = 0; i < 100; i++) {
    // launch the kernel
    std::cout << " NEW IMAGE START" << std::endl;
    _clOperator->LaunchKernel();
    std::cout << " NEW IMAGE READY" << std::endl;
    // emit newImgSignal();

    _cam->foyer = _cam->foyer + vec_displ;
    _clOperator->SetCamera(_cam);
  }

  // release memory
  // cleanUp();
  system("PAUSE");
  emit finishedSignal();
}

void ImgWindowWorker::copyImage(QPixmap &pixmap) {
  std::cout << "WORKER - copy Image" << std::endl;

  _clOperator->ReadOutput(_cpu_output);
  std::cout << "         copy Image -> read output done" << std::endl;

  tbb::parallel_for((unsigned int)0, (unsigned int)_imageW * _imageH,
                    [&](unsigned int i) {
                      _pixelBuffer[4 * i + 0] = toInt(_cpu_output[i].s[0]);
                      _pixelBuffer[4 * i + 1] = toInt(_cpu_output[i].s[1]);
                      _pixelBuffer[4 * i + 2] = toInt(_cpu_output[i].s[2]);
                      _pixelBuffer[4 * i + 3] = 255;
                    });

  QImage image(_pixelBuffer, _imageW, _imageH, QImage::Format::Format_RGBA8888);
  pixmap.convertFromImage(image);

  if (_imgIdx < 1) {
    std::string fileName =
        "image_raytracing_" + std::to_string(_imgIdx) + ".png";
    image.save(fileName.c_str(), "PNG");
  }

  _imgIdx++;
}