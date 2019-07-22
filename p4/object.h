typedef struct {
  int kind; // 0 = camera, 1 = sphere, 2 = plane, 3 = light
  union {
    struct {
      double width;
      double height;
    } camera;
    struct {
      double position[3];
      double radius;
      double diffuseColor[3];
      double specularColor[3];
      double reflectivity;
      double refractivity;
      double ior;
    } sphere;
    struct {
      double position[3];
      double normal[3];
      double diffuseColor[3];
      double specularColor[3];
      double reflectivity;
      double refractivity;
      double ior;
    } plane;
    struct {
      double position[3];
      double direction[3];
      double color[3];
      double theta;
      double radialA0;
      double radialA1;
      double radialA2;
      double angularA0;
      double ns;
    } light;
  };
} Object;
