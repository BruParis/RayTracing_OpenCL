#ifndef CL_OPERATOR_H
#define CL_OPERATOR_H

#include "../Scene/common_struct.h"
#include "../Scene/Scene.h"
#include <CL/cl.hpp>

enum class KernelParam : unsigned int {
  BUFFER_SPHERES = 0,
  SPHERE_COUNT,
  CAM_FOYER,
  CAM_FOV,
  IMAGE_WIDTH,
  IMAGE_HEIGHT,
  BUFFER_OUTPUT
};

class CLOperator {
public:
  CLOperator(std::string kernel_path);

  void SetScene(const Scene *scene);
  void SetCamera(const Camera* cam);
  void SetBufferOutput(int imageW, int imageH);
  void SetKernelParam(KernelParam paramIdx, const void* data, size_t size);
  void WriteBuffer(const cl::Buffer& buffer, const void* data, size_t size);
  void LaunchKernel();
  void ReadOutput(void* data);

private:
  int _imageW, _imageH;
  cl::Buffer _cl_output;
  cl::Kernel _kernel;
  cl::Device _device;
  cl::Context _context;
  cl::CommandQueue _queue;
  size_t _global_work_size;
  size_t _local_work_size;

  cl::Platform pickPlatform();
  cl::Device pickDevice(cl::Platform &platform);
  void printErrorLog(const cl::Program &program);
};

#endif