__constant float PI = 3.14159f;
__constant float EPSILON_SPACE = 0.01f;
__constant float EPSILON_CALC = 0.001f;
__constant int BRDF_NUM_RAYS = 6;
__constant int USE_BRDF = 1;
__constant int MAX_BOUNCES = 3;
__constant int ANTI_ALIASING_SAMPLES = 3;
__constant int UINT16_MAX = 2 * 32767;
__constant int MAX_CALL_RAY = 3;

typedef struct Ray {
  float3 origin;
  float3 dir;
} Ray;

typedef struct Intersection {
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

bool intersect(Intersection* inter, __constant Sphere *sph, Ray *r, bool compareDist) {

  float3 centreToRay = r->origin - sph->Centre;

  float a = 1.0f;
  float b = 2.0f * dot(r->dir, centreToRay);
  float c = dot(centreToRay, centreToRay) - sph->R * sph->R;
  float discr = b * b - 4.0f * a * c;

  if (discr < 0.0f)
    return false;
  else {
    float sqrt_discr = sqrt(discr);
    float t1 = (-b - sqrt_discr) / (2.0f * a);
    float t2 = (-b + sqrt_discr) / (2.0f * a);

    if (t2 < 0.0f) /* two intersections behind the cam */
      return false;

     /* t1 < 0.0f one intersections ahead, one behind */
    float t = t1 < 0.0f ? t2 : t1;
    if (!compareDist || t < inter->t) {

      inter->t = t;
      inter->pInter = r->origin + (inter->t * r->dir);
      inter->N = normalize(inter->pInter - sph->Centre);

      return true;
    }

    return false;
  }
}

void find_intersect(Intersection* it, __constant Sphere *spheres, const int sphere_count, Ray *r) {

  it->t = FLT_MAX;
  it->objInter = -1;

  for (int i = 0; i < sphere_count; i++) {

    if (spheres[i].R < 0.0f)
      continue;

    bool inter_success = intersect(it, &spheres[i], r, true);

    if (inter_success)
      it->objInter = i;
  }
}

float3 diffused_light_and_shadow(const float3 pInter, const float3 normalInter, int idObj,
                          __constant Sphere *spheres, const int sphere_count) {
  float3 diffBgr = (float3)(0.0f, 0.0f, 0.0f);

  /* For each primary light source in scene */
  for (int i = 0; i < sphere_count; i++) {
    const Sphere L = spheres[i];

    if (L.light < 0.0f)
      continue;

    float3 lightDir = pInter - L.Centre;
    float lightDist_2 = dot(lightDir, lightDir) - max(L.R * L.R, 0.0f);
    lightDir = normalize(lightDir);

    Ray rShadow;
    rShadow.origin = pInter + EPSILON_SPACE * normalInter;
    rShadow.dir = -lightDir;

    Intersection itShadow;
    find_intersect(&itShadow, spheres, sphere_count, &rShadow);

    int idShadow = itShadow.objInter;
    if (idShadow < 0)
      continue;
    
    float shadowDist = itShadow.t * itShadow.t;
    bool objBeforeLight = shadowDist < lightDist_2;
    bool lighting = (spheres[idShadow].light > 0) || (L.R <= 0.0f && !objBeforeLight);
    bool objShadowRefr = objBeforeLight && spheres[idShadow].iRefr > 1.0f;

    if (lighting || objShadowRefr) {

      float li = L.light;
      float intensity = max(0.0f, li * (0.5f / (2.0f * PI))
          * dot(normalInter, -lightDir) / sqrt(lightDist_2));

      diffBgr += intensity * spheres[idObj].diff * L.diff;

      if (objShadowRefr) {
        float compTan = -dot(itShadow.N, rShadow.dir);
        diffBgr *= compTan;
      }
    }
  }

  return diffBgr;
}

Ray refraction(__constant Sphere *sph, Ray *r, Intersection *it) {
  float3 normal = it->N;
  float rappRef = 1.0f / (sph->iRefr);
  float compTan = dot(r->dir, normal);
  float rootTerm = 1.0f - rappRef * rappRef * (1.0f - compTan * compTan);

  Ray r_refr = *r;
  if (rootTerm > 0.0f) {
    r_refr.origin = it->pInter - EPSILON_SPACE * normal;
    r_refr.dir = (rappRef * r_refr.dir) - (rappRef * compTan + sqrt(rootTerm)) * normal;
    r_refr.dir = normalize(r_refr.dir);
    intersect(it, sph, &r_refr, false); /* internal intersection */
    normal = -it->N;
    rappRef = 1.0f / rappRef;
    compTan = dot(r_refr.dir, normal);

    /* > 0 inside the sphere */
    float rootTerm = 1.0f - rappRef * rappRef * (1.0f - compTan * compTan);
    r_refr.origin = it->pInter - EPSILON_SPACE * normal;
    r_refr.dir = (rappRef * r_refr.dir) - (rappRef * compTan + sqrt(rootTerm)) * normal;
    r_refr.dir = normalize(r_refr.dir);
  } 
  else { 
    /* total reflection */
    r_refr.origin = it->pInter + EPSILON_SPACE * normal;
    r_refr.dir = r_refr.dir - 2.0f * compTan * normal;
    r_refr.dir = normalize(r_refr.dir);
  }

  return r_refr;
}

void brdf_ray(Ray *ray, float3 normal, unsigned int* rand_seed) {
  float r1 = fabs(get_uniform(rand_seed));
  float r2 = fabs(get_uniform(rand_seed));

  /*new random direction for indirect -diffusive- light, 
    relative to BRDF distrib, in local coord. system*/
  float sqrt_1_r2 = sqrt(1 - r2);
  float3 localDir = (float3)(cos(2 * PI * r1) * sqrt_1_r2,
                             sin(2 * PI * r1) * sqrt_1_r2, sqrt(r2));

  /* convert into global coordinate, with normal as one of its axes*/
  float3 randomvec = (float3)(get_uniform(rand_seed),
                              get_uniform(rand_seed),
                              get_uniform(rand_seed));
  float3 tangent1 = normalize(cross(normal, randomvec));
  float3 tangent2 = cross(tangent1, normal);

  float3 globalDir = localDir[0] * tangent1 + localDir[1] * tangent2
    + localDir[2] * normal;

  ray->dir = normalize(globalDir);
}

float3 diffused_light_brdf(float3 pointInter, float3 normalInter, int idObj,
                          __constant Sphere* spheres, int sphere_count,
                          unsigned int* rand_seed) {
  float3 result = (float3)(0.0f, 0.0f, .0f);

  Ray r_brdf;
  r_brdf.origin = pointInter + EPSILON_SPACE * normalInter;
  Intersection it_brdf;
  for (int i = 0; i < BRDF_NUM_RAYS; i++) {

    brdf_ray(&r_brdf, normalInter, rand_seed);
    find_intersect(&it_brdf, spheres, sphere_count, &r_brdf);

     /* inter id < 0 -> No intersections */
    int idObj_Brdf = it_brdf.objInter;
    if (idObj_Brdf < 0 && idObj_Brdf != idObj)
      continue;

    const Sphere brdfObj = spheres[idObj_Brdf];
    float light_brdf = brdfObj.light;
    float3 pInter_brdf = it_brdf.pInter;
    float3 normal_brdf = it_brdf.N;

    if (light_brdf > 0.0f) {
      float3 brdf_ObjDir = pointInter - brdfObj.Centre;
      float brdfObj_Dist_2 = dot(brdf_ObjDir, brdf_ObjDir)- max(brdfObj.R * brdfObj.R, 0.0f);
      brdf_ObjDir = normalize(brdf_ObjDir);

      float light_intensity = max(0.0f, light_brdf * (0.5f / (2.0f * PI)) *
                                 dot(normalInter, -brdf_ObjDir) / sqrt(brdfObj_Dist_2));
      result += light_intensity * spheres[idObj].diff * brdfObj.diff;
    }
    else {
      /* float cosTheta = dot(r_brdf.dir, normalInter); */
      float3 brdf_mask = diffused_light_and_shadow(pInter_brdf, normal_brdf, idObj_Brdf,
                        spheres, sphere_count);
      result += (1.0f - spheres[idObj].spec) * brdf_mask * spheres[idObj].diff;
    }
  }

  result *= PI / BRDF_NUM_RAYS;

  return result;
}

float3 trace(__constant Sphere *spheres, const int sphere_count, Ray *r,
             unsigned int* rand_seed) {
  float3 gen_colBgr = (float3)(0.0f, 0.0f, 0.0f);

  int call_count = 0;
  Intersection it;
  Ray next_ray;
  bool newRay = false;

  while (call_count < MAX_CALL_RAY) {

    float3 colBgr = (float3)(0.0f, 0.0f, 0.0f);
    float3 mask = (float3)(1.0f, 1.0f, 1.0f);

    if (newRay) {
      *r = next_ray;
      newRay = false;
    }
    else if (call_count > 0)
      break;

    call_count++;
    for (int bounce = 0; bounce < MAX_BOUNCES; bounce++) {

      find_intersect(&it, spheres, sphere_count, r);

      int idObj = it.objInter;
      if (idObj < 0) /* inter id < 0 -> No intersections */
        break;

      float light = spheres[idObj].light;
      float3 diff = spheres[idObj].diff;

      if (light > 0.0f) /* Case when object is primary light source */
      {
        colBgr += mask * diff;
        break;
      }

      float3 pointInter = it.pInter;
      float3 normalInter = it.N;
      float spec = spheres[idObj].spec;

      /* transparency -> refraction effect - counts as 1 bounce */
      if (spheres[idObj].iRefr >= 1.0f)
      {
        next_ray = refraction(&spheres[idObj], r, &it);
        newRay = true;
        /* *r = r_refr; */
        /* continue; */
      }

      /* reflexive material -> new ray from bounce on intersection */
      if (spec > 0.0f) {
        r->dir = r->dir - 2.0f * dot(r->dir, normalInter) * normalInter;
        r->origin = pointInter + EPSILON_SPACE * normalInter;
        mask *= spec * diff;
        continue;
      }

      float3 lightReceived = USE_BRDF ?
                  diffused_light_brdf(pointInter, normalInter, idObj, spheres,
                                      sphere_count, rand_seed) :
                  diffused_light_and_shadow(pointInter, normalInter, idObj, spheres,
                                        sphere_count);

      /* if weak or no light, no need to follow ray anymore */
      /* if (lightReceived[0] < EPSILON_CALC && lightReceived[1] < EPSILON_CALC
          && lightReceived[2] < EPSILON_CALC)
        break; */

      mask *= (1.0f - spec) * lightReceived;

      colBgr += mask * diff;
      break;
    }

    gen_colBgr += colBgr;
  }

  return gen_colBgr;
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

  float pixRight = (j - width / 2.0f + rand_u - 0.5f);
  float pixUp = (i - height / 2.0f + rand_v - 0.5f);
  float3 dir = pixRight * right + pixUp * up + mainDir;
  
  dir = normalize(dir);

  return dir;
}

__kernel void render_kernel(__constant Sphere *spheres, const int sphere_count, 
                            const float3 foyer, const float fov, const int width,
                            const int height, unsigned int random_seed,
                            __global float3 *output) {

  unsigned int work_item_id = get_global_id(0);
  unsigned int j = work_item_id % width; /* x-coordinate of the pixel */
  unsigned int i = work_item_id / width; /* y-coordinate of the pixel */

  float depth = (width * 0.5f) / (tan(fov * 0.5f));

  float3 finalcolor = (float3)(0.0f, 0.0f, 0.0f);

  random_seed += work_item_id;
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