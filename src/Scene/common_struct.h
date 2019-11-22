#ifndef COMMON_STRUCT_H
#define COMMON_STRUCT_H

#include <CL/cl.hpp>

typedef struct Sphere {
  cl_float3 Centre;
  cl_float R;
  cl_float3 diff;
  cl_float spec;
  cl_float iRefr;
  cl_float light;
} Sphere;

typedef struct Camera {
  cl_float3 foyer;
  float fov;
} Camera;

#endif