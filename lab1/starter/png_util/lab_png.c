#include "lab_png.h"
#include "crc.h"


int is_png(U8 *buf, size_t n) {
    const U8 png_sign[8] = {0x89,'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A};
    if(n < 8){
        return 0;
    }
    for(int i = 0; i < 8; ++i) {
        if(buf[i] != png_sign[i]) {
            return 0;
        }
    }
    return 1;
}

int get_png_data_IHDR(struct data_IHDR *out, FILE *fp, long offset, int whence) {
    if(fseek(fp, offset + 8, whence) != 0) {
        return 0;
    }

    int ok = 1;
    U32 buf;
    ok &= fread(&buf, 1, 4, fp) == 4;
    out->height = ntohl(buf);
    ok &= fread(&buf, 1, 4, fp) == 4;
    out->width = ntohl(buf);
    ok &= fread(&out->bit_depth, 1, 1, fp) == 1;
    ok &= fread(&out->color_type, 1, 1, fp) == 1;
    ok &= fread(&out->compression, 1, 1, fp) == 1;
    ok &= fread(&out->filter, 1, 1, fp) == 1;
    ok &= fread(&out->interlace, 1, 1, fp) == 1;

    return ok;
}

int get_png_height(struct data_IHDR *buf) {
    return buf->height;
}

int get_png_width(struct data_IHDR *buf) {
    return buf->width;
}
chunk_p get_chunk(FILE *fp) {
    chunk_p chunk = malloc(sizeof(struct chunk));
    if(!chunk){
        printf("Failed to create chunk");
        return NULL;
    }

    if(fread(&chunk->length, 1, 4, fp) != 4) {
        printf("Failed to read chunk length\n");
        free(chunk);
        return NULL;
    }
    chunk->length = ntohl(chunk->length);

    if(fread(&chunk->type, 1, 4, fp) != 4) {
        printf("Failed to read chunk type\n");
        free(chunk);
        return NULL;
    }

    if (chunk->length > 0) {
        chunk->p_data = malloc(chunk->length);
        if (!chunk->p_data) {
            printf("Failed to allocate chunk data\n");
            free(chunk);
            return NULL;
        }

        if (fread(chunk->p_data, 1, chunk->length, fp) != chunk->length) {
            printf("Failed to read chunk data of length %u\n", chunk->length);
            free(chunk->p_data);
            free(chunk);
            return NULL;
        }
    } else {
        chunk->p_data = NULL; 
    }

    if(fread(&chunk->crc, 1, 4, fp) != 4) {
        printf("Failed to read chunk crc\n");
        free(chunk);
        return NULL;
    }
    chunk->crc = ntohl(chunk->crc);

    return chunk;
}

int get_png_chunks(simple_PNG_p out,FILE *fp, long offset, int whence) {
    if (fseek(fp, offset, whence) != 0) {
        return -1;
    }
    
    out->p_IHDR = get_chunk(fp);
    if(!out->p_IHDR || strncmp((const char *)out->p_IHDR->type, "IHDR", 4) != 0) {
        return -2;
    }

    out->p_IDAT = get_chunk(fp);
    if(!out->p_IDAT || strncmp((const char *)out->p_IDAT->type, "IDAT", 4) != 0) {
        return -3;
    }

    out->p_IEND = get_chunk(fp);
    if(!out->p_IEND || strncmp((const char *)out->p_IEND->type, "IEND", 4) != 0) {
        return -4;
    }

    return 1;
}

U32 get_chunk_crc(chunk_p in) {
    return in->crc;
}

U32 calculate_chunk_crc(chunk_p in) {
    int total_len = 4 + in->length;
    U8* buf = malloc(total_len);
    if(!buf) return 0;
    
    memcpy(buf, in->type, 4);
    if (in->length > 0 && in->p_data) {
        memcpy(buf + 4, in->p_data, in->length);
    }

    U32 result = crc(buf, total_len);
    free(buf);
    return result;
}

void free_simple_png(simple_PNG_p img) {
    free(img->p_IHDR->p_data); 
    free(img->p_IHDR);
    free(img->p_IDAT->p_data); 
    free(img->p_IDAT);
    free(img->p_IEND->p_data); 
    free(img->p_IEND);
    free(img);
}


