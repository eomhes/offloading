#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

const unsigned int PGMHeaderSize = 0x40;

int loadPPM(const char* file, unsigned char** data, 
            unsigned int *w, unsigned int *h, unsigned int *channels) 
{
    FILE* fp = 0;

    if ((fp = fopen(file, "rb")) == 0) {
        if (fp) {
            fclose (fp);
        }
        fprintf(stderr, "open failed\n");
        return -1;
    }

    char header[PGMHeaderSize];
    if ((fgets( header, PGMHeaderSize, fp) == NULL) && ferror(fp)) {
        if (fp) {
            fclose (fp);
        }
        fprintf(stderr, "not a valid ppm file\n");
        *channels = 0;
        return -1;
    }

    if (strncmp(header, "P5", 2) == 0) {
        *channels = 1;
    }
    else if (strncmp(header, "P6", 2) == 0) {
        *channels = 3;
    }
    else {
        fprintf(stderr, "loadPPM() : File is not a PPM or PGM image\n");
        *channels = 0;
        return -1;
    }

    unsigned int width = 0;
    unsigned int height = 0;
    unsigned int maxval = 0;
    unsigned int i = 0;
    while(i < 3) {
        if ((fgets(header, PGMHeaderSize, fp) == NULL) && ferror(fp)) {
            if (fp) {
                fclose (fp);
            }
            fprintf(stderr, "loadPPM() : File is not a valid PPM or PGM image\n");
            return -1;
        }

        if(header[0] == '#') continue;

        if(i == 0) {
            i += sscanf(header, "%u %u %u", &width, &height, &maxval);
        }
        else if (i == 1) {
            i += sscanf(header, "%u %u", &height, &maxval);
        }
        else if (i == 2) 
        {
            i += sscanf(header, "%u", &maxval);
        }
    }

    if(NULL != *data) {
        if (*w != width || *h != height) {
            fclose(fp);
            fprintf(stderr, "loadPPM() : Invalid image dimensions.\n");
            return -1;
        }
    } 
    else {
        *data = (unsigned char*)malloc( sizeof(unsigned char) * width * 
            height * *channels);
        printf("malloc1 %d\n", sizeof(unsigned char) * width * height * 
            *channels);
        *w = width;
        *h = height;
    }

    if (fread(*data, sizeof(unsigned char), width * height * *channels, fp) != 
        width * height * *channels) {
        fclose(fp);
        fprintf(stderr, "loadPPM() : Invalid image.\n");
        return -1;
    }
    fclose(fp);

    return 0;
}

int shrLoadPPM4ub( const char* file, unsigned char** OutData, 
                unsigned int *w, unsigned int *h)
{
    unsigned char* cLocalData = 0;
    unsigned int channels;
    int bLoadOK = loadPPM(file, &cLocalData, w, h, &channels);

    if (bLoadOK == 0) {
        int size = *w * *h;
        if (*OutData == NULL) {
            *OutData = (unsigned char*)malloc(sizeof(unsigned char) * 
                size * 4);
        }

        unsigned char* cTemp = cLocalData;
        unsigned char* cOutPtr = *OutData;

        int i;
        for(i=0; i<size; i++) {
            *cOutPtr++ = *cTemp++;
            *cOutPtr++ = *cTemp++;
            *cOutPtr++ = *cTemp++;
            *cOutPtr++ = 0;
        }

        free(cLocalData);
        return 0;
    }
    else {
        free(cLocalData);
        return -1;
    }
}

int savePPM( const char* file, unsigned char *data, 
             unsigned int w, unsigned int h, unsigned int channels) 
{
    FILE* fp = 0;
    if ((fp = fopen(file, "wb")) == 0) {
        if (fp) {
            fclose (fp);
        }
        fprintf(stderr, "open failed\n");
        return -1;
    }

    if (channels == 1) {
        fwrite("P5\n", sizeof(char), 3, fp);
    }
    else if (channels == 3) {
        fwrite("P6\n", sizeof(char), 3, fp);
    }
    else {
        fprintf(stderr, "savePPM() : Invalid number of channels.\n");
        return -1;
    }

    int maxval = 255;
    char header[100];
    int len = sprintf(header, "%d %d\n%d\n", w, h, maxval);

    fwrite(header, sizeof(char), len, fp);

    if (fwrite(data, sizeof(unsigned char), w * h * channels, fp) < 0) {
        fprintf(stderr, "fwrite failed\n");
        fclose(fp);
        return -1;
    }

    fclose(fp);
    return 0;
}

int shrSavePPM4ub( const char* file, unsigned char *data, 
               unsigned int w, unsigned int h) 
{
    int size = w * h;
    unsigned char *ndata = (unsigned char*) malloc( sizeof(unsigned char) * 
        size*3);
    unsigned char *ptr = ndata;
    int i;
    for(i=0; i < size; i++) {
        *ptr++ = *data++;
        *ptr++ = *data++;
        *ptr++ = *data++;
        data++;
    }
    
    int succ = savePPM(file, ndata, w, h, 3);
    free(ndata);
    return succ;
}

size_t shrRoundUp(int group_size, int global_size)
{
    int r = global_size % group_size;
    if(r == 0) {
        return global_size;
    } 
    else {
        return global_size + group_size - r;
    }
}

void shrFillArray(float* data, int size)
{
    int i;
    const float scale = 1.0f / (float)RAND_MAX;
    for (i = 0; i < size; ++i) {
        data[i] = scale * rand();
    }
}

char *load_program_source(const char *filename)
{
    struct stat statbuf;
    FILE *fh;
    char *source;

    fh = fopen(filename, "r");
    if (fh == 0) return 0;

    stat(filename, &statbuf);
    source = (char *) malloc(statbuf.st_size + 1);
    fread(source, statbuf.st_size, 1, fh);
    source[statbuf.st_size] = '\0';
    return source;
}

