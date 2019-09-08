#include <fstream>
#include <iostream>
#include <vector>

#define CL_HPP_TARGET_OPENCL_VERSION 200
#include </usr/include/CL/cl.hpp>

#define ANTI_ALIASING_SAMPLES 1
#define PI 3.14159

typedef struct Sphere {
  cl_float3 Centre;
  cl_float R;
  cl_float3 diff;
  cl_float spec;
  cl_float iRefr;
  cl_float light;
} Sphere;

cl_float4 *cpu_output;
cl::CommandQueue queue;
cl::Device device;
cl::Kernel kernel;
cl::Context context;
cl::Program program;
cl::Buffer cl_output;
cl::Buffer cl_spheres;

void pickPlatform(cl::Platform &platform,
                  const std::vector<cl::Platform> &platforms) {

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
}

void pickDevice(cl::Device &device, const std::vector<cl::Device> &devices) {

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
}

void printErrorLog(const cl::Program &program, const cl::Device &device) {

  // Get the error log and print to console
  std::string buildlog = program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(device);
  std::cerr << "Build log:" << std::endl << buildlog << std::endl;

  // Print the error log to a file
  FILE *log = fopen("errorlog.txt", "w");
  fprintf(log, "%s\n", buildlog);
  std::cout << "Error log saved in 'errorlog.txt'" << std::endl;
  system("PAUSE");
  exit(1);
}

void initOpenCL() {
  // Get all available OpenCL platforms (e.g. AMD OpenCL, Nvidia CUDA,
  // IntelOpenCL)
  std::vector<cl::Platform> platforms;
  cl::Platform::get(&platforms);

  std::cout << "Available OpenCL platforms : " << std::endl << std::endl;
  for (int i = 0; i < platforms.size(); i++)
    std::cout << "\t" << i + 1 << ": "
              << platforms[i].getInfo<CL_PLATFORM_NAME>() << std::endl;

  // Pick one platform
  cl::Platform platform;
  pickPlatform(platform, platforms);
  std::cout << "\nUsing OpenCL platform: \t"
            << platform.getInfo<CL_PLATFORM_NAME>() << std::endl;

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

  // Pick one device
  pickDevice(device, devices);
  std::cout << "\nUsing OpenCL device: \t" << device.getInfo<CL_DEVICE_NAME>()
            << std::endl;
  std::cout << "\t\t\tMax compute units : "
            << device.getInfo<CL_DEVICE_MAX_COMPUTE_UNITS>() << std::endl;
  std::cout << "\t\t\tMax work group size: "
            << device.getInfo<CL_DEVICE_MAX_WORK_GROUP_SIZE>() << std::endl;

  // Create an OpenCL context and command queue on that device.
  context = cl::Context(device);
  queue = cl::CommandQueue(context, device);

  // Convert the OpenCL source code to a string
  std::string source;
  std::ifstream file("opencl_kernel.cl");
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
  program = cl::Program(context, kernel_source);
  cl_int result = program.build({device});

  if (result)
    std::cout << "Error during compilation OpenCL code !!!\n(" << result << ") "
              << std::endl;
  if (result == CL_BUILD_PROGRAM_FAILURE)
    printErrorLog(program, device);

  // Create a kernel (entry point in the OpenCL source program)
  kernel = cl::Kernel(program, "render_kernel");
}

void cleanUp() { delete cpu_output; }

inline float clamp(float x) { return x < 0.0f ? 0.0f : x > 1.0f ? 1.0f : x; }

// convert RGB float in range [0,1] to int in range [0, 255] and perform gamma
// correction
inline int toInt(float x) { return int(clamp(x) * 255 + .5); }

#define float3(x, y, z) {{x, y, z}}  // macro to replace ugly initializer braces

