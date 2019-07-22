#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "parser.c"


// create a stuct that represents a single pixel, same as what we did in class
typedef struct PPMRGBpixel {
	unsigned char r;
	unsigned char g;
	unsigned char b;
} PPMRGBpixel;

// create struct that represents a single image
typedef struct PPMimage {
	int width;
	int height;
	int maxColorValue;
	unsigned char *data;
} PPMimage;


// return the square value of v
static inline double sqr(double v) {
  	return v*v;
}

// normalize the vector to a 3d unit vector
static inline void normalize(double* v) {
 	double len = sqrt(sqr(v[0]) + sqr(v[1]) + sqr(v[2]));
	v[0] /= len;
	v[1] /= len;
  	v[2] /= len;
}


// this function writes the body data from buffer->data to output file
int PPMDataWrite(char ppmVersionNum, FILE *outputFile, PPMimage* buffer) {
	// write image data to the file if the ppm version is P6
	if (ppmVersionNum == '6') {
		// using fwrite to write data, basically it just like copy and paste data for P6
		fwrite(buffer->data, sizeof(PPMRGBpixel), buffer->width*buffer->height, outputFile);
		printf("The file saved successfully! \n");
		return (0);
	}
	// write image data to the file if the ppm version is p3
	else if (ppmVersionNum == '3') {
		int i, j;
		for (i = 0; i < buffer->height; i++) {
			for (j = 0; j < buffer->width; j++) {
				// similar thing as we did in reading body data for P3, but we use fprintf here to write data.
				fprintf(outputFile, "%d %d %d ", buffer->data[i*buffer->width*3+j*3], buffer->data[i*buffer->width*3+j*3+1], buffer->data[i*buffer->width*3+j*3+2]);
			}
			fprintf(outputFile, "\n");
		}

		printf("The file saved successfully! \n");
		return (0);
	}
	else {
		fprintf(stderr, "Error: incorrect ppm version. \n");
		return (1);
	}
}

// this function writes the header data from buffer to output file
int PPMWrite(char *outPPMVersion, char *outputFilename, PPMimage* buffer) {
	int width = buffer->width;
	int height = buffer->height;
	int maxColorValue = buffer->maxColorValue;
	char ppmVersionNum = outPPMVersion[1];
	FILE *fh = fopen(outputFilename, "wb");
	if (fh == NULL) {
		fprintf(stderr, "Error: open the file unscuccessfully. \n");
		return (1);
	}
	char *comment = "# output.ppm";
	fprintf(fh, "P%c\n%s\n%d %d\n%d\n", ppmVersionNum, comment, width, height, 255);
	// call the PPMDataWrite function which writes the body data
	PPMDataWrite(ppmVersionNum, fh, buffer);
	fclose(fh);
}


double sphereIntersection(double* Ro, double* Rd, double* Center, double r) {
  	// x = Rox + Rdx*t
  	// y = Roy + Rdy*t
  	// z = Roz + Rdz*t
  	// (x - Centerx)^2 + (y - Centery)^2 + (z - Centerz)^2 = r^2
  	//
  	// Then, (Rox + Rdx*t - Centerx)^2 + (Roy + Rdy*t - Centery)^2
  	// + (Roz + Rdz*t - Centery)^2 = r^2
  	//
  	// Then, Rox^2 + Rdx^2*t^2 + Centerx^2 + 2*Rox*Rdx*t - 2*Rox*Centerx - 2*Rdx*t*Centerx
  	// + Roy^2 + Rdy^2*t^2 + Centery^2 + 2*Roy*Rdy*t - 2*Roy*Centery - 2*Rdy*t*Centery
  	// + Roz^2 + Rdz^2*t^2 + Centerz^2 + 2*Roz*Rdz*t - 2*Roz*Centerz - 2*Rdz*t*Centerz
  	//
  	// Then, t^2(Rdx^2 + Rdy^2 + Rdz^2) +
  	// t*(2*Rox*Rdx + 2*TRoz*Rdz + 2*Roy*Rdy - 2*Rdx*Centerx - 2*Rdy*Centery - 2*Rdz*Centerz) +
  	// Rox^2 + Centerx^2 + Roy^2 + Centery^2 + Roz^2 + Centerz^2 -
  	// 2*Rox*Centerx - 2*Roy*Centery - 2*Roz*Centerz
  	// - r^2 = 0
  	double a = sqr(Rd[0]) + sqr(Rd[1]) + sqr(Rd[2]);
  	double b = 2*(Ro[0]*Rd[0] + Ro[1]*Rd[1] + Ro[2]*Rd[2] - Rd[0]*Center[0] - Rd[1]*Center[1] - Rd[2]*Center[2]);
  	double c = sqr(Ro[0]) + sqr(Ro[1]) + sqr(Ro[2]) + sqr(Center[0]) +
             sqr(Center[1]) + sqr(Center[2]) - 2*(Ro[0]*Center[0]
             + Ro[1]*Center[1] + Ro[2]*Center[2]) - sqr(r);
  	double det = sqr(b) - 4*a*c;
  	if (det < 0) return -1;
  	det = sqrt(det);
  	double t0 = (-b - det) / (2*a);
 	double t1 = (-b + det) / (2*a);
	if (t0 > 0) return t0;
 	if (t1 > 0) return t1;

  	return -1;
}

