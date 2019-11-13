#include <fstream>
#include <iostream>
#include <vector>

#include "CLOperator/CLOperator.h"

#define CL_HPP_TARGET_OPENCL_VERSION 200
#include <CL/cl.hpp>

#define ANTI_ALIASING_SAMPLES 1
#define PI 3.14159

using namespace cl;

cl_float4 *cpu_output;

void cleanUp() { delete cpu_output; }

inline float clamp(float x) { return x < 0.0f ? 0.0f : x > 1.0f ? 1.0f : x; }

// convert RGB float in range [0,1] to int in range [0, 255] and perform gamma
// correction
inline int toInt(float x) { return int(clamp(x) * 255 + .5); }

cl_float3 operator+(const cl_float3 &lhs, const cl_float3 &rhs) {
  cl_float3 result;
  for (uint i = 0; i < 3; ++i) {
    result.s[i] = lhs.s[i] + rhs.s[i];
  }
  return result;
};

// const int imageW = 240, imageH = 160;
// const int imageW = 480, imageH = 320;
const int imageW = 720, imageH = 480;
// const int imageW = 1280, imageH = 720;
// const int imageW = 1920, imageH = 1080;

void saveImage(int imgIdx) {
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

int main(int argc, const char *argv[]) {

  std::string kernel_path = "opencl_kernel.cl";
  CLOperator *clOperator = new CLOperator(kernel_path);

  // allocate memory on CPU for the image
  clOperator->SetBufferOutput(imageW, imageH);
  cpu_output = new cl_float3[imageW * imageH];

  const Scene* scene = new Scene();
  clOperator->SetScene(scene);

  Camera *cam = new Camera();
  cam->foyer = {0.0f, 50.0f, 90.0f};
  cam->fov = 60.0f * PI / 180.0f;
  clOperator->SetCamera(cam);

  cl_float3 vec_displ = {0.0f, 0.0f, -5.0f};
  for (int i = 0; i < 10; i++) {
    // launch the kernel
    clOperator->LaunchKernel();
    clOperator->ReadOutput(cpu_output);

    // save image
    //saveImage(i);
    std::cout << "Saved image" << std::endl;

    cam->foyer = cam->foyer + vec_displ;
    clOperator->SetCamera(cam);
  }

  // release memory
  cleanUp();
  system("PAUSE");

  return 0;
}
