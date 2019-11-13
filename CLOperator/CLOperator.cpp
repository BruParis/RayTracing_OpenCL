#include "CLOperator.h"
#include <fstream>
#include <iostream>

CLOperator::CLOperator(std::string kernel_path) {

  // Pick one platform
  cl::Platform platform = pickPlatform();
  std::cout << "\nUsing OpenCL platform: \t"
            << platform.getInfo<CL_PLATFORM_NAME>() << std::endl;

  // Pick one device
  _device = pickDevice(platform);
  std::cout << "\nUsing OpenCL device: \t" << _device.getInfo<CL_DEVICE_NAME>()
            << std::endl;
  std::cout << "\t\t\tMax compute units : "
            << _device.getInfo<CL_DEVICE_MAX_COMPUTE_UNITS>() << std::endl;
  std::cout << "\t\t\tMax work group size: "
            << _device.getInfo<CL_DEVICE_MAX_WORK_GROUP_SIZE>() << std::endl;

  // Create an OpenCL context and command queue on that device.
  _context = cl::Context(_device);
  _queue = cl::CommandQueue(_context, _device);

  // Convert the OpenCL source code to a string
  std::string source;
  std::ifstream file(kernel_path);
  if (!file) {
    std::cout << "\nNo OpenCL file found!" << std::endl
              << "Exiting..." << std::endl;
    system("PAUSE");
    exit(1);
  }
  while (!file.eof()) {
    char line[256];
    file.getline(line, 255);
    source += line;
  }

  const char *kernel_source = source.c_str();

  // Create an OpenCL program by performing runtime source compilation for the
  // chosen device
  cl::Program program = cl::Program(_context, kernel_source);
  cl_int result = program.build({_device});

  if (result)
    std::cout << "Error during compilation OpenCL code !!!\n(" << result << ") "
              << std::endl;
  if (result == CL_BUILD_PROGRAM_FAILURE)
    printErrorLog(program);

  // Create a kernel (entry point in the OpenCL source program)
  _kernel = cl::Kernel(program, "render_kernel");
}

void CLOperator::printErrorLog(const cl::Program &program) {

  // Get the error log and print to console
  std::string buildlog = program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(_device);
  std::cerr << "Build log:" << std::endl << buildlog << std::endl;

  // Print the error log to a file
  FILE *log = fopen("errorlog.txt", "w");
  fprintf(log, "%s\n", buildlog);
  std::cout << "Error log saved in 'errorlog.txt'" << std::endl;
  system("PAUSE");
  exit(1);
}

cl::Platform CLOperator::pickPlatform() {

  // Get all available OpenCL platforms (e.g. AMD OpenCL, Nvidia CUDA,
  // IntelOpenCL)
  std::vector<cl::Platform> platforms;
  cl::Platform::get(&platforms);

  std::cout << "Available OpenCL platforms : " << std::endl << std::endl;
  for (int i = 0; i < platforms.size(); i++)
    std::cout << "\t" << i + 1 << ": "
              << platforms[i].getInfo<CL_PLATFORM_NAME>() << std::endl;

  cl::Platform platform;
  if (platforms.size() == 1)
    platform = platforms[0];
  else {
    int input = 0;
    std::cout << "\nChoose an OpenCL platform: ";
    std::cin >> input;

    // handle incorrect user input
    while (input < 1 || input > platforms.size()) {

      // clear errors/bad flags on cin
      // ignores exact number of chars in cin buffer
      std::cin.clear();
      std::cin.ignore(std::cin.rdbuf()->in_avail(), '\n');
      std::cout << "No such option. Choose an OpenCL platform : ";
      std::cin >> input;
    }
    platform = platforms[input - 1];
  }

  return platform;
}

