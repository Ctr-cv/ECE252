#include <stdio.h> 
#include <stdlib.h> 
#include <errno.h>   
#include "crc.h"    
#include "zutil.h"    
#include "lab_png.h"  
#include "png_op.h"

int main (int argc, char **argv) {
    char *path = argv[1];
    char** png_files = find_png(path);
    if (!png_files) {
        fprintf(stderr, "No PNG files found or error occurred.\n");
        return 1;
    }

    for (int i = 0; png_files[i] != NULL; ++i) {
        printf("%s\n", png_files[i]);
        free(png_files[i]);  
    }

    free(png_files); 
    return 0;
}