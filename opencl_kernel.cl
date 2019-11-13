__constant float PI = 3.14159f;
__constant float EPSILON_SPACE = 0.01f;
__constant float EPSILON_CALC = 0.001f;
__constant int BRDF_NUM_RAYS = 10;
__constant int USE_BRDF = 1;
__constant int MAX_BOUNCES = 3;
__constant int ANTI_ALIASING_SAMPLES = 4;
__constant int UINT16_MAX = 2 * 32767;

typedef struct Ray {
  float3 origin;
  float3 dir;
} Ray;

typedef struct Intersection {
  bool hasIntersection;
  float t;
  float3 N;
  float3 pInter;
  int objInter;
} Intersection;

typedef struct Sphere {
  float3 Centre;
  float R;
  float3 diff;
  float spec;
  float iRefr;
  float light;
} Sphere;

typedef struct Camera {
  float3 foyer;
  float fov;
} Camera;

unsigned int get_random(unsigned int *seed) {
  *seed = (*seed * 0x5DEECE66DL + 0xBL) & ((1L << 48) - 1);
  unsigned int result = *seed >> 16;
  return result;
}

float get_uniform(unsigned int *seed) {
  unsigned int random = get_random(seed);
  float result = 2.0f * ((float) random / UINT16_MAX) - 1.0f;
  return result;
}

Intersection intersect(const Sphere *sph, Ray *r) {

  Intersection result;

  float3 centreToRay = r->origin - sph->Centre;

  float a = 1.0f;
  float b = 2.0f * dot(r->dir, centreToRay);
  float c = dot(centreToRay, centreToRay) - sph->R * sph->R;
  float discr = b * b - 4.0f * a * c;

  if (discr < 0.0f)
    result.hasIntersection = false;
  else {
    float t1 = (-b - sqrt(discr)) / (2.0f * a);
    float t2 = (-b + sqrt(discr)) / (2.0f * a);

    if (t2 < 0.0f) /* two intersections behind the cam */
      result.hasIntersection = false;
    else {
      result.hasIntersection = true;

      if (t1 < 0.0f) /* one intersections ahead, one behind */
        result.t = t2;
      else
        result.t = t1;

      result.pInter = r->origin + (result.t * r->dir);
      result.N = normalize(result.pInter - sph->Centre);
    }
  }

  return result;
}

Intersection find_intersect(__constant Sphere *spheres, const int sphere_count, Ray *r) {
  Intersection result;

  result.t = FLT_MAX;
  result.objInter = -1;
  result.hasIntersection = false;

  for (int i = 0; i < sphere_count; i++) {

    Sphere sph = spheres[i];

    if (sph.R < 0.0f)
      continue;

    Intersection aux = intersect(&sph, r);

    if (aux.hasIntersection && (aux.t <= result.t)) {
      result.t = aux.t;
      result.objInter = i;
      result.hasIntersection = true;
      result.N = aux.N;
      result.pInter = aux.pInter;
    }
  }

  return result;
}

float3 diffuse_and_shadow(const float3 pInter, const float3 normalInter, int idObj,
                          __constant Sphere *spheres, const int sphere_count) {
  float3 diffBgr = (float3)(0.0f, 0.0f, 0.0f);

  /* For each primary light source in scene */
  for (int i = 0; i < sphere_count; i++) {
    const Sphere L = spheres[i];

    if (L.light < 0.0f)
      continue;

    float3 lightDir = pInter - L.Centre;
    float lightDist = dot(lightDir, lightDir) - max(L.R, 0.0f);
    lightDir = normalize(lightDir);

    Ray rShadow;
    rShadow.origin = pInter + EPSILON_SPACE * normalInter;
    rShadow.dir = -lightDir;

    Intersection itShadow = find_intersect(spheres, sphere_count, &rShadow);

    if (itShadow.objInter < 0)
      continue;
    
    const Sphere sphShadow = spheres[itShadow.objInter];
    float shadowDist = itShadow.t * itShadow.t;

    bool objBeforeLight = shadowDist < lightDist; 
    bool lighting = itShadow.objInter == i
      || (L.R <= 0.0f && !objBeforeLight);
    bool objShadowRefr = objBeforeLight && sphShadow.iRefr > 1.0f;

    if (lighting || objShadowRefr) {

      float li = L.light;
      float intensity =
          max(0.0f, li * (0.5f / (2.0f * PI)) * dot(normalInter, -lightDir) / lightDist);

      diffBgr += intensity * spheres[idObj].diff * L.diff;

      if (objShadowRefr) {
        float compTan = -dot(itShadow.N, rShadow.dir);
        diffBgr *= compTan;
      }
    }
  }

  return diffBgr;
}

