
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

int fill_rand(float* data, size_t size, const int min, const int max)
{
    srand(2012);
    double range = 1.0 + (double)(max - min);

    size_t i;
    for (i = 0; i < size; i++) {
        data[i] = min + (float)(range*rand()/(RAND_MAX + 1.0));
    }
    return 0;
}

void fftCPU(short int dir,long m,float *x,float *y)
{
   long n, i, i1, j, k, i2, l, l1, l2;
   double c1, c2, tx, ty, t1, t2, u1, u2, z;

   // Calculate the number of points 
   n = 1;
   for (i = 0;i < m;i++) 
      n *= 2;

   // Do the bit reversal 
   i2 = n >> 1;
   j = 0;
   for (i = 0;i < n - 1;i++) 
   {
      if (i < j) 
      {
         tx = x[i];
         ty = y[i];
         x[i] = x[j];
         y[i] = y[j];
         x[j] = (float)tx;
         y[j] = (float)ty;
      }
      k = i2;
      while (k <= j)
      {
         j -= k;
         k >>= 1;
      }
      j += k;
   }

   // Compute the FFT 
   c1 = -1.0; 
   c2 = 0.0;
   l2 = 1;
   for (l = 0;l < m;l++) 
   {
      l1 = l2;
      l2 <<= 1;
      u1 = 1.0; 
      u2 = 0.0;
      for (j = 0;j < l1;j++)
      {
         for (i = j;i < n;i += l2) 
         {
            i1 = i + l1;
            t1 = u1 * x[i1] - u2 * y[i1];
            t2 = u1 * y[i1] + u2 * x[i1];
            x[i1] = (float)(x[i] - t1); 
            y[i1] = (float)(y[i] - t2);
            x[i] += (float)t1;
            y[i] += (float)t2;
         }
         z =  u1 * c1 - u2 * c2;
         u2 = u1 * c2 + u2 * c1;
         u1 = z;
      }
      c2 = sqrt((1.0 - c1) / 2.0);
      if (dir == 1) 
         c2 = -c2;
      c1 = sqrt((1.0 + c1) / 2.0);
   }

}


int main(int argc, char *argv[])
{
    int length = 1024 * 1024;
    size_t buf_size = length * sizeof(float);
    float *input_r, *input_i, *output_r, *output_i;

    input_r = (float*)malloc(buf_size);
    input_i = (float*)malloc(buf_size);
    output_r = (float*)malloc(buf_size);
    output_i = (float*)malloc(buf_size);

    fill_rand(input_r, length, 0, 255);
    fill_rand(input_i, length, 0, 0);
    memcpy(output_r, input_r, buf_size);
    memcpy(output_i, input_i, buf_size);

    fftCPU(1, 20, output_r, output_i);
/*
    int i;
    for (i = 0; i < length; i++) {
        printf("%i %f %f\n", i, output_r[i], output_i[i]);
    }
*/
}
