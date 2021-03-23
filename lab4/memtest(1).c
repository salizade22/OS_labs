#include <stdlib.h>
#include <stdio.h>

int main (int argc, char **argv){

  //Initial memory allocation
  char* x=malloc(24);
  for(int i =0;i<24;i++){
    x[i]=2*i;
  }
  // Reallocating memory
  char* a = (char*)realloc(x,48);
 
  for(int i =0;i<24;i++){
    printf("%d ", a[i]);
     if((int)a[i]!=(int)x[i]){
       printf("\nRealloc failed\n");
     }
     else if (i==23){
       printf("\nRealloc works properly\n");
     }
  }
  printf("\n The elements of x are:          ");
  for(int i=0;i<24;i++){
    printf("%d ",x[i]);
  }
   printf("\n The first 24 elements of a are: ");
    for(int i =0;i<24;i++){
      printf("%d ", a[i]);}

  
  char* y = malloc(19);
  char* z = malloc(32);
  
 printf("\n\nx = %p\n", x);
 if((*x) % 16==0){
    printf("x is double-word aligned\n");
  }

  
  else{
     printf("x is not double-word aligned\n");
  }

 
  printf("\ny = %p\n", y);

  if((*y) % 16==0){
    printf(" y is double-word aligned\n");
  }
  else{
     printf(" y is not double-word aligned\n");
  }
  free(y);
  
  printf("\nz = %p\n", z);


  if((*z) % 16==0){
    printf(" z is double-word aligned\n");
  }
  else{
     printf(" z is not double-word aligned\n");
  }

}