double planeIntersection(double* Ro, double* Rd, double* position, double* normal) {
  	// A(X0 + Xd * t) + B(Y0 + Yd * t) + (Z0 + Zd * t) + D = 0
  	// A(Rox + Rdx*t) + B(Roy + Rdy*t) + C(Roz + Rdz*t) + D = 0
  	// it could also be written as:
  	// A(Rox + Rdx*t - positionx) + B(Roy + Rdy*t - postiony) + C(Roz + Rdz*t - positionz) = 0
  	// t(A*Rdx + B*Rdy + C*Rdz) + (A*Rox + B*Roy + C*Roz - A*positionx - B*positiony - C*postionz) = 0
  	// t = - (A*Rox + B*Roy + C*Roz - A*positionx - B*positiony - C*positionz) / (A*Rdx + B*Rdy + C*Rdz)
  	double t = - (normal[0]*Ro[0] + normal[1]*Ro[1] + normal[2]*Ro[2] - normal[0]*position[0]
                - normal[1]*position[1] - normal[2]*position[2]) / (normal[0]*Rd[0]
                + normal[1]*Rd[1] + normal[2]*Rd[2]);

  	if (t > 0) return t;
	return -1;
}

int intersect(double* Rd, int objectNum, Object** objects) {
	int closestObjectNum = -1;
	double bestT = INFINITY;
	int i;
	double t;
	double Ro[3] = { 0, 0, 0 };
	for (i = 0; i<objectNum; i++){
		if (objects[i]->kind == 1) {
			t = sphereIntersection(Ro, Rd, objects[i]->sphere.position, objects[i]->sphere.radius);
			if (t) {
				if (t > 0 && t <= bestT) {
					bestT = t;
					closestObjectNum = i;
				}
			}
			else {
				fprintf(stderr, "Error: finding the distance unsuccessfully.\n");
				return (1);
			}
		}
		else if (objects[i]->kind == 2) {
			t = planeIntersection(Ro, Rd, objects[i]->plane.position, objects[i]->plane.normal);
			if (t) {
				if (t > 0 && t <= bestT) {
					bestT = t;
					closestObjectNum = i;
				}
			}
			else {
				fprintf(stderr, "Error: finding the distance unsuccessfully.\n");
				return (1);
			}
		}
	}
	return closestObjectNum;
}

PPMimage* rayCasting(char* filename, int w, int h, Object** objects) {
	PPMimage* buffer = (PPMimage*)malloc(sizeof(PPMimage));
	if (objects[0] == NULL) {
		fprintf(stderr, "Error: no object found");
		exit(1);
	}
	int cameraFound = 0;
	double width;
	double height;
	int i;
	for (i = 0; objects[i] != 0; i += 1) {
		if (objects[i]->kind == 0) {
			cameraFound = 1;
			width = objects[i]->camera.width;
			height = objects[i]->camera.height;
			if (width <= 0 || height <= 0) {
				fprintf(stderr, "Error: invalid size for camera");
				exit(1);
			}
		}
	}
	if (cameraFound == 0) {
		fprintf(stderr, "Error: Camera is not found");
		exit(1);
	}

	buffer->data = (unsigned char*)malloc(w*h * sizeof(PPMRGBpixel));
	PPMRGBpixel *pixel = (PPMRGBpixel*)malloc(sizeof(PPMRGBpixel));
	if (buffer->data == NULL || buffer == NULL) {
		fprintf(stderr, "Error: allocate the memory un successfully. \n");
		exit(1);
	}

	double pixwidth = width / w;
	double pixheight = height / h;
	double pointx, pointy, pointz;
	int j, k;
	for (k = 0; k<h; k++) {
		int count = (h-k-1)*w*3;
		double vy = -height / 2 + pixheight * (k + 0.5);
		for (j = 0; j<w; j++) {
			double vx = -width / 2 + pixwidth * (j + 0.5);
			double Rd[3] = { vx, vy, 1 };

			normalize(Rd);
			int intersection = intersect(Rd, i, objects);
			if (intersection>=0) {
				pixel->r = (int)((objects[intersection]->color[0]) * 255);
				pixel->g = (int)((objects[intersection]->color[1]) * 255);
				pixel->b = (int)((objects[intersection]->color[2]) * 255);
			}
			else {
				pixel->r = 0;
				pixel->g = 0;
				pixel->b = 0;
			}
			buffer->data[count++] = pixel->r;
			buffer->data[count++] = pixel->g;
			buffer->data[count++] = pixel->b;
			}
		}
	return buffer;
}


int main(int argc, char **argv) {
	if (argc != 5) {
		fprintf(stderr, "Error: incorrect format('raycast width height input.json output.ppm')");
		return (1);
	}
	char *w = argv[1];
	char *h = argv[2];
	char *inputFilename = argv[3];
	char *outputFilename = argv[4];

	Object** objects = malloc(sizeof(Object*) * 128);
	int width = atoi(w);
	int height = atoi(h);
	if (width <= 0) {
		fprintf(stderr, "Error: Invalid width input!");
		return (1);
	}
	if (height <= 0) {
		fprintf(stderr, "Error: Invalid height input!");
		return (1);
	}
	readScene(inputFilename, objects);
	PPMimage* buffer = rayCasting(inputFilename, width, height, objects);
	buffer->width = width;
	buffer->height = height;
	PPMWrite("P3", outputFilename, buffer);
	return (0);
}