cl::Device CLOperator::pickDevice(cl::Platform &platform) {
  // Get available OpenCL devices on platform
  std::vector<cl::Device> devices;
  platform.getDevices(CL_DEVICE_TYPE_ALL, &devices);

  std::cout << "Available OpenCL devices on this platform: " << std::endl
            << std::endl;
  for (int i = 0; i < devices.size(); i++) {
    std::cout << "\t" << i + 1 << ": " << devices[i].getInfo<CL_DEVICE_NAME>()
              << std::endl;
    std::cout << "\t\tMax compute units: "
              << devices[i].getInfo<CL_DEVICE_MAX_COMPUTE_UNITS>() << std::endl;
    std::cout << "\t\tMax work group size: "
              << devices[i].getInfo<CL_DEVICE_MAX_WORK_GROUP_SIZE>()
              << std::endl
              << std::endl;
  }

  cl::Device device;
  if (devices.size() == 1)
    device = devices[0];
  else {
    int input = 0;
    std::cout << "\nChoose an OpenCL device: ";
    std::cin >> input;

    // handle incorrect user input
    while (input < 1 || input > devices.size()) {
      // clear errors/bad flags on cin
      // ignores exact number of chars in cin buffer
      std::cin.clear();
      std::cin.ignore(std::cin.rdbuf()->in_avail(), '\n');
      std::cout << "No such option. Choose an OpenCL device : ";
      std::cin >> input;
    }
    device = devices[input - 1];
  }

  return device;
}

void CLOperator::SetBufferOutput(int imageW, int imageH) {
  _imageW = imageW;
  _imageH = imageH;
  _cl_output = cl::Buffer(_context, CL_MEM_WRITE_ONLY,
                          _imageW * _imageH * sizeof(cl_float3));
  SetKernelParam(KernelParam::BUFFER_OUTPUT, &_cl_output);
  SetKernelParam(KernelParam::IMAGE_WIDTH, &_imageW);
  SetKernelParam(KernelParam::IMAGE_HEIGHT, &_imageH);

  // the total amount of work items equals the number of pixels
  _global_work_size = _imageW * _imageH;
  _local_work_size = _kernel.getWorkGroupInfo<CL_KERNEL_WORK_GROUP_SIZE>(_device);

  std::cout << "Kernel work group size: " << _local_work_size << std::endl;

  // Ensure the global work size is a multiple of local work size
  if (_global_work_size % _local_work_size != 0)
    _global_work_size =
        (_global_work_size / _local_work_size + 1) * _local_work_size;
}

void CLOperator::SetScene(const Sphere *spheres, int sphere_count) {
  cl::Buffer cl_spheres =
      cl::Buffer(_context, CL_MEM_READ_ONLY, sphere_count * sizeof(Sphere));

  // Create buffers on the OpenCL device for the image and the spheres
  WriteBuffer(cl_spheres, spheres, sphere_count * sizeof(Sphere));
  SetKernelParam(KernelParam::BUFFER_SPHERES, &cl_spheres);
  SetKernelParam(KernelParam::SPHERE_COUNT, &sphere_count);
}

void CLOperator::SetCamera(const Camera *cam) {
  SetKernelParam(KernelParam::CAM_FOYER, &cam->foyer);
  SetKernelParam(KernelParam::CAM_FOV, &cam->fov);
}

void CLOperator::WriteBuffer(const cl::Buffer &buffer, const void *data,
                             size_t size) {
  _queue.enqueueWriteBuffer(buffer, CL_TRUE, 0, size, data);
}

void CLOperator::SetKernelParam(KernelParam paramIdx, const void *data) {
  _kernel.setArg(static_cast<unsigned int>(paramIdx), data);
}

void CLOperator::ExecuteKernel() {
  std::cout << "Rendering started..." << std::endl;

  _queue.enqueueNDRangeKernel(_kernel, NULL, _global_work_size,
                              _local_work_size);
  _queue.finish();

  std::cout << "Rendering done! \nCopying output from device to host"
            << std::endl;
}

void CLOperator::ReadOutput(void *data) {
  // read and copy OpenCL output to CPU
  _queue.enqueueReadBuffer(_cl_output, CL_TRUE, 0,
                           _imageW * _imageH * sizeof(cl_float3), data);
}