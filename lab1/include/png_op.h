#define _GNU_SOURCE
#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>
#include <limits.h>
#include <zlib.h>
#include "zutil.h"
#include "lab_png.h"
#include "crc.h"

#ifndef PATH_MAXs
#define PATH_MAX 4096
#endif
#define MAX_FILES 4096


char** find_png(const char *path);

int cat_png(const char** input_paths, int file_count, const char* out_path);

int end_with_png(const char *filename);

void search_dir(const char *path);

int check_png(const char *path);

simple_PNG_p* load_png(const char** paths, int count);

U8* extract_scanlines(simple_PNG_p img, data_IHDR_p ihdr, uLongf* out_len);
  
U8* build_idat(U8** raw_data_array, const int* heights, int file_count, int scanline_len, U64* out_compressed_len);

int build_stitched_png(const char* output_path, simple_PNG_p* pngs, data_IHDR_p* ihdrs, U8** raw_data_array, int* heights, int file_count, int width);
