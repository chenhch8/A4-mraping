#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <iostream>

#define ffabs(x) ( (x) >= 0 ? (x) : -(x) ) 
#define GAUSSIAN_CUT_OFF 0.005f
#define MAGNITUDE_SCALE 100.0f
#define MAGNITUDE_LIMIT 1000.0f
#define MAGNITUDE_MAX ((int) (MAGNITUDE_SCALE * MAGNITUDE_LIMIT))

typedef struct {
    unsigned char *data; /* input image */
    int width;           
    int height;
    int *idata;          /* output for edges */
    int *magnitude;      /* edge magnitude as detected by Gaussians */
    float *xConv;        /* temporary for convolution in x direction */
    float *yConv;        /* temporary for convolution in y direction */
    float *xGradient;    /* gradients in x direction, as detected by Gaussians */
    float *yGradient;    /* gradients in x direction,a s detected by Gaussians */
} CANNY;

static CANNY *allocatebuffers(unsigned char *grey, int width, int height);
static void killbuffers(CANNY *can);
static int computeGradients(CANNY *can, float kernelRadius, int kernelWidth);
static void performHysteresis(CANNY *can, int low, int high);
static void follow(CANNY *can, int x1, int y1, int i1, int threshold);

static void normalizeContrast(unsigned char *data, int width, int height);
static  float hypotenuse(float x, float y);
static float gaussian(float x, float sigma);


unsigned char * cannyparam(unsigned char *grey, int width, int height, 
                          float lowthreshold, float highthreshold, 
                          float gaussiankernelradius, int gaussiankernelwidth,  
                          int contrastnormalised) {
    CANNY *can = 0;
    unsigned char *answer = 0;
    int low, high;
    int err;
    int i;

    answer = (unsigned char *)malloc(width * height);
    if(!answer)
        goto error_exit;
    can = allocatebuffers(grey, width, height);
    if(!can)
        goto error_exit;
    if (contrastnormalised) 
        normalizeContrast(can->data, width, height);

    err = computeGradients(can, gaussiankernelradius, gaussiankernelwidth);
    if(err < 0)
      goto error_exit;
    low = (int) (lowthreshold * MAGNITUDE_SCALE + 0.5f);
    high = (int) ( highthreshold * MAGNITUDE_SCALE + 0.5f);
    performHysteresis(can, low, high);
    for(i=0;i<width*height;i++)
        answer[i] = can->idata[i] > 0 ? 1 : 0;
    
    killbuffers(can);
    return answer;
error_exit:
    free(answer);
    killbuffers(can);
    return 0;
}


/*
  buffer allocation
*/
CANNY *allocatebuffers(unsigned char *grey, int width, int height) {
    CANNY *answer;

    answer = (CANNY *)malloc(sizeof(CANNY));
    if(!answer)
        goto error_exit;
    answer->data = (unsigned char *)malloc(width * height);
    answer->idata = (int *)malloc(width * height * sizeof(int));
    answer->magnitude = (int *)malloc(width * height * sizeof(int));
    answer->xConv = (float *)malloc(width * height * sizeof(float));
    answer->yConv = (float *)malloc(width * height * sizeof(float));
    answer->xGradient = (float *)malloc(width * height * sizeof(float));
    answer->yGradient = (float *)malloc(width * height * sizeof(float));
    if(!answer->data || !answer->idata || !answer->magnitude || 
        !answer->xConv || !answer->yConv || 
        !answer->xGradient || !answer->yGradient)
         goto error_exit;

    memcpy(answer->data, grey, width * height);
    answer->width = width;
    answer->height = height;

    return answer;
error_exit:
    killbuffers(answer);
    return 0;
}

/*
  buffers destructor
*/
void killbuffers(CANNY *can) {
    if(can) {
        free(can->data);
        free(can->idata);
        free(can->magnitude);
        free(can->xConv);
        free(can->yConv);
        free(can->xGradient);
        free(can->yGradient);
    }
}
    