Ray *refraction(const Sphere *sph, Ray *r, const Intersection *it) {
  float3 normal = it->N;
  float rappRef = 1.0f / (sph->iRefr);
  float compTan = dot(r->dir, normal);
  float rootTerm = 1.0f - rappRef * rappRef * (1.0f - compTan * compTan);

  if (rootTerm > 0.0f) {
    r->origin = it->pInter - EPSILON_SPACE * normal;
    r->dir = (rappRef * r->dir) - (rappRef * compTan + sqrt(rootTerm)) * normal;
    Intersection internalIt = intersect(sph, r); /* internal intersection */
    normal = -internalIt.N;
    rappRef = 1.0f / rappRef;
    compTan = dot(r->dir, normal);

    /* > 0 inside the sphere */
    float rootTerm = 1.0f - rappRef * rappRef * (1.0f - compTan * compTan);
    r->origin = internalIt.pInter - EPSILON_SPACE * normal;
    r->dir = (rappRef * r->dir) - (rappRef * compTan + sqrt(rootTerm)) * normal;
  } else /* total reflection */
  {
    r->origin = it->pInter + EPSILON_SPACE * normal;
    r->dir = r->dir - 2.0f * compTan * normal;
  }

  return r;
}

float3 brdf_ray(float3 normal, unsigned int* rand_seed) {
  float r1 = get_uniform(rand_seed);
  float r2 = get_uniform(rand_seed);

  /*new random direction for indirect -diffusive- light, 
    relative to BRDF distrib, in local coord. system*/
  float3 localDir = (float3)(cos(2 * PI * r1) * sqrt(1 - r2),
                             cos(2 * PI * r1) * sqrt(1 - r2), sqrt(r2));

  /* convert into global coordinate, with normal as one of its axes*/
  float3 randomvec = (float3)(get_uniform(rand_seed),
                              get_uniform(rand_seed),
                              get_uniform(rand_seed));
  float3 tangent1 = normalize(cross(normal, randomvec));
  float3 tangent2 = cross(tangent1, normal);

  float3 globalDir = localDir[0] * tangent1 + localDir[1] * tangent2
    + localDir[2] * normal;

  return globalDir;
}

float3 diffusive_over_brdf(float3 pointInter, float3 normalInter, int idObj,
                          __constant Sphere* spheres, int sphere_count,
                          unsigned int* rand_seed) {
  float3 result = (float3)(0.0f, 0.0f, .0f);

  for (int i = 0; i < BRDF_NUM_RAYS; i++) {
    float3 brdfDir = brdf_ray(normalInter, rand_seed);
    /*float cosTheta = dot(brdfDir,Norm);*/
    float3 light = diffuse_and_shadow(pointInter, brdfDir, idObj, 
                                      spheres, sphere_count);
    result += light / BRDF_NUM_RAYS;
  }

  return result;
}

