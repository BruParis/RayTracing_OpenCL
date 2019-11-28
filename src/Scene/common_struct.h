#ifndef COMMON_STRUCT_H
#define COMMON_STRUCT_H

#include <CL/cl.hpp>

inline float clamp(float x) { return x < 0.0f ? 0.0f : x > 1.0f ? 1.0f : x; }

inline int toInt(float x) { return int(clamp(x) * 255 + .5); }

inline cl_float3 operator+(const cl_float3 &lhs, const cl_float3 &rhs) {
  cl_float3 result;
  for (uint i = 0; i < 3; ++i) {
    result.s[i] = lhs.s[i] + rhs.s[i];
  }
  return result;
};

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