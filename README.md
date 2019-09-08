## Raytracing with OpenCL
A quick ray tracey using GPU acceleration as a first use of OpenCL. OpenCL init following tutorials at http://raytracey.blogspot.com/.


For now, the ray tracer involves displays only spheres, using effects such as lighting, reflexion, refraction. Other features such as Anti-aliasing and BRDF are present in the code, but not effective. 

## Dependencies
OpenCL is obviously needed. Library file directive in the code is 
```#include </usr/include/CL/cl.hpp>```

## Build & Usage
Simple build using cmake, and then just launch the executable.
```
cmake .
make
./RayTracing
```
The image produced is ```image_raytracing.ppm```.
