#include <stdio.h>
#include <stdlib.h>

int main()
{
   FILE *fptr;
    char * line = NULL;
    size_t len = 0;
    ssize_t read;

   if ((fptr = fopen("./config.txt","r")) == NULL){
       printf("Error! opening file\n");

       // Program exits if the file pointer returns NULL.
       exit(1);
   }

    while ((read = getline(&line, &len, fptr)) != -1) {
        if(line[0] == '%')
            continue;
        printf("%s", line);
    }

   fclose(fptr); 
  
   return 0;
}