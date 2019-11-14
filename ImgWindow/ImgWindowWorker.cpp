#include "ImgWindowWorker.h"

using namespace cl;

cl_float4 *cpu_output;

cl_float3 operator+(const cl_float3 &lhs, const cl_float3 &rhs) {
  cl_float3 result;
  for (uint i = 0; i < 3; ++i) {
    result.s[i] = lhs.s[i] + rhs.s[i];
  }
  return result;
};

void cleanUp() { delete cpu_output; }

inline float clamp(float x) { return x < 0.0f ? 0.0f : x > 1.0f ? 1.0f : x; }

// const int imageW = 240, imageH = 160;
// const int imageW = 480, imageH = 320;
const int imageW = 720, imageH = 480;
// const int imageW = 1280, imageH = 720;
// const int imageW = 1920, imageH = 1080;

inline int toInt(float x) { return int(clamp(x) * 255 + .5); }

ImgWindowWorker::ImgWindowWorker() {

  std::string kernel_path = "opencl_kernel.cl";
  _clOperator = new CLOperator(kernel_path);

  // allocate memory on CPU for the image
  _clOperator->SetBufferOutput(imageW, imageH);
  cpu_output = new cl_float3[imageW * imageH];

  _scene = new Scene();
  _clOperator->SetScene(_scene);

  _cam = new Camera();
  _cam->foyer = {0.0f, 50.0f, 90.0f};
  _cam->fov = 60.0f * PI / 180.0f;
  _clOperator->SetCamera(_cam);
}

ImgWindowWorker::~ImgWindowWorker() {
  delete _clOperator;
  delete _scene;
  delete _cam;
}

void ImgWindowWorker::startProcess() {

  std::cout << "ImgWindowWorker START" << std::endl;

  cl_float3 vec_displ = {0.0f, 0.0f, -5.0f};
  for (int i = 0; i < 10; i++) {
    // launch the kernel
    _clOperator->LaunchKernel();
    _clOperator->ReadOutput(cpu_output);

    // save image
    saveImage(i);
    std::cout << "Saved image" << std::endl;

    _cam->foyer = _cam->foyer + vec_displ;
    _clOperator->SetCamera(_cam);
  }

  // release memory
  cleanUp();
  system("PAUSE");
  emit finished();
}

void ImgWindowWorker::saveImage(int imgIdx) {
  std::string fileName = "image_raytracing_" + std::to_string(imgIdx) + ".ppm";

  // write image to PPM file, a very simple image file format
  FILE *f = fopen(fileName.c_str(), "w");
  fprintf(f, "P3\n%d %d\n%d\n", imageW, imageH, 255);

  // loop over all pixels, write RGB values
  for (int i = 0; i < imageW * imageH; i++) {
    fprintf(f, "%d %d %d ", toInt(cpu_output[i].s[0]),
            toInt(cpu_output[i].s[1]), toInt(cpu_output[i].s[2]));
  }
}