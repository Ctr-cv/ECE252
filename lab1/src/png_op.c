#include "png_op.h"

static char** result_list = NULL;
static int result_index = 0;

int end_with_png(const char *filename) {
    const char *exd = strrchr(filename, '.');
    return exd && strcasecmp(exd, ".png") == 0;
}

int check_png(const char *path) {
    FILE* fp = fopen(path, "rb");
    if(!fp) {
        perror("Error opening png file\n");
        return 1;
    }
    U8 header[8];
    fread(header, 1, 8, fp);
    fclose(fp);
    return is_png(header, 8);
}

void search_dir(const char *path) {
    DIR* dir = opendir(path);
    if (!dir) {
        perror("Error opening dir\n");
        return;
    }

    struct dirent* entry;
    while((entry = readdir(dir)) != NULL) {
        if(strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;
        char fullpath[PATH_MAX];
        snprintf(fullpath, sizeof(fullpath),"%s/%s", path, entry->d_name);

        struct stat st;
        if(stat(fullpath, &st) == -1) continue;

        if(S_ISDIR(st.st_mode)) {
            search_dir(fullpath);
        } else if(S_ISREG(st.st_mode) && end_with_png(entry->d_name)) {
            if (check_png(fullpath)) result_list[result_index++] = strdup(fullpath);
        }

        if(result_index >= MAX_FILES - 1) break;
    }
    closedir(dir);
}


char** find_png(const char* path) {
    result_list = malloc(sizeof(char*) * MAX_FILES);
    result_index = 0;
    search_dir(path);
    result_list[result_index] = NULL;
    return result_list;
}

simple_PNG_p* load_png(const char** paths, int count) {
    simple_PNG_p* png_array = malloc(sizeof(simple_PNG_p) * count);
    if(!paths) {
        printf("Failed to allocate simple_PNG_p array\n");
        return NULL;
    }

    for(int i = 0; i < count; ++i) {
        FILE *fp = fopen(paths[i], "rb");

        if (!fp) {
            printf("Failed to open PNG file: %s\n", paths[i]);
            png_array[i] = NULL;
            fclose(fp);
            continue;
        }

        if(!check_png(paths[i])) {
            printf("%s not a png file", paths[i]);
            png_array[i] = NULL;
            fclose(fp);
            continue;
        }

        simple_PNG_p img = malloc(sizeof(struct simple_PNG));
        if(get_png_chunks(img, fp, 8, SEEK_SET) != 1) {
            printf("Failed to get chunk");
            free(img);
            png_array[i] = NULL;
            fclose(fp);
            continue;
        }

        png_array[i] = img;
        fclose(fp);
    }

    return png_array;
}

U8* extract_scanlines(simple_PNG_p img, data_IHDR_p ihdr, uLongf* out_len) {
    if (!img || !img->p_IDAT || !ihdr) {
        fprintf(stderr, "Invalid input to extract_scanlines\n");
        return NULL;
    }

    int width = ihdr->width;
    int height = ihdr->width;
    
    int scanline_len = 1 + width * 4;
    *out_len = scanline_len * height;

    U8* raw_data = malloc(*out_len);
    if(!raw_data) {
        fprintf(stderr, "Error creating raw data");
        return NULL;
    }

    U64 dest_len = *out_len;
    U64 src_len = img->p_IDAT->length;
    int result = mem_inf(raw_data, &dest_len, img->p_IDAT->p_data, src_len);
    if (result != Z_OK) {
        perror("malloc");
        free(raw_data);
        return NULL;
    }

    return raw_data;
}

U8* build_idat(U8** raw_data_array, const int* heights, int file_count, int scanline_len, U64* out_compressed_len) {
    if (!raw_data_array || !heights || file_count <= 0 || scanline_len <= 0) {
        fprintf(stderr, "Invalid input to build_idat\n");
        return NULL;
    }

    int total_lines = 0;
    for (int i = 0; i < file_count; ++i) {
        total_lines += heights[i];
    }

    U64 raw_len = total_lines * scanline_len;
    U8* stitched = malloc(raw_len);
    if (!stitched) {
        perror("malloc");
        free(stitched);
        return NULL;
    }

    int offset = 0;
    for (int i = 0; i < file_count; ++i) {
        int size = heights[i] * scanline_len;
        memcpy(stitched + offset, raw_data_array[i], size);
        offset += size;
    }

    U8* compressed = malloc(raw_len); 
    if (!compressed) {
        perror("malloc");
        free(stitched);
        return NULL;
    }

    int ret = mem_def(compressed, out_compressed_len, stitched, raw_len, Z_DEFAULT_COMPRESSION);
    if (ret != Z_OK) {
        zerr(ret);
        free(stitched);
        free(compressed);
        return NULL;
    }

    free(stitched);
    return compressed;
}

int build_stitched_png(const char* output_path, simple_PNG_p* pngs, data_IHDR_p* ihdrs, U8** raw_data_array, int* heights, int file_count, int width) {
    if (!output_path || !pngs || !ihdrs || !raw_data_array || !heights || file_count <= 0) {
        fprintf(stderr, "Invalid input to build_stitched_png\n");
        return 0;
    }

    int total_height = 0;
    for (int i = 0; i < file_count; ++i) {
        total_height += heights[i];
    }

    int scanline_len = 1 + width * 4;
    U64 compressed_len = 0;
    U8* idat_data = build_idat(raw_data_array, heights, file_count, scanline_len, &compressed_len);
    if (!idat_data) return 0;

    chunk_p new_IHDR = malloc(sizeof(struct chunk));
    new_IHDR->length = 13;
    memcpy(new_IHDR->type, "IHDR", 4);
    new_IHDR->p_data = malloc(13);

    memcpy(new_IHDR->p_data, pngs[0]->p_IHDR->p_data, 13);
    U32 new_height_net = htonl(total_height);
    memcpy(new_IHDR->p_data + 4, &new_height_net, 4);
    new_IHDR->crc = calculate_chunk_crc(new_IHDR);

    chunk_p new_IDAT = malloc(sizeof(struct chunk));
    new_IDAT->length = compressed_len;
    memcpy(new_IDAT->type, "IDAT", 4);
    new_IDAT->p_data = idat_data;
    new_IDAT->crc = calculate_chunk_crc(new_IDAT);

    chunk_p new_IEND = malloc(sizeof(struct chunk));
    new_IEND->length = 0;
    memcpy(new_IEND->type, "IEND", 4);
    new_IEND->p_data = NULL;
    new_IEND->crc = crc((U8*)"IEND", 4);

    simple_PNG_p out = malloc(sizeof(struct simple_PNG));
    out->p_IHDR = new_IHDR;
    out->p_IDAT = new_IDAT;
    out->p_IEND = new_IEND;

    int result = write_PNG((char*)output_path, out);
    free_simple_png(out);
    return result;

}

int cat_png(const char** input_paths, int file_count, const char* out_path) {
    if (!out_path || !input_paths || file_count <= 0) return 0;

    simple_PNG_p* pngs = load_png(input_paths, file_count);
    if (!pngs) {
        fprintf(stderr, "Failed to load PNGs\n");
        return 0;
    }

    data_IHDR_p* ihdrs = malloc(sizeof(data_IHDR_p) * file_count);
    U8** raw_data_array = malloc(sizeof(U8*) * file_count);
    int* heights = malloc(sizeof(int) * file_count);

    int width;
    for (int i = 0; i < file_count; ++i) {
        ihdrs[i] = malloc(sizeof(struct data_IHDR));
        get_png_data_IHDR(ihdrs[i], pngs[i]->p_IHDR);

        if (i == 0) width = ihdrs[i]->width;

        uLongf len;
        raw_data_array[i] = extract_scanlines(pngs[i], ihdrs[i], &len);
        heights[i] = ihdrs[i]->height;
    }

    int result = build_stitched_png(out_path, pngs, ihdrs, raw_data_array, heights, file_count, width);

    for (int i = 0; i < file_count; ++i) {
        free(ihdrs[i]);
        free(raw_data_array[i]);
        free_simple_png(pngs[i]);
    }

    free(pngs);
    free(ihdrs);
    free(raw_data_array);
    free(heights);

    return result;
}