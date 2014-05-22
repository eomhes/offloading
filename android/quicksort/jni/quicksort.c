
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

void generateInput(int* inputArray, size_t arraySize)
{
    const size_t minElement = 0;
    const size_t maxElement = arraySize + 1;

    srand(12345);

    // random initialization of input
    size_t i;
    for (i = 0; i < arraySize; ++i) {
        inputArray[i] = (int)((float) (maxElement - minElement) * 
            (rand() / (float) RAND_MAX));
    }
}

void swap(int *x,int *y)
{
   int temp;
   temp = *x;
   *x = *y;
   *y = temp;
}

int choose_pivot(int i,int j )
{
   return((i+j) /2);
}

void quicksort(int list[],int m,int n)
{
   int key,i,j,k;
   if( m < n)
   {
      k = choose_pivot(m,n);
      swap(&list[m],&list[k]);
      key = list[m];
      i = m+1;
      j = n;
      while(i <= j)
      {
         while((i <= n) && (list[i] <= key))
                i++;
         while((j >= m) && (list[j] > key))
                j--;
         if( i < j)
                swap(&list[i],&list[j]);
      }
      // swap two elements
      swap(&list[m],&list[j]);
      // recursively sort the lesser list
      quicksort(list,m,j-1);
      quicksort(list,j+1,n);
   }
}

int main(int argc, char *argv[])
{
    size_t i;
    int scale = atoi(argv[1]); // scale should be higher than 8
    size_t array_size = powl(2, scale) * 4;
    int *input = (int *) malloc(sizeof(int) * array_size);
    int *output = (int *) malloc(sizeof(int) * array_size);
    printf("size %lu\n", array_size * sizeof(int)/(1024*1024));

    int dir = 1;
    int no_stages = 0;
    int temp;

    generateInput(input, array_size);
    quicksort(input, 0, array_size-1);

    exit(0);

    for (i = 0; i < array_size; i++) {
        printf("%i %i\n", i, input[i]);
    }

}