float3 trace(__constant Sphere *spheres, const int sphere_count, Ray *r,
             unsigned int* rand_seed) {
  float3 colBgr = (float3)(0.0f, 0.0f, 0.0f);
  float3 mask = (float3)(1.0f, 1.0f, 1.0f);

  for (int bounce = 0; bounce < MAX_BOUNCES; bounce++) {

    Intersection it = find_intersect(spheres, sphere_count, r);
    int idObj = it.objInter;

    if (idObj < 0) /* inter id < 0 -> No intersections */
      return colBgr;

    float light = spheres[idObj].light;
    float3 diff = spheres[idObj].diff;

    if (light > 0.0f) /* Case when object is primary light source */
    {
      colBgr += mask * diff;
      return colBgr;
    }

    float3 pointInter = it.pInter;
    float3 normalInter = it.N;
    float spec = spheres[idObj].spec;

    /* If reflexive material -> new ray from bounce on intersection */
    if (spec > 0.0f) {
      r->dir = r->dir - 2.0f * dot(r->dir, normalInter) * normalInter;
      r->origin = pointInter + EPSILON_SPACE * normalInter;
      mask *= spec * diff;
      continue;
    }
    if (spheres[idObj].iRefr >= 1.0f) /* Transparency, refraction effects */
    {
      Sphere refrSph = spheres[idObj]; 
      r = refraction(&refrSph, r, &it);
      continue;
    }

    float3 lightReceived;
    if (USE_BRDF)
      lightReceived = diffusive_over_brdf(pointInter, normalInter, idObj, spheres,
                                          sphere_count, rand_seed);
    else
      lightReceived = diffuse_and_shadow(pointInter, normalInter, idObj, spheres,
                                sphere_count);

    /* if weak or no light, no need to follow ray anymore */
    if (lightReceived[0] < EPSILON_CALC && lightReceived[1] < EPSILON_CALC
        && lightReceived[2] < EPSILON_CALC)
      break;

    mask *= (1.0f - spec) * lightReceived;

    colBgr += mask * diff;
  }

  return colBgr;
}

static float3 generate_ray(unsigned int i, unsigned int j, unsigned int *rand_seed,
  float depth, float width, float height) {

  /* Gaussian Sampling with Box-Muller method */
  float3 mainDir = (float3)(j - width / 2.0f, i - height / 2.0f, -depth);
  float3 up = (float3)(0.0f, 1.0f, 0.0f);
  float3 right = cross(normalize(mainDir), up);

  /* NEED : implement random generator for kernel */
  float rand_x = get_uniform(rand_seed);
  float rand_y = get_uniform(rand_seed);
  float s = rand_x * rand_x + rand_y * rand_y;

  while (s == 0 || s >= 1.0f) {
    int random_0 = get_random(rand_seed);
    int random_1 = get_random(rand_seed);

    rand_x = 2.0f * ((float) random_0 / UINT16_MAX) - 1.0f;
    rand_y = 2.0f * ((float) random_1 / UINT16_MAX) - 1.0f;
    s =  rand_x * rand_x + rand_y * rand_y;
  }

  float R = sqrt(-2.0f * log(s));
  float rand_u = R * cos(2.0f * PI * rand_x) * 0.5f;
  float rand_v = R * sin(2.0f * PI * rand_y) * 0.5f;
  /*printf("u: %f\n", rand_u);*/

  float pixRight = (j - width / 2.0f + rand_u - 0.5f);
  float pixUp = (i - height / 2.0f + rand_v - 0.5f);
  float3 dir = pixRight * right + pixUp * up + mainDir;
  
  dir = normalize(dir);

  return dir;
}

__kernel void render_kernel(__constant Sphere *spheres, const int sphere_count, const float3 foyer, const float fov, const int width,
                            const int height, __global float3 *output) {
  
  unsigned int work_item_id = get_global_id(0);
  unsigned int j = work_item_id % width; /* x-coordinate of the pixel */
  unsigned int i = work_item_id / width; /* y-coordinate of the pixel */

  float depth = (width * 0.5f) / (tan(fov * 0.5f));

  float3 finalcolor = (float3)(0.0f, 0.0f, 0.0f);

  unsigned int random_seed = work_item_id;
  for (int spl = 0; spl < ANTI_ALIASING_SAMPLES; spl++) {

    /* ANTI-ALIASING -> Gaussian sampling around pixel */
    float3 dir = generate_ray(i, j, &random_seed, depth, width, height);
    Ray r;
    r.origin = foyer;
    r.dir = dir;

    float3 col = trace(spheres, sphere_count, &r, &random_seed);

    finalcolor[0] += min(pow(col[0], (1.0f/2.2f)), 1.0f);
    finalcolor[1] += min(pow(col[1], (1.0f/2.2f)), 1.0f);
    finalcolor[2] += min(pow(col[2], (1.0f/2.2f)), 1.0f);
  }

  finalcolor = (1.0f / ANTI_ALIASING_SAMPLES) * finalcolor;

  output[work_item_id] = finalcolor;
}