int computeGradients(CANNY *can, float kernelRadius, int kernelWidth) {   
        float *kernel;
        float *diffKernel;
        int kwidth;

        int width, height;

        int initX;
        int maxX;
        int initY;
        int maxY;

        int x, y;
        int i;
        int flag;

        width = can->width;
        height = can->height;

        kernel = (float *)malloc(kernelWidth * sizeof(float));
        diffKernel = (float *)malloc(kernelWidth * sizeof(float));
        if(!kernel || !diffKernel)
            goto error_exit;

        /* initialise the Gaussian kernel */
        for (kwidth = 0; kwidth < kernelWidth; kwidth++) {
            float g1, g2, g3;
            g1 = gaussian((float) kwidth, kernelRadius);
            if (g1 <= GAUSSIAN_CUT_OFF && kwidth >= 2) 
                break;
            g2 = gaussian(kwidth - 0.5f, kernelRadius);
            g3 = gaussian(kwidth + 0.5f, kernelRadius);
            kernel[kwidth] = (g1 + g2 + g3) / 3.0f / (2.0f * (float) 3.14 * kernelRadius * kernelRadius);
            diffKernel[kwidth] = g3 - g2;
        }

        initX = kwidth - 1;
        maxX = width - (kwidth - 1);
        initY = width * (kwidth - 1);
        maxY = width * (height - (kwidth - 1));
        
        /* perform convolution in x and y directions */
        for(x = initX; x < maxX; x++) {
            for(y = initY; y < maxY; y += width) {
                int index = x + y;
                float sumX = can->data[index] * kernel[0];
                float sumY = sumX;
                int xOffset = 1;
                int yOffset = width;
                while(xOffset < kwidth) {
                    sumY += kernel[xOffset] * (can->data[index - yOffset] + can->data[index + yOffset]);
                    sumX += kernel[xOffset] * (can->data[index - xOffset] + can->data[index + xOffset]);
                    yOffset += width;
                    xOffset++;
                }
                
                can->yConv[index] = sumY;
                can->xConv[index] = sumX;
            }
 
        }
 
        for (x = initX; x < maxX; x++) {
            for (y = initY; y < maxY; y += width) {
                float sum = 0.0f;
                int index = x + y;
                for (i = 1; i < kwidth; i++)
                    sum += diffKernel[i] * (can->yConv[index - i] - can->yConv[index + i]);
 
                can->xGradient[index] = sum;
            }
 
        }

        for(x = kwidth; x < width - kwidth; x++) {
            for (y = initY; y < maxY; y += width) {
                float sum = 0.0f;
                int index = x + y;
                int yOffset = width;
                for (i = 1; i < kwidth; i++) {
                    sum += diffKernel[i] * (can->xConv[index - yOffset] - can->xConv[index + yOffset]);
                    yOffset += width;
                }
 
                can->yGradient[index] = sum;
            }
 
        }
 
        initX = kwidth;
        maxX = width - kwidth;
        initY = width * kwidth;
        maxY = width * (height - kwidth);
        for(x = initX; x < maxX; x++) {
            for(y = initY; y < maxY; y += width) {
                int index = x + y;
                int indexN = index - width;
                int indexS = index + width;
                int indexW = index - 1;
                int indexE = index + 1;
                int indexNW = indexN - 1;
                int indexNE = indexN + 1;
                int indexSW = indexS - 1;
                int indexSE = indexS + 1;
                
                float xGrad = can->xGradient[index];
                float yGrad = can->yGradient[index];
                float gradMag = hypotenuse(xGrad, yGrad);

                /* perform non-maximal supression */
                float nMag = hypotenuse(can->xGradient[indexN], can->yGradient[indexN]);
                float sMag = hypotenuse(can->xGradient[indexS], can->yGradient[indexS]);
                float wMag = hypotenuse(can->xGradient[indexW], can->yGradient[indexW]);
                float eMag = hypotenuse(can->xGradient[indexE], can->yGradient[indexE]);
                float neMag = hypotenuse(can->xGradient[indexNE], can->yGradient[indexNE]);
                float seMag = hypotenuse(can->xGradient[indexSE], can->yGradient[indexSE]);
                float swMag = hypotenuse(can->xGradient[indexSW], can->yGradient[indexSW]);
                float nwMag = hypotenuse(can->xGradient[indexNW], can->yGradient[indexNW]);
                float tmp;

                flag = ( (xGrad * yGrad <= 0.0f) /*(1)*/
                    ? ffabs(xGrad) >= ffabs(yGrad) /*(2)*/
                        ? (tmp = ffabs(xGrad * gradMag)) >= ffabs(yGrad * neMag - (xGrad + yGrad) * eMag) /*(3)*/
                            && tmp > fabs(yGrad * swMag - (xGrad + yGrad) * wMag) /*(4)*/
                        : (tmp = ffabs(yGrad * gradMag)) >= ffabs(xGrad * neMag - (yGrad + xGrad) * nMag) /*(3)*/
                            && tmp > ffabs(xGrad * swMag - (yGrad + xGrad) * sMag) /*(4)*/
                    : ffabs(xGrad) >= ffabs(yGrad) /*(2)*/
                        ? (tmp = ffabs(xGrad * gradMag)) >= ffabs(yGrad * seMag + (xGrad - yGrad) * eMag) /*(3)*/
                            && tmp > ffabs(yGrad * nwMag + (xGrad - yGrad) * wMag) /*(4)*/
                        : (tmp = ffabs(yGrad * gradMag)) >= ffabs(xGrad * seMag + (yGrad - xGrad) * sMag) /*(3)*/
                            && tmp > ffabs(xGrad * nwMag + (yGrad - xGrad) * nMag) /*(4)*/
                    );
                if(flag) {
                    can->magnitude[index] = (gradMag >= MAGNITUDE_LIMIT) ? MAGNITUDE_MAX : (int) (MAGNITUDE_SCALE * gradMag);
                } 
                else {
                    can->magnitude[index] = 0;
                }
            }
        }
        free(kernel);
        free(diffKernel);
        return 0;
error_exit:
        free(kernel);
        free(diffKernel);
        return -1;
}

