## Raytracing with OpenCL
A quick ray tracer using GPU acceleration as a first use of OpenCL.
Currently working on real-time display on screen + Camera movement.

For now, the ray tracer displays only spheres, using effects such as lighting, reflexion, refraction, Anti-aliasing and BRDF.

## Dependencies
OpenCL
Qt5

## Build & Usage
Simple build using cmake, and then just launch the executable.
```
cmake .
make
./RayTracing
```
The image produced is ```image_raytracing.png```.

![first](image_raytracing.png)

