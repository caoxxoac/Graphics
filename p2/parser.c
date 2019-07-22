#include <stdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "object.h"
// the line is used to track the line number for error
int line = 1;

// nextC() wraps the getc() function and provides error checking and
// line number maintenance
int nextC(FILE* json) {
	int c = fgetc(json);
#ifdef DEBUG
	printf("nextC: '%c'\n", c);
#endif
	if (c == '\n') {
		line += 1;
	}
	if (c == EOF) {
		fprintf(stderr, "Error: Unexpected end of file on lin number %d. \n", line);
		fclose(json);
		exit(1);
	}
	return c;
}

// expectC() check that the next character is d. If it is not, it emits an error
void expectC(FILE* json, int d) {
	int c = nextC(json);
	if (c == d) return;
	fprintf(stderr, "Error: Expected '%c' on line %d.\n", d, line);
	fclose(json);
	exit(1);
}

// skipWS() skips white spaces in the file
void skipWS(FILE *json) {
	int c = nextC(json);
	while (isspace(c)) {
		c = nextC(json);
	}
	ungetc(c, json);
}

// nextString() gets the next string from the file handle and emits an error
// if a string cannot be obtained
char* nextString(FILE* json) {
	char buffer[129];
	int c = nextC(json);
	if (c != '"') {
		fprintf(stderr, "Error: Expected string on line %d.\n", line);
		fclose(json);
		exit(1);
	}
	c = nextC(json);
	int i = 0;
	while (c != '"') {
		if (i >= 128) {
			fprintf(stderr, "Error: Strings longer than 128 characters in length are not supported.\n");
			fclose(json);
			exit(1);
		}
		if (c == '\\') {
			fprintf(stderr, "Error: Strings with escape codes are not supported.\n");
			fclose(json);
			exit(1);
		}
		if (c < 32 || c > 126) {
			fprintf(stderr, "Error: Strings may contain only ascii characters.\n");
			fclose(json);
			exit(1);
		}
		buffer[i] = c;
		i += 1;
		c = nextC(json);
	}
	buffer[i] = 0;
	return strdup(buffer);
}

// get the next number if it is a number and saved as double.
// when I was doing this part, using '%f' could not save the value for fscanf
// It worked when I change the '%f' to '%lf'
double nextNumber(FILE* json) {
	double value;
	// the number of items successfully stored
	int f = fscanf(json, "%lf", &value);
	if (f != 1) {
		fprintf(stderr, "Error: floating point is expected in line number %d.\n", line);
	}
	return value;
}

// get the next vector, 3 numbers should be included in this vector.
double* nextVector(FILE* json) {
	double* v = malloc(3 * sizeof(double));
	expectC(json, '[');
	skipWS(json);
	v[0] = nextNumber(json);
	skipWS(json);
	expectC(json, ',');
	skipWS(json);
	v[1] = nextNumber(json);
	skipWS(json);
	expectC(json, ',');
	skipWS(json);
	v[2] = nextNumber(json);
	skipWS(json);
	expectC(json, ']');
	return v;
}