void initScene(Sphere *elements) {
  // RGB Values
  // if negative R: sphere reduced to point -> example: punctual light source
  // image row (top to bottom), column(left to right) coordinates

  // reflecting sphere
  elements[0].Centre = float3(20.0f, 65.0f, -10.0f);
  elements[0].R = 25.0f;
  elements[0].diff = float3(1.0f, 1.0f, 1.0f);
  elements[0].spec = 0.85f;
  elements[0].iRefr = 0.0f;
  elements[0].light = -1.0f;

  // purple opaque sphere
  elements[1].Centre = float3(40.0f, -5.0f, -40.0f);
  elements[1].R = 25.0f;
  elements[1].diff = float3(1.0f, 0.0f, 0.7f);
  elements[1].spec = 0.0f;
  elements[1].iRefr = 0.0f;
  elements[1].light = -1.0f;

  // white sphere primary light source
  elements[2].Centre = float3(-25.0f, 25.0f, -30.0f);
  elements[2].R = 13.0f;
  elements[2].diff = float3(1.0f, 1.0f, 1.0f);
  elements[2].spec = 0.0f;
  elements[2].iRefr = 0.0f;
  elements[2].light = 2000.0f;

  // transparent ball
  elements[3].Centre = float3(-29.0f, 40.0f, 5.0f);
  elements[3].R = 20.0f;
  elements[3].diff = float3(1.0f, 1.0f, 1.0f);
  elements[3].spec = 0.0f;
  elements[3].iRefr = 1.2f;
  elements[3].light = -1.0f;

  // orange primary light source
  elements[4].Centre = float3(-35.0f, -35.0f, -35.0f);
  elements[4].R = -1.0f;
  elements[4].diff = float3(1.0f, 0.3f, 0.0f);
  elements[4].spec = 0.0f;
  elements[4].iRefr = 0.0f;
  elements[4].light = 40000.0f;

  // Wall down
  elements[5].Centre = float3(0.0f, 2000.0f, 0.0f);
  elements[5].R = 1900.0f;
  elements[5].diff = float3(1.0f, 0.05f, 0.05f);
  elements[5].spec = 0.0f;
  elements[5].iRefr = 0.0f;
  elements[5].light = -1.0f;

  // Wall up
  elements[6].Centre = float3(0.0f, -2000.0f, 0.0f);
  elements[6].R = 1900.0f;
  elements[6].diff = float3(0.05f, 1.0f, 0.05f);
  elements[6].spec = 0.0f;
  elements[6].iRefr = 0.0f;
  elements[6].light = -1.0f;

  // Wall right
  elements[7].Centre = float3(2000.0f, 0.0f, 0.0f);
  elements[7].R = 1900.0f;
  elements[7].diff = float3(0.5f, 0.5f, 0.5f);
  elements[7].spec = 0.0f;
  elements[7].iRefr = 0.0f;
  elements[7].light = -1.0f;

  // Wall left
  elements[8].Centre = float3(-2000.0f, 0.0f, 0.0f);
  elements[8].R = 1900.0f;
  elements[8].diff = float3(0.5f, 0.5f, 0.5f);
  elements[8].spec = 0.0f;
  elements[8].iRefr = 0.0f;
  elements[8].light = -1.0f;

  // Wall front
  elements[9].Centre = float3(0.0f, 0.0f, -2000.0f);
  elements[9].R = 1900.0f;
  elements[9].diff = float3(0.05f, 0.05f, 1.0f);
  elements[9].spec = 0.0f;
  elements[9].iRefr = 0.0f;
  elements[9].light = -1.0f;

  // Wall behind
  elements[10].Centre = float3(0.0f, 0.0f, 2000.0f);
  elements[10].R = 1900.0f;
  elements[10].diff = float3(1.0f, 0.4f, 0.0f);
  elements[10].spec = 0.0f;
  elements[10].iRefr = 0.0f;
  elements[10].light = -1.0f;
}

// const int imageW = 240, imageH = 160;
// const int imageW = 480, imageH = 320;
// const int imageW = 720, imageH = 480;
const int imageW = 1280, imageH = 720;
// const int imageW = 1920, imageH = 1080;

void saveImage() {
  // write image to PPM file, a very simple image file format
  FILE *f = fopen("image_raytracing.ppm", "w");
  fprintf(f, "P3\n%d %d\n%d\n", imageW, imageH, 255);

  // loop over all pixels, write RGB values
  for (int i = 0; i < imageW * imageH; i++) {
    fprintf(f, "%d %d %d ", toInt(cpu_output[i].s[0]),
            toInt(cpu_output[i].s[1]), toInt(cpu_output[i].s[2]));
  }
}

int main(int argc, const char *argv[]) {

  initOpenCL();

  // allocate memory on CPU for the image
  cpu_output = new cl_float3[imageW * imageH];

  int sphere_count = 11;
  Sphere spheres[sphere_count];
  initScene(spheres);

  // Create buffers on the OpenCL device for the image and the spheres
  cl_output = cl::Buffer(context, CL_MEM_WRITE_ONLY,
                         imageW * imageH * sizeof(cl_float3));
  cl_spheres =
      cl::Buffer(context, CL_MEM_READ_ONLY, sphere_count * sizeof(Sphere));
  queue.enqueueWriteBuffer(cl_spheres, CL_TRUE, 0, sphere_count * sizeof(Sphere),
                           spheres);

  // specify OpenCL kernel arguments
  kernel.setArg(0, cl_spheres);
  kernel.setArg(1, imageW);
  kernel.setArg(2, imageH);
  kernel.setArg(3, sphere_count);
  kernel.setArg(4, cl_output);

  // the total amount of work items equals the number of pixels
  std::size_t global_work_size = imageW * imageH;
  std::size_t local_work_size =
      kernel.getWorkGroupInfo<CL_KERNEL_WORK_GROUP_SIZE>(device);

  std::cout << "Kernel work group size: " << local_work_size << std::endl;

  // Ensure the global work size is a multiple of local work size
  if (global_work_size % local_work_size != 0)
    global_work_size =
        (global_work_size / local_work_size + 1) * local_work_size;

  std::cout << "Rendering started..." << std::endl;

  // launch the kernel
  queue.enqueueNDRangeKernel(kernel, NULL, global_work_size, local_work_size);
  queue.finish();

  std::cout << "Rendering done! \nCopying output from device to host"
            << std::endl;

  // read and copy OpenCL output to CPU
  queue.enqueueReadBuffer(cl_output, CL_TRUE, 0,
                          imageW * imageH * sizeof(cl_float3), cpu_output);

  // save image
  saveImage();
  std::cout << "Saved image to 'image_raytracing.ppm'" << std::endl;

  // release memory
  cleanUp();

  system("PAUSE");

  return 0;
}