void performHysteresis(CANNY *can, int low, int high) {
  int offset = 0;
  int x, y;
    
  memset(can->idata, 0, can->width * can->height * sizeof(int));
        
  for(y = 0; y < can->height; y++) {
    for(x = 0; x < can->width; x++) {
      if(can->idata[offset] == 0 && can->magnitude[offset] >= high) 
        follow(can, x, y, offset, low);
      offset++;
    }
  }
}
    
 void follow(CANNY *can, int x1, int y1, int i1, int threshold) {
  int x, y;
  int x0 = x1 == 0 ? x1 : x1 - 1;
  int x2 = x1 == can->width - 1 ? x1 : x1 + 1;
  int y0 = y1 == 0 ? y1 : y1 - 1;
  int y2 = y1 == can->height -1 ? y1 : y1 + 1;
        
  can->idata[i1] = can->magnitude[i1];
  for (x = x0; x <= x2; x++) {
    for (y = y0; y <= y2; y++) {
      int i2 = x + y * can->width;
      if ((y != y1 || x != x1) && can->idata[i2] == 0 && can->magnitude[i2] >= threshold) 
        follow(can, x, y, i2, threshold);
    }
  }
}

void normalizeContrast(unsigned char *data, int width, int height) {
    int histogram[256] = {0};
    int remap[256];
    int sum = 0;
    int j = 0;
    int k;
    int target;
    int i;

    for (i = 0; i < width * height; i++) 
        histogram[data[i]]++;
        
    
    for (i = 0; i < 256; i++) {
        sum += histogram[i];
        target = (sum*255)/(width * height);
        for (k = j+1; k <= target; k++) 
            remap[k] = i;
        j = target;
     }
        
    for (i = 0; i < width * height; i++) 
        data[i] = remap[data[i]];
}

 float hypotenuse(float x, float y) {
    return (float) sqrt(x*x +y*y);
}
 
float gaussian(float x, float sigma) {
    return (float) exp(-(x * x) / (2.0f * sigma * sigma));
}

