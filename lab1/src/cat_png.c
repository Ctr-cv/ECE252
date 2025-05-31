#include <stdio.h> 
#include <stdlib.h> 
#include <errno.h>   
#include "crc.h"    
#include "zutil.h"    
#include "lab_png.h"  
#include "png_op.h"

int main (int argc, char **argv) {
    const char** input_paths = (const char**)&argv[1];
    int file_count = argc - 1;
    cat_png(input_paths, file_count, "all.png");
    return 0;
}