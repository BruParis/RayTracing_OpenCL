#ifndef SCENE_HPP
#define SCENE_HPP

#include "common_struct.h"
#include <vector>

class Scene {
  public:
    Scene();
    std::vector<Sphere> GetSphereVec() const { return _spheres; };

  private:
    std::vector<Sphere> _spheres;
};

#endif