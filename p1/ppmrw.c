// CS 430
// Project 1
// Xiangzhi Cao

// Header
#include <stdio.h>
#include <stdlib.h>

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

// the buffer is used to store image data and some header data from ppm file
// so that I could use the data from buffer when we write into output file
PPMimage *buffer;

PPMimage PPMRead(char *inputFilename);
int PPMWrite(char *outPPMVersion, char *outputFilename);
int PPMDataWrite(char ppmVersionNum, FILE *outputFile);

// the PPMRead function is used to read data from ppm file
PPMimage PPMRead(char *inputFilename) {
        // allocate the memory for buffer
	buffer = (PPMimage*)malloc(sizeof(PPMimage));
	FILE* fh = fopen(inputFilename, "rb");
        // check whether the file open successfully. If not, show the error message
	if (fh == NULL) {
		fprintf(stderr, "Error: open the file unsuccessfully. \n");
		exit(1);
	}
	int c = fgetc(fh);            // get the first character of the file
        // check the file type, if the file does not start with the 'P'
        // then the file is not a ppm file, and show th error message
	if (c != 'P') {
		fprintf(stderr, "Error: incorrect input file formart, the file should be a PPM file. \n");
		exit(1);
	}
	c = fgetc(fh);              // get the second character of the file, which should be a number with char type
        // save the ppm version number as a char, I tried to save the ppmVersionNum as an integer
        // but it converts to the ascii value. Thus, I figured out that saving as a character would be a better idea
	char ppmVersionNum = c;
	// if ppm version number is not 3 and 6, show the error message
	if (ppmVersionNum != '3' && ppmVersionNum != '6') {
		fprintf(stderr, "Error: invalid magic number, the ppm version should be either P3 or P6. \n");
		exit(1);
	}

	// since I want to go to the next line, so I check the new line character.
	while (c != '\n') {
		c = fgetc(fh);
	}
	// once I get the new line character, get the first character of next line
	// and check if the first character is '#', skip comments if so
	c = fgetc(fh);
	while (c == '#') {
		while (c != '\n') {
			c = fgetc(fh);
		}
		c = fgetc(fh);
	}
	
	// After I skip comments, I would like to do fscanf here, but the first digit of the width
	// might not include in the width, so I want to go back to the previous character
	ungetc(c, fh);

	/* I store all the header data into buffer here, my basic idea is using fscanf to save
	width, height, and maxvalue. Also, I would like to check if the file has correct number of width, height and
	max color value. For instance, some files might havce 2 max color value, which should be considered as invalid file format */
	int wh = fscanf(fh, "%d %d", &buffer->width, &buffer->height);
	if (wh != 2) {
		fprintf(stderr, "Error: the size of image has to include width and height, invalid data for image. \n");
		exit(1);
	}
	int mcv = fscanf(fh, "%d", &buffer->maxColorValue);
	if (mcv != 1) {
		fprintf(stderr, "Error: the max color value has to be one single value. \n");
		exit(1);
	}
	if (buffer->maxColorValue != 255) {
		fprintf(stderr, "Error: the image has to be 8-bit per channel. \n");
		exit(1);
	}

	/* I did buffer->width * buffer->height * sizeof(PPMRGBpixel), so that each (buffer->width*buffer->height) number
	of pixel memory will be allocated. Also, each pixel includes r, g, b. The reason I did it is that it makes me easier to read 
	every single element of body data */
	buffer->data = (unsigned char*)malloc(buffer->width*buffer->height*sizeof(PPMRGBpixel));
	if (buffer == NULL) {
		fprintf(stderr, "Error: allocate the memory unsuccessfully. \n");
		exit(1);
	}
	
	/* if the ppm version number is 3, then do the p3 reading. What I did is building two nested for loops so that
	the program will go through each element of body data. After that read all data into pixels in r, g, b respectively.
	Then, I could save those data into buffer->data */
	if (ppmVersionNum == '3') {
		int i, j;
		for (i = 0; i<buffer->height; i++) {
			for (j = 0; j<buffer->width; j++) {
				PPMRGBpixel *pixel = (PPMRGBpixel*)malloc(sizeof(PPMRGBpixel));
				fscanf(fh, "%d %d %d", &pixel->r, &pixel->g, &pixel->b);
				buffer->data[i*buffer->width * 3 + j * 3] = pixel->r;
				buffer->data[i*buffer->width * 3 + j * 3 + 1] = pixel->g;
				buffer->data[i*buffer->width * 3 + j * 3 + 2] = pixel->b;
			}
		}
	}
	// p6 reading, basically just save the whole body data into buffer->data directly
	else if (ppmVersionNum == '6') {
		// read the pixel data, the type size_t might be used
		size_t s = fread(buffer->data, sizeof(PPMRGBpixel), buffer->width*buffer->height, fh);
		if (s != buffer->width*buffer->height) {
			fprintf(stderr, "Error: read size and real size are not match");
			exit(1);
		}
	}
	else {
		fprintf(stderr, "Error: the ppm version cannot be read. \n");
		exit(1);
	}
	fclose(fh);
	// Since the function expects a PPMimage return value, so I return *buffer here.
	return *buffer;
}

// this function writes the header data from buffer to output file
int PPMWrite(char *outPPMVersion, char *outputFilename) {
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
	PPMDataWrite(ppmVersionNum, fh);
	fclose(fh);
}

// this function writes the body data from buffer->data to output file
int PPMDataWrite(char ppmVersionNum, FILE *outputFile) {
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
				fprintf(outputFile, "%d %d %d ", buffer->data[i*buffer->width*3+j*3], buffer->data[i*buffer->width+j*3+1], buffer->data[i*buffer->width*3+2]);
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

//
int main(int argc, char *argv[]) {
	// the project requires the output looks like 'ppmrw 6 input.ppm out.ppm', so checking the number of arguments is necessary
	if (argc != 4) {
		fprintf(stderr, "Error: the magic number, input file name and output file name are required. \n");
		return (1);
	}
	char *ppmVersion = argv[1];
	char *inputFilename = argv[2];
	char *outputFilename = argv[3];

	PPMRead(inputFilename);
	if (*ppmVersion == '6') {
		PPMWrite("P6", outputFilename);
	}
	else if (*ppmVersion == '3') {
		PPMWrite("P3", outputFilename);
	}
	else {
		fprintf(stderr, "Error: The file has to be read before the conversion \n");
		return (1);
	}
	return (0);
}


