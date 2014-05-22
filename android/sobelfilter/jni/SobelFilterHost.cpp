/*
 * Copyright 1993-2010 NVIDIA Corporation.  All rights reserved.
 *
 * Please refer to the NVIDIA end user license agreement (EULA) associated
 * with this source code for terms and conditions that govern your use of
 * this software. Any use, reproduction, disclosure, or distribution of
 * this software and related documentation outside the terms of the EULA
 * is strictly prohibited.
 *
 */

// standard utilities and systems includes
//#include <oclUtils.h>
#include <math.h>
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

using namespace std;

const unsigned int PGMHeaderSize = 0x40;

int loadPPM(const char* file, unsigned char** data, 
            unsigned int *w, unsigned int *h, unsigned int *channels) 
{
    FILE* fp = 0;

        // open the file for binary read
        if ((fp = fopen(file, "rb")) == 0)
        {
            // if error on attempt to open, be sure the file is null or close it, then return negative error code
            if (fp)
            {
                fclose (fp);
            }
            std::cerr << "loadPPM() : Failed to open file: " << file << std::endl;
            return -1;
        }

    // check header
    char header[PGMHeaderSize];
    if ((fgets( header, PGMHeaderSize, fp) == NULL) && ferror(fp))
    {
        if (fp)
        {
            fclose (fp);
        }
        std::cerr << "loadPPM() : File is not a valid PPM or PGM image" << std::endl;
        *channels = 0;
        return -1;
    }

    if (strncmp(header, "P5", 2) == 0)
    {
        *channels = 1;
    }
    else if (strncmp(header, "P6", 2) == 0)
    {
        *channels = 3;
    }
    else
    {
        std::cerr << "loadPPM() : File is not a PPM or PGM image" << std::endl;
        *channels = 0;
        return -1;
    }

    // parse header, read maxval, width and height
    unsigned int width = 0;
    unsigned int height = 0;
    unsigned int maxval = 0;
    unsigned int i = 0;
    while(i < 3) 
    {
        if ((fgets(header, PGMHeaderSize, fp) == NULL) && ferror(fp))
        {
            if (fp)
            {
                fclose (fp);
            }
            std::cerr << "loadPPM() : File is not a valid PPM or PGM image" << std::endl;
            return -1;
        }
        if(header[0] == '#') continue;

            if(i == 0) 
            {
                i += sscanf(header, "%u %u %u", &width, &height, &maxval);
            }
            else if (i == 1) 
            {
                i += sscanf(header, "%u %u", &height, &maxval);
            }
            else if (i == 2) 
            {
                i += sscanf(header, "%u", &maxval);
            }
    }

    // check if given handle for the data is initialized
    if(NULL != *data) 
    {
        if (*w != width || *h != height) 
        {
            fclose(fp);
            std::cerr << "loadPPM() : Invalid image dimensions." << std::endl;
            return -1;
        }
    } 
    else 
    {
        *data = (unsigned char*)malloc( sizeof(unsigned char) * width * height * *channels);
        *w = width;
        *h = height;
    }

    // read and close file
    if (fread(*data, sizeof(unsigned char), width * height * *channels, fp) != width * height * *channels)
    {
        fclose(fp);
        std::cerr << "loadPPM() : Invalid image." << std::endl;
        return -1;
    }
    fclose(fp);

    return 0;
}

int shrLoadPPM4ub( const char* file, unsigned char** OutData, 
                unsigned int *w, unsigned int *h)
{
    // Load file data into a temporary buffer with automatic allocation
    unsigned char* cLocalData = 0;
    unsigned int channels;
    int bLoadOK = loadPPM(file, &cLocalData, w, h, &channels);   // this allocates cLocalData, which must be freed later

    // If the data loaded OK from file to temporary buffer, then go ahead with padding and transfer 
    if (bLoadOK == 0)
    {
        // if the receiving buffer is null, allocate it... caller must free this 
        int size = *w * *h;
        if (*OutData == NULL)
        {
            *OutData = (unsigned char*)malloc(sizeof(unsigned char) * size * 4);
        }

        // temp pointers for incrementing
        unsigned char* cTemp = cLocalData;
        unsigned char* cOutPtr = *OutData;

        // transfer data, padding 4th element
        for(int i=0; i<size; i++) 
        {
            *cOutPtr++ = *cTemp++;
            *cOutPtr++ = *cTemp++;
            *cOutPtr++ = *cTemp++;
            *cOutPtr++ = 0;
        }

        // free temp lcoal buffer and return OK
        free(cLocalData);
        return 0;
    }
    else
    {
        // image wouldn't load
        free(cLocalData);
        return -1;
    }
}

int savePPM( const char* file, unsigned char *data, 
             unsigned int w, unsigned int h, unsigned int channels) 
{
    fstream fh( file, fstream::out | fstream::binary );
    if( fh.bad()) 
    {
        std::cerr << "savePPM() : Opening file failed." << std::endl;
        return -1;
    }

    if (channels == 1)
    {
        fh << "P5\n";
    }
    else if (channels == 3) {
        fh << "P6\n";
    }
    else {
        std::cerr << "savePPM() : Invalid number of channels." << std::endl;
        return -1;
    }

    fh << w << "\n" << h << "\n" << 0xff << std::endl;

    for( unsigned int i = 0; (i < (w*h*channels)) && fh.good(); ++i) 
    {
        fh << data[i];
    }
    fh.flush();

    if( fh.bad()) 
    {
        std::cerr << "savePPM() : Writing data failed." << std::endl;
        return -1;
    } 
    fh.close();

    return 0;
}

