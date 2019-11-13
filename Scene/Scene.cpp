#include "Scene.h"

Scene::Scene() {
  // RGB Values
  // if negative R: sphere reduced to point -> example: punctual light source
  // image row (top to bottom), column(left to right) coordinates

  // reflecting sphere
  Sphere sph;
  sph.Centre = {20.0f, 65.0f, -10.0f};
  sph.R = 25.0f;
  sph.diff = {1.0f, 1.0f, 1.0f};
  sph.spec = 0.85f;
  sph.iRefr = 0.0f;
  sph.light = -1.0f;
  _spheres.push_back(sph);

  // purple opaque sphere
  sph.Centre = {40.0f, -5.0f, -40.0f};
  sph.R = 25.0f;
  sph.diff = {1.0f, 0.0f, 0.7f};
  sph.spec = 0.0f;
  sph.iRefr = 0.0f;
  sph.light = -1.0f;
  _spheres.push_back(sph);

  // white sphere primary light source
  sph.Centre = {-25.0f, 25.0f, -30.0f};
  sph.R = 13.0f;
  sph.diff = {1.0f, 1.0f, 1.0f};
  sph.spec = 0.0f;
  sph.iRefr = 0.0f;
  sph.light = 6000.0f;
  _spheres.push_back(sph);

  // transparent ball
  sph.Centre = {-29.0f, 40.0f, 5.0f};
  sph.R = 20.0f;
  sph.diff = {1.0f, 1.0f, 1.0f};
  sph.spec = 0.0f;
  sph.iRefr = 1.2f;
  sph.light = -1.0f;
  _spheres.push_back(sph);

  // orange primary light source
  sph.Centre = {-35.0f, -35.0f, -35.0f};
  sph.R = -1.0f;
  sph.diff = {1.0f, 0.3f, 0.0f};
  sph.spec = 0.0f;
  sph.iRefr = 0.0f;
  sph.light = 60000.0f;
  _spheres.push_back(sph);

  // Wall down
  sph.Centre = {0.0f, 2000.0f, 0.0f};
  sph.R = 1900.0f;
  sph.diff = {1.0f, 0.05f, 0.05f};
  sph.spec = 0.0f;
  sph.iRefr = 0.0f;
  sph.light = -1.0f;
  _spheres.push_back(sph);

  // Wall up
  sph.Centre = {0.0f, -2000.0f, 0.0f};
  sph.R = 1900.0f;
  sph.diff = {0.05f, 1.0f, 0.05f};
  sph.spec = 0.0f;
  sph.iRefr = 0.0f;
  sph.light = -1.0f;
  _spheres.push_back(sph);

  // Wall right
  sph.Centre = {2000.0f, 0.0f, 0.0f};
  sph.R = 1900.0f;
  sph.diff = {0.5f, 0.5f, 0.5f};
  sph.spec = 0.0f;
  sph.iRefr = 0.0f;
  sph.light = -1.0f;
  _spheres.push_back(sph);

  // Wall left
  sph.Centre = {-2000.0f, 0.0f, 0.0f};
  sph.R = 1900.0f;
  sph.diff = {0.5f, 0.5f, 0.5f};
  sph.spec = 0.0f;
  sph.iRefr = 0.0f;
  sph.light = -1.0f;
  _spheres.push_back(sph);

  // Wall front
  sph.Centre = {0.0f, 0.0f, -2000.0f};
  sph.R = 1900.0f;
  sph.diff = {0.05f, 0.05f, 1.0f};
  sph.spec = 0.0f;
  sph.iRefr = 0.0f;
  sph.light = -1.0f;
  _spheres.push_back(sph);

  // Wall behind
  sph.Centre = {0.0f, 0.0f, 2000.0f};
  sph.R = 1900.0f;
  sph.diff = {1.0f, 0.4f, 0.0f};
  sph.spec = 0.0f;
  sph.iRefr = 0.0f;
  sph.light = -1.0f;
  _spheres.push_back(sph);
}