#include "lab_png.h"
#include <stdio.h>
#include <stdint.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>  /* for exit().    man 3 exit   */
#include <string.h>  /* for strcat().  man strcat   */
#include <crc.h>
int is_png(U8 *buf, size_t n){
    const uint8_t png_signature[8] = {0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A};
    for (int i = 0; i < n; i++){
        if (buf[i] != png_signature[i]) return 0;   // PNG signature not found
    }
    return 1; // PNG signature found
}

int get_png_data_IHDR(struct data_IHDR *out, FILE *fp, long offset, int whence){
    // function returns -1 in case of any error, probably EOR error
    fseek(fp, offset, whence);
    if (fread(&(out->width), 4, 1, fp) != 1) return 0;
    out->width = ntohl(out->width); // convert to little endian
    if (fread(&(out->height), 4, 1, fp) != 1) return 0;
    out->height = ntohl(out->height); // same thing here
    if (fread(&(out->bit_depth), 1, 1, fp) != 1) return 0;
    if (fread(&(out->color_type), 1, 1, fp) != 1) return 0;
    if (fread(&(out->compression), 1, 1, fp) != 1) return 0;
    if (fread(&(out->filter), 1, 1, fp) != 1) return 0;
    if (fread(&(out->interlace), 1, 1, fp) != 1) return 0;
    return 1; // Process was successful
}

int get_png_height(struct data_IHDR *buf){
    return (int) buf->height;
}

int get_png_width(struct data_IHDR *buf){
    return (int) (buf->width);
}

int get_png_chunks(simple_PNG_p out, FILE* fp, long offset, int whence){
    if (fseek(fp, offset, whence) != 0) {
        perror("fseek failed");
        return 1;
    }
    // now fp should be set to beginning of IHDR
    if (!(out->p_IHDR = get_chunk(fp))){
        printf("Failed to read IHDR chunk \n");
        return 1;
    }
    if (!(out->p_IDAT = get_chunk(fp))){
        printf("Failed to read IDAT chunk \n");
        return 1;
    }
    if (!(out->p_IEND = get_chunk(fp))){
        printf("Failed to read IEND chunk \n");
        return 1;
    }
    return 0;
}

chunk_p get_chunk(FILE *fp){
    chunk_p c1 = malloc(sizeof(struct chunk));
    if (!c1){
        perror("malloc failed");
        return NULL;
    }
    fread(&(c1->length), 1, 4, fp);
    c1->length = ntohl(c1->length);
    // 2. Assign to type, data and crc respectively
    fread(&(c1->type), 1, 4, fp);
    c1->p_data = malloc(c1->length);
    if (!c1->p_data){
        free(c1);
        return NULL;
    }
    fread(c1->p_data, 1, c1->length, fp);
    fread(&(c1->crc), 1, 4, fp);
    c1->crc = ntohl(c1->crc);
    return c1;
}

U32 get_chunk_crc(chunk_p in){
    // Get pointer directly
    return in->crc;
}

U32 calculate_chunk_crc(chunk_p in){
    size_t len = 4 + in->length;
    unsigned char *buf = malloc(len);
    if (!buf) return 0; 
    // Copy type (4 bytes) and data into buffer
    memcpy(buf, in->type, 4); // First 4 bytes: chunk type
    memcpy(buf + 4, in->p_data, in->length); // Remaining bytes: chunk data
    
    U32 calculated_crc = crc(buf, len);
    free(buf);
    return calculated_crc;
}

simple_PNG_p mallocPNG(){
    simple_PNG_p myPNG = malloc(sizeof(struct simple_PNG));
    return myPNG;
}

void free_png(simple_PNG_p in){
    if (!in) return;
    if (in->p_IHDR) free_chunk(in->p_IHDR);
    if (in->p_IDAT) free_chunk(in->p_IDAT);
    if (in->p_IEND) free_chunk(in->p_IEND);
    free(in);
}

void free_chunk(chunk_p in){
    if (!in) return;
    if (in->p_data != NULL){
        free(in->p_data);
        in->p_data = NULL;
    }
    free(in);
}

int write_PNG(char* filepath, simple_PNG_p in){
    FILE *fp = fopen(filepath, "wb");
    if (!fp){
        perror("Failed to write PNG\n");
        return 1;
    }
    U8 png_signature[8] = {0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A};
    if (fwrite(png_signature, 1, PNG_SIG_SIZE, fp) != PNG_SIG_SIZE)
    {
        fclose(fp);
        return 1;
    }
    if (write_chunk(fp, in->p_IHDR) != 0) return 1;
    if (write_chunk(fp, in->p_IDAT) != 0) return 1;
    if (write_chunk(fp, in->p_IEND) != 0) return 1;
    fclose(fp);
    return 0;
}

int write_chunk(FILE* fp, chunk_p chunk){
    U32 n_len = htonl(chunk->length);
    U32 n_crc = htonl(chunk->crc);
    if (fwrite(&n_len, 1, 4, fp) != 4) return 1;
    if (fwrite(chunk->type, 1, 4, fp) != 4) return 1;
    if (fwrite(chunk->p_data, 1, chunk->length, fp) != chunk->length) return 1;
    if (fwrite(&n_crc, 1, 4, fp) != 4) return 1;
    return 0;
}

// is_fname method
void is_fname(int argc, char *argv[]){
    DIR *p_dir;
    struct dirent *p_dirent;
    char str[64];

    if (argc == 1) {
        fprintf(stderr, "Usage: %s <directory name>\n", argv[0]);
        exit(1);
    }

    if ((p_dir = opendir(argv[1])) == NULL) {
        sprintf(str, "opendir(%s)", argv[1]);
        perror(str);
        exit(2);
    }

    while ((p_dirent = readdir(p_dir)) != NULL) {
        char *str_path = p_dirent->d_name;  /* relative path name! */

        if (str_path == NULL) {
            fprintf(stderr,"Null pointer found!"); 
            exit(3);
        } else {
            printf("%s\n", str_path);
        }
    }

    if ( closedir(p_dir) != 0 ) {
        perror("closedir");
        exit(3);
    }
    return;
}

void is_ftype(int argc, char *argv[]){
    int i;
    char *ptr;
    struct stat buf;

    for (i = 1; i < argc; i++) {
        printf("%s: ", argv[i]);
        if (lstat(argv[i], &buf) < 0) {
            perror("lstat error");
            continue;
        }   

        if      (S_ISREG(buf.st_mode))  ptr = "regular";
        else if (S_ISDIR(buf.st_mode))  ptr = "directory";
        else if (S_ISCHR(buf.st_mode))  ptr = "character special";
        else if (S_ISBLK(buf.st_mode))  ptr = "block special";
        else if (S_ISFIFO(buf.st_mode)) ptr = "fifo";
#ifdef S_ISLNK
        else if (S_ISLNK(buf.st_mode))  ptr = "symbolic link";
#endif
#ifdef S_ISSOCK
        else if (S_ISSOCK(buf.st_mode)) ptr = "socket";
#endif
        else                            ptr = "**unknown mode**";
        printf("%s\n", ptr);
    }
    return;
}

const char* get_filename(const char *path){
    const char *slash = strrchr(path, '/');
    return (slash) ? slash + 1 : path;
}