// I modified a little bit in this readScene() function
void readScene(char* filename, Object** objects) {
	int c;
	FILE* json = fopen(filename, "r");
	if (json == NULL) {
		fprintf(stderr, "Error: Could not open the file %s.\n", filename);
		exit(1);
	}
	skipWS(json);

	// Find the beginning of the list
	expectC(json, '[');
	skipWS(json);

	// Find the objects
	int i = 0;
	while (1) {
		Object* object = malloc(sizeof(Object));
		objects[i] = object;
		c = fgetc(json);
		if (c == ']') {
			fprintf(stderr, "Error: This is the worst scene file EVER.\n");
			fclose(json);
			exit(1);
		}
		if (c == '{') {
			skipWS(json);

			// Parse the object
			char* key = nextString(json);
			if (strcmp(key, "type") != 0) {
				fprintf(stderr, "Error: Expected \"type\" key on line number %d.\n", line);
				fclose(json);
				exit(1);
			}
			skipWS(json);

			expectC(json, ':');

			skipWS(json);

			char* value = nextString(json);
			
			// this tempKey is used to check if it is a sphere position or plane position, and sphere color or plane color
			char* tempKey = value;
			// save the kind value for the object
			if (strcmp(value, "camera") == 0) {
				objects[i]->kind = 0;
			}
			else if (strcmp(value, "sphere") == 0) {
				objects[i]->kind = 1;
			}
			else if (strcmp(value, "plane") == 0) {
				objects[i]->kind = 2;
			}
			else {
				fprintf(stderr, "Error: Unknown type, \"%s\", on line number %d.\n", value, line);
				fclose(json);
				exit(1);
			}

			skipWS(json);

			while (1) {
				// , }
				c = nextC(json);
				if (c == '}') {
					// stop parsing this object
					break;
				}
				else if (c == ',') {
					// read another field
					skipWS(json);
					char* key = nextString(json);
					skipWS(json);
					expectC(json, ':');
					skipWS(json);
					// saving the values into objects
					// if the key is width, radius, or height, using nextNumber() would be 
					// a good choice to get those values
					if ((strcmp(key, "width") == 0) || (strcmp(key, "height") == 0) ||
						(strcmp(key, "radius") == 0)) {
						double value = nextNumber(json);
						// Also, object[i]->someObject.property = value would be the solution for
						// saving value into the object.
						if (strcmp(key, "width") == 0) {
							objects[i]->camera.width = value;
						}
						else if (strcmp(key, "height") == 0) {
							objects[i]->camera.height = value;
						}
						else {
							objects[i]->sphere.radius = value;
						}
					}
					// if the key is color, position or normal, then it would be a 3d vector in the json file
					// so we would like to read that as a 3d vector, and saving the values into objects.
					else if ((strcmp(key, "color") == 0) || (strcmp(key, "position") == 0) ||
						strcmp(key, "normal") == 0) {
						double* value = nextVector(json);
						if (strcmp(key, "color") == 0) {
							objects[i]->color[0] = value[0];
							objects[i]->color[1] = value[1];
							objects[i]->color[2] = value[2];
						}
						// if the key is position, then sphere and plane would be considered respectively
						else if (strcmp(key, "position") == 0) {
							if (strcmp(tempKey, "sphere") == 0) {
								objects[i]->sphere.position[0] = value[0];
								objects[i]->sphere.position[1] = value[1];
								objects[i]->sphere.position[2] = value[2];
							}
							else if (strcmp(tempKey, "plane") == 0) {
								objects[i]->plane.position[0] = value[0];
								objects[i]->plane.position[1] = value[1];
								objects[i]->plane.position[2] = value[2];
							}
							else {
								fprintf(stderr, "Error: Unknown type!.\n");
								exit(1);
							}
						}
						else {
							objects[i]->plane.normal[0] = value[0];
							objects[i]->plane.normal[1] = value[1];
							objects[i]->plane.normal[2] = value[2];
						}
					}
					else {
						fprintf(stderr, "Error: Unkonwn property, %s, on line %d.\n", key, line);
						fclose(json);
						exit(1);
					}
					skipWS(json);
				}
				else {
					fprintf(stderr, "Error: Unexpected value on line %d\n", line);
					fclose(json);
					exit(1);
				}
			}
			skipWS(json);
			c = nextC(json);
			if (c == ',') {
				// noop
				skipWS(json);
			}
			else if (c == ']') {
				fclose(json);
				// the file does not have object anymore, so that I set the next object to NULL
				// for easy use in  the future.
				objects[i + 1] = NULL;
				return;
			}
			else {
				fprintf(stderr, "Error: Expecting ',' or ']' on line %d.\n", line);
				fclose(json);
				exit(1);
			}
		}
		// increment the index of the object, so that we can move on to the next object
		i = i + 1;
	}
}
