#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#pragma pack(1)
typedef unsigned char BYTE;

struct BMP_HEADER {
    char bfType1;
    char bfType2;
    int bfSize;
    short int bfReserved1;
    short int bfReserved2;
    int bfOffBits;
};

struct BMP_INFO_HEADER {
    int biSize;
    int biWidth;
    int biHeight;
    short int biPlanes;
    short int biCount;
    int biCompression;
    int biSizeImage;
    int biXPelPerMtr;
    int biYPelPerMtr;
    int biClrUsed;
    int biCrlImportant;
};

struct ColorPal{
    BYTE rgbBlue;
    BYTE rgbRed;
    BYTE rgbGreen;
    BYTE reserved; 
};

struct Image {
    struct BMP_HEADER bmph;
    struct BMP_INFO_HEADER bmpih;
    struct ColorPal *CP;
    BYTE *data;
};

int imageSize(struct Image *image) {
    int size = 0, length;
    size += sizeof(struct BMP_HEADER) + sizeof(struct BMP_INFO_HEADER);
    if (image -> bmpih.biCount <= 8) size += sizeof(struct ColorPal) * (int) pow(2, image -> bmpih.biCount);
    length = image -> bmpih.biHeight * ((image -> bmpih.biWidth * image -> bmpih.biCount + 31) / 32 * 4);
    size += length;
    return size;
}

void readImage(char *fname, struct Image *image) {
    FILE *fp;
    int dataSize, palSize = 0;
    fp = fopen(fname, "rb");
    if (fp == NULL) {
        printf("Image not found");
        exit(1);
    }
    fread(&image -> bmph, sizeof(struct BMP_HEADER), 1, fp);
    fread(&image -> bmpih, sizeof(struct BMP_INFO_HEADER), 1, fp);
    if (image -> bmpih.biCount == 1) palSize = 4;
    if (image -> bmpih.biCount == 4) palSize = 64;
    if (image -> bmpih.biCount == 8) palSize = 1024;
    if (palSize != 0) {
        image -> CP = (struct ColorPal *) malloc(palSize);
        fread(image -> CP, palSize, 1, fp);
    }
    dataSize = image -> bmpih.biHeight * (image -> bmpih.biWidth * image -> bmpih.biCount + 31) / 32 * 4;
    image -> data = (BYTE *) malloc(dataSize);
    fread(image -> data, dataSize, 1, fp);
    return;
}

void writeImage(char *fname, struct Image *image) {
    int dataSize, palSize = 0;
    char ch;
    FILE *fp;
    fp = fopen(fname, "rb");
    if (fp != NULL) {
        printf("Overwrite (y/n : )");
        ch = getchar();
        fclose(fp);
        if (ch == 'n' || ch == 'N') return;
    }
    fp = fopen(fname, "wb");
    fwrite(&image -> bmph, sizeof(struct BMP_HEADER), 1, fp);
    fwrite(&image -> bmpih, sizeof(struct BMP_INFO_HEADER), 1, fp);
    if (image -> bmpih.biCount == 24) palSize = 0;
    else palSize = pow(2, image -> bmpih.biCount) * 4;
    if (palSize != 0) fwrite(image -> CP, palSize, 1, fp);
    dataSize = image -> bmpih.biHeight * (image -> bmpih.biCount * image -> bmpih.biWidth + 31) / 32 * 4;
    fwrite(image -> data, dataSize, 1, fp);
    fclose(fp);
    return;

}

void negativeImage(struct Image *image) {
    int size, i, j, h = image -> bmpih.biHeight; 
    size = (image -> bmpih.biCount * image -> bmpih.biWidth + 31) / 32 * 4;
    for (i = 0; i < h; i++) {
        for (j = 0; j < size; j++) {
            image -> data[i*size + j] = 255 - image -> data[i*size + j];
        }
    }
}

void powerTrans(struct Image *image, double gama) {
    int size, i, j, h = image -> bmpih.biHeight;
    size = (image -> bmpih.biCount * image -> bmpih.biWidth + 31) / 32 * 4;
    double t;
    for (i = 0; i < h; i++) {
        for (j = 0; j < size; j++) {
            t = image -> data[i*size + j] / 255.;
            t = pow(t, gama) * 255;
            image -> data[i*size + j] = (BYTE) t;
        }
    }
}

void threshImage(struct Image *image, int thres) {
    int size, i, j, h = image -> bmpih.biHeight;
    size = (image -> bmpih.biCount * image -> bmpih.biWidth + 31) / 32 * 4;
    for (i = 0; i < h; i++) {
        for (j = 0; j < size; j++) {
            if(image -> data[i*size + j] > thres) image -> data[size*i + j] = 255;
            else image -> data[i*size + j] = 0;
        }
    }
}

void logTrans(struct Image *image) {
    int size, i, j, h = image -> bmpih.biHeight;
    size = (image -> bmpih.biCount * image -> bmpih.biWidth + 31) / 32 * 4;
    double t;
    for (i = 0; i < h; i++) {
        for (j = 0; j < size; j++) {
            t = image -> data[i*size + j] / 255.;
            t = log(1+t) * 255;
            image -> data[i*size + j] = (BYTE) t;
        }
    }
}


// contrast and image streching class 
BYTE contrastImage(struct Image *image) {
    int i, size, h = image -> bmpih.biHeight;
    size = (image -> bmpih.biCount * image -> bmpih.biWidth + 31) / 32 * 4;
    BYTE max, min;
    min = max = image -> data[0];
    for (i=1; i<size * h; i++) {
        if(image -> data[i] < min) min = image -> data[i];
        if(image -> data[i] > max) max = image -> data[i];
    }
    return max - min; 
}

void contImage(struct Image *image, BYTE *min, BYTE *max) {
    int i, size, h = image -> bmpih.biHeight;
    size = (image -> bmpih.biCount * image -> bmpih.biWidth + 31) / 32 * 4;
    *min = *max = image -> data[0];
    for (i=1; i<size * h; i++) {
        if(image -> data[i] < *min) *min = image -> data[i];
        if(image -> data[i] > *max) *max = image -> data[i];
    }
}

void increaseContrast(struct Image *image) { 
    int i, size, h = image -> bmpih.biHeight;
    size = (image -> bmpih.biCount * image -> bmpih.biWidth + 31) / 32 * 4;
    BYTE min, max;
    contImage(image, &min, &max);
    printf("max: %d", max);
    printf("min: %d", min);
    printf("contrast is %d", max - min);
    for (i = 0; i < size * h; i++) {
        
        image -> data[i] = 
            (pow(2, image -> bmpih.biCount) - 1) * (image -> data[i] - min) / (max - min); 
    }
}

void imageStreching(struct Image *image) {
 
    int i, size, h = image -> bmpih.biHeight;
    size = (image -> bmpih.biCount * image -> bmpih.biWidth + 31) / 32 * 4;
    BYTE min, max;
    int MAX, MIN;
    contImage(image, &min, &max);
    printf("Enter: ");
    scanf("%d", &MIN);
    scanf("%d", &MAX);
    for (i = 0; i < size * h; i++) { 
        image -> data[i] = (MAX - MIN) * (image -> data[i] - min) / (max - min) + MIN; 
    }
}
int main() {
    struct Image *image;
    image = (struct Image *) malloc(sizeof(struct Image));
    if (image == NULL) exit(1);
//    readImage("biber.bmp", image);
//    logTrans(image);
//    writeImage("log.bmp", image);
   readImage("biber.bmp", image);
   imageStreching(image);
   writeImage("imageStrech.bmp", image);
   free(image);
   
}