int shrSavePPM4ub( const char* file, unsigned char *data, 
               unsigned int w, unsigned int h) 
{
    // strip 4th component
    int size = w * h;
    unsigned char *ndata = (unsigned char*) malloc( sizeof(unsigned char) * size*3);
    unsigned char *ptr = ndata;
    for(int i=0; i<size; i++) {
        *ptr++ = *data++;
        *ptr++ = *data++;
        *ptr++ = *data++;
        data++;
    }
    
    int succ = savePPM(file, ndata, w, h, 3);
    free(ndata);
    return succ;
}

//*****************************************************************
//! Exported Host/C++ RGB Sobel gradient magnitude function
//! Gradient intensity is from RSS combination of H and V gradient components
//! R, G and B gradient intensities are treated separately then combined with linear weighting
//!
//! Implementation below is equivalent to linear 2D convolutions for H and V compoonents with:
//!	    Convo Coefs for Horizontal component {1,0,-1,   2,0,-2,  1,0,-1}
//!	    Convo Coefs for Vertical component   {-1,-2,-1,  0,0,0,  1,2,1};
//! @param uiInputImage     pointer to input data
//! @param uiOutputImage    pointer to output dataa
//! @param uiWidth          width of image
//! @param uiHeight         height of image
//! @param fThresh          output intensity threshold 
//*****************************************************************
void SobelFilterHost(unsigned int* uiInputImage, unsigned int* uiOutputImage, unsigned int uiWidth, unsigned int uiHeight, float fThresh)
{
    // start computation timer
    //shrDeltaT(0);

	// do the Sobel magnitude with thresholding 
	for(unsigned int y = 0; y < uiHeight; y++)			// all the rows
	{
		for(unsigned int x = 0; x < uiWidth; x++)		// all the columns
		{
            // local registers for working with RGB subpixels and managing border
            unsigned char* ucRGBA; 
            const unsigned int uiZero = 0U;

            // Init summation registers to zero
            float fTemp = 0.0f; 
            float fHSum[3] = {0.0f, 0.0f, 0.0f}; 
            float fVSum[3] = {0.0f, 0.0f, 0.0f}; 

            // Read in pixel value to local register:  if boundary pixel, use zero
            if ((x > 0) && (y > 0))
            {
                ucRGBA = (unsigned char*)&uiInputImage [((y - 1) * uiWidth) + (x - 1)];
            }
            else 
            {
                ucRGBA = (unsigned char*)&uiZero;
            }

            // NW
		    fHSum[0] += ucRGBA[0];    // horizontal gradient of Red
		    fHSum[1] += ucRGBA[1];    // horizontal gradient of Green
		    fHSum[2] += ucRGBA[2];    // horizontal gradient of Blue
            fVSum[0] -= ucRGBA[0];    // vertical gradient of Red
		    fVSum[1] -= ucRGBA[1];    // vertical gradient of Green
		    fVSum[2] -= ucRGBA[2];    // vertical gradient of Blue

            // Read in next pixel value to a local register:  if boundary pixel, use zero
            if (y > 0) 
            {
                ucRGBA = (unsigned char*)&uiInputImage [((y - 1) * uiWidth) + x];
            }
            else 
            {
                ucRGBA = (unsigned char*)&uiZero;
            }

            // N
		    fVSum[0] -= (ucRGBA[0] << 1);  // vertical gradient of Red
		    fVSum[1] -= (ucRGBA[1] << 1);  // vertical gradient of Green
		    fVSum[2] -= (ucRGBA[2] << 1);  // vertical gradient of Blue

            // Read in next pixel value to a local register:  if boundary pixel, use zero
            if ((x < (uiWidth - 1)) && (y > 0))
            {
                ucRGBA = (unsigned char*)&uiInputImage [((y - 1) * uiWidth) + (x + 1)];
            }
            else 
            {
                ucRGBA = (unsigned char*)&uiZero;
            }

            // NE
		    fHSum[0] -= ucRGBA[0];    // horizontal gradient of Red
		    fHSum[1] -= ucRGBA[1];    // horizontal gradient of Green
		    fHSum[2] -= ucRGBA[2];    // horizontal gradient of Blue
		    fVSum[0] -= ucRGBA[0];    // vertical gradient of Red
		    fVSum[1] -= ucRGBA[1];    // vertical gradient of Green
		    fVSum[2] -= ucRGBA[2];    // vertical gradient of Blue

            // Read in pixel value to a local register:  if boundary pixel, use zero
            if (x > 0) 
            {
                ucRGBA = (unsigned char*)&uiInputImage [(y * uiWidth) + (x - 1)];
            }
            else 
            {
                ucRGBA = (unsigned char*)&uiZero;
            }

            // W
		    fHSum[0] += (ucRGBA[0] << 1);  // vertical gradient of Red
		    fHSum[1] += (ucRGBA[1] << 1);  // vertical gradient of Green
		    fHSum[2] += (ucRGBA[2] << 1);  // vertical gradient of Blue

            // C
            // nothing to do for center pixel

            // Read in pixel value to a local register:  if boundary pixel, use zero
            if (x < (uiWidth - 1))
            {
                ucRGBA = (unsigned char*)&uiInputImage [(y * uiWidth) + (x + 1)];
            }
            else 
            {
                ucRGBA = (unsigned char*)&uiZero;
            }

            // E
		    fHSum[0] -= (ucRGBA[0] << 1);  // vertical gradient of Red
		    fHSum[1] -= (ucRGBA[1] << 1);  // vertical gradient of Green
		    fHSum[2] -= (ucRGBA[2] << 1);  // vertical gradient of Blue

            // Read in pixel value to a local register:  if boundary pixel, use zero
            if ((x > 0) && (y < (uiHeight - 1)))
            {
                ucRGBA = (unsigned char*)&uiInputImage [((y + 1) * uiWidth) + (x - 1)];
            }
            else 
            {
                ucRGBA = (unsigned char*)&uiZero;
            }

            // SW
		    fHSum[0] += ucRGBA[0];    // horizontal gradient of Red
		    fHSum[1] += ucRGBA[1];    // horizontal gradient of Green
		    fHSum[2] += ucRGBA[2];    // horizontal gradient of Blue
		    fVSum[0] += ucRGBA[0];    // vertical gradient of Red
		    fVSum[1] += ucRGBA[1];    // vertical gradient of Green
		    fVSum[2] += ucRGBA[2];    // vertical gradient of Blue

            // Read in pixel value to a local register:  if boundary pixel, use zero
            if (y < (uiHeight - 1))
            {
                ucRGBA = (unsigned char*)&uiInputImage [((y + 1) * uiWidth) + x];
            }
            else 
            {
                ucRGBA = (unsigned char*)&uiZero;
            }

            // S
		    fVSum[0] += (ucRGBA[0] << 1);  // vertical gradient of Red
		    fVSum[1] += (ucRGBA[1] << 1);  // vertical gradient of Green
		    fVSum[2] += (ucRGBA[2] << 1);  // vertical gradient of Blue

            // Read in pixel value to a local register:  if boundary pixel, use zero
            if ((x < (uiWidth - 1)) && (y < (uiHeight - 1)))
            {
                ucRGBA = (unsigned char*)&uiInputImage [((y + 1) * uiWidth) + (x + 1)];
            }
            else 
            {
                ucRGBA = (unsigned char*)&uiZero;
            }

            // SE
		    fHSum[0] -= ucRGBA[0];    // horizontal gradient of Red
		    fHSum[1] -= ucRGBA[1];    // horizontal gradient of Green
		    fHSum[2] -= ucRGBA[2];    // horizontal gradient of Blue
		    fVSum[0] += ucRGBA[0];    // vertical gradient of Red
		    fVSum[1] += ucRGBA[1];    // vertical gradient of Green
		    fVSum[2] += ucRGBA[2];    // vertical gradient of Blue
            
		    // Weighted combination of Root-Sum-Square per-color-band H & V gradients for each of RGB
            fTemp =  0.30f * sqrtf((fHSum[0] * fHSum[0]) + (fVSum[0] * fVSum[0]));
			fTemp += 0.55f * sqrtf((fHSum[1] * fHSum[1]) + (fVSum[1] * fVSum[1]));
			fTemp += 0.15f * sqrtf((fHSum[2] * fHSum[2]) + (fVSum[2] * fVSum[2]));

            // threshold and clamp
            if (fTemp < fThresh)
            {
                fTemp = 0.0f;
            }
            else if (fTemp > 255.0f)
            {
                fTemp = 255.0f;
            }

            // pack into a monochrome uint (including average alpha)
            unsigned int uiPackedPix = 0x000000FF & (unsigned int)fTemp;
            uiPackedPix |= 0x0000FF00 & (((unsigned int)fTemp) << 8);
            uiPackedPix |= 0x00FF0000 & (((unsigned int)fTemp) << 16);

			// copy to output
			uiOutputImage[y * uiWidth + x] = uiPackedPix;	
		}
	}
	
    // return computation elapsed time in seconds
    //return shrDeltaT(0);	
}

int main()
{
    unsigned char *uiInImage = NULL;
    unsigned int width = 1920;
    unsigned int height = 1080;
    float fThresh = 80.0f;
    unsigned int totalSize = width * height * sizeof(unsigned int);
    unsigned char *uiOutImage = (unsigned char*) malloc(totalSize);

    shrLoadPPM4ub("tmp.ppm", &uiInImage, &width, &height);
    SobelFilterHost((unsigned int *)uiInImage, (unsigned int*)uiOutImage, 
                    width, height, fThresh);
    shrSavePPM4ub("out.ppm", uiOutImage, width, height);

    return 0;
}
