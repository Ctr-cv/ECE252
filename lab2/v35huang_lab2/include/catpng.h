#include <sys/stat.h>
#include <stdio.h>    /* for printf(), perror()...   */
#include <stdlib.h>   /* for malloc()                */
#include <errno.h>    /* for errno                   */
#include "zutil.h"    /* for mem_def() and mem_inf() */
#include "lab_png.h"  /* yes */
#define _POSIX_C_SOURCE 200112L  // or higher

int cat(int argc, char *argv[]);