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

void initScene(Sphere *elements) {
  // RGB Values
  // if negative R: sphere reduced to point -> example: punctual light source
  // image row (top to bottom), column(left to right) coordinates

  // reflecting sphere
  elements[0].Centre = {20.0f, 65.0f, -10.0f};
  elements[0].R = 25.0f;
  elements[0].diff = {1.0f, 1.0f, 1.0f};
  elements[0].spec = 0.85f;
  elements[0].iRefr = 0.0f;
  elements[0].light = -1.0f;

  // purple opaque sphere
  elements[1].Centre = {40.0f, -5.0f, -40.0f};
  elements[1].R = 25.0f;
  elements[1].diff = {1.0f, 0.0f, 0.7f};
  elements[1].spec = 0.0f;
  elements[1].iRefr = 0.0f;
  elements[1].light = -1.0f;

  // white sphere primary light source
  elements[2].Centre = {-25.0f, 25.0f, -30.0f};
  elements[2].R = 13.0f;
  elements[2].diff = {1.0f, 1.0f, 1.0f};
  elements[2].spec = 0.0f;
  elements[2].iRefr = 0.0f;
  elements[2].light = 6000.0f;

  // transparent ball
  elements[3].Centre = {-29.0f, 40.0f, 5.0f};
  elements[3].R = 20.0f;
  elements[3].diff = {1.0f, 1.0f, 1.0f};
  elements[3].spec = 0.0f;
  elements[3].iRefr = 1.2f;
  elements[3].light = -1.0f;

  // orange primary light source
  elements[4].Centre = {-35.0f, -35.0f, -35.0f};
  elements[4].R = -1.0f;
  elements[4].diff = {1.0f, 0.3f, 0.0f};
  elements[4].spec = 0.0f;
  elements[4].iRefr = 0.0f;
  elements[4].light = 60000.0f;

  // Wall down
  elements[5].Centre = {0.0f, 2000.0f, 0.0f};
  elements[5].R = 1900.0f;
  elements[5].diff = {1.0f, 0.05f, 0.05f};
  elements[5].spec = 0.0f;
  elements[5].iRefr = 0.0f;
  elements[5].light = -1.0f;

  // Wall up
  elements[6].Centre = {0.0f, -2000.0f, 0.0f};
  elements[6].R = 1900.0f;
  elements[6].diff = {0.05f, 1.0f, 0.05f};
  elements[6].spec = 0.0f;
  elements[6].iRefr = 0.0f;
  elements[6].light = -1.0f;

  // Wall right
  elements[7].Centre = {2000.0f, 0.0f, 0.0f};
  elements[7].R = 1900.0f;
  elements[7].diff = {0.5f, 0.5f, 0.5f};
  elements[7].spec = 0.0f;
  elements[7].iRefr = 0.0f;
  elements[7].light = -1.0f;

  // Wall left
  elements[8].Centre = {-2000.0f, 0.0f, 0.0f};
  elements[8].R = 1900.0f;
  elements[8].diff = {0.5f, 0.5f, 0.5f};
  elements[8].spec = 0.0f;
  elements[8].iRefr = 0.0f;
  elements[8].light = -1.0f;

  // Wall front
  elements[9].Centre = {0.0f, 0.0f, -2000.0f};
  elements[9].R = 1900.0f;
  elements[9].diff = {0.05f, 0.05f, 1.0f};
  elements[9].spec = 0.0f;
  elements[9].iRefr = 0.0f;
  elements[9].light = -1.0f;

  // Wall behind
  elements[10].Centre = {0.0f, 0.0f, 2000.0f};
  elements[10].R = 1900.0f;
  elements[10].diff = {1.0f, 0.4f, 0.0f};
  elements[10].spec = 0.0f;
  elements[10].iRefr = 0.0f;
  elements[10].light = -1.0f;
}

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

  int sphere_count = 11;
  Sphere spheres[sphere_count];
  initScene(spheres);
  clOperator->SetScene(spheres, sphere_count);

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
    saveImage(i);
    std::cout << "Saved image" << std::endl;

    cam->foyer = cam->foyer + vec_displ;
    clOperator->SetCamera(cam);
  }

  // release memory
  cleanUp();
  system("PAUSE");

  return 0;
}
