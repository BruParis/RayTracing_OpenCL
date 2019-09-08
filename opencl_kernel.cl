__constant float PI = 3.14159f;
__constant float EPSILON_SPACE = 0.01f;
__constant float EPSILON_CALC = 0.001f;
__constant int BRDF_NUM_RAYS = 10;
__constant int USE_BRDF = 0;
__constant int MAX_BOUNCES = 2;
__constant int ANTI_ALIASING_SAMPLES = 1;

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

typedef struct Camera {
  float3 foyer;
  float fov;
} Camera;

typedef struct Sphere {
  float3 Centre;
  float R;
  float3 diff;
  float spec;
  float iRefr;
  float light;
} Sphere;

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
    float lightDist = dot(lightDir, lightDir) - L.R;
    lightDir = normalize(lightDir);

    Ray rShadow;
    rShadow.origin = pInter + EPSILON_SPACE * normalInter;
    rShadow.dir = -lightDir;

    Intersection itShadow = find_intersect(spheres, sphere_count, &rShadow);

    if (itShadow.objInter < 0)
      continue;
    
    bool lighting = false;
    if (itShadow.objInter == i)
      lighting = true;

    float shadowDist = itShadow.t * itShadow.t;
    const Sphere sphShadow = spheres[itShadow.objInter];
    bool objShadowRefr = shadowDist < lightDist && sphShadow.iRefr > 1.0f;

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

float3 trace(__constant Sphere *spheres, const int sphere_count, Ray *r) {
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
      mask = spec * mask * diff;
      continue;
    }
    if (spheres[idObj].iRefr >= 1.0f) /* Transparency, refraction effects */
    {
      Sphere refrSph = spheres[idObj]; 
      r = refraction(&refrSph, r, &it);
      continue;
    }

    /* Ray brdfRay = USE_BRDF ? Ray(pointInter + EPSILON_SPACE * Norm , BRDF(Norm));
      float cosTheta = dot(brdfRay.dir,Norm); */
    float3 lightReceived = diffuse_and_shadow(pointInter, normalInter, idObj, spheres,
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

static float3 generate_ray(unsigned int i, unsigned int j, float depth,
                           float width, float height) {

  /* Gaussian Sampling with Box-Muller method */
  float3 mainDir = (float3)(j - width / 2.0f, i - height / 2.0f, -depth);
  float3 up = (float3)(0.0f, 1.0f, 0.0f);
  float3 right = cross(normalize(mainDir), up);

  /* NEED : implement random generator for kernel */

  float pixRight = (j - width / 2.0f - 0.5f);
  float pixUp = (i - height / 2.0f - 0.5f);
  float3 dir = pixRight * right + pixUp * up + depth * mainDir;
  
  dir = normalize(dir);

  return dir;
}

__kernel void render_kernel(__constant Sphere *spheres, const int width,
                            const int height, const int sphere_count,
                            __global float3 *output) {
  
  /* the unique global id of the work item for the current pixel */
  unsigned int work_item_id = get_global_id(0);
  unsigned int j = work_item_id % width; /* x-coordinate of the pixel */
  unsigned int i = work_item_id / width; /* y-coordinate of the pixel */

  Camera cam;
  cam.foyer = (float3)(0.0f, 70.0f, 90.0f);
  cam.fov = 90.0f * PI / 180.0f;

  /* define depth from field of view
    float D = (height / 2) / tan(cam.fov / 2); */
  float depth = height / (2.0f * tan(cam.fov * 0.5f));

  float3 finalcolor = (float3)(0.0f, 0.0f, 0.0f);

  for (int spl = 0; spl < ANTI_ALIASING_SAMPLES; spl++) {
    /* ANTI-ALIASING -> Gaussian sampling around pixel */
    float3 dir = generate_ray(i, j, depth, width, height);
    Ray r;
    r.origin = cam.foyer;
    r.dir = dir;

    float3 col = trace(spheres, sphere_count, &r);

    finalcolor += (1 / ANTI_ALIASING_SAMPLES) * col;
  }

  finalcolor[0] = min(pow(finalcolor[0], (1.0f/2.2f)), 1.0f);
  finalcolor[1] = min(pow(finalcolor[1], (1.0f/2.2f)), 1.0f);
  finalcolor[2] = min(pow(finalcolor[2], (1.0f/2.2f)), 1.0f);

  output[work_item_id] = finalcolor;
}