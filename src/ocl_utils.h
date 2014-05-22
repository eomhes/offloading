
#ifndef _OCL_UTILS_H_
#define _OCL_UTILS_H_

int loadPPM(const char* file, unsigned char** data, 
            unsigned int *w, unsigned int *h, unsigned int *channels) ;

int shrLoadPPM4ub(const char* file, unsigned char** OutData, 
                  unsigned int *w, unsigned int *h);

int savePPM(const char* file, unsigned char *data, 
            unsigned int w, unsigned int h, unsigned int channels);

int shrSavePPM4ub(const char* file, unsigned char *data, 
                  unsigned int w, unsigned int h);

size_t shrRoundUp(int group_size, int global_size);

void shrFillArray(float* data, int size);

char *load_program_source(const char *filename);

#endif
