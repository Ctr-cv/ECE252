#include <catpng.h>

int cat(int argc, char *argv[]){
    if (argc < 2) { // housekeeping ensures at least 1 argument to concatenate
        fprintf(stderr, "Usage: %s <png_file1> [png_file2 ...]\n", argv[0]);
        return 1;
    }
    // Initialize some parameters as needed
    simple_PNG_p allPNG = mallocPNG();
    // Buffer for concatenated data
    U8 *combined_data = NULL;
    size_t combined_data_size = 0;
    // Records final height for IDAT chunk
    U32 total_height = 0;
    // holds final data_IHDR chunk, needs to be updated
    data_IHDR_p b_ihdr = NULL;

    for (int i = 1; i < argc; i++){
        const char *filename = argv[i];
        FILE *fp = fopen(filename, "rb");
        if (!fp){ // check fopen
            perror(filename);
            continue;
        }
        
        simple_PNG_p cur_png = mallocPNG();
        // error check + get all three chunks from cur png
        if (!cur_png || get_png_chunks(cur_png, fp, 8, SEEK_SET) != 0){
            fprintf(stderr, "Couldn't parse PNG: %s\n", argv[i]);
            if (cur_png) free(cur_png);
            fclose(fp);
            return 1;
        }
        // Initialize the new data_ihdr for allPNG
        data_IHDR_p cur_ihdr = malloc(sizeof(struct data_IHDR));
        if (!cur_ihdr) {
            perror("malloc failed for cur_ihdr");
            free_png(cur_png);
            fclose(fp);
            return 1;
        }
        memcpy(cur_ihdr, cur_png->p_IHDR->p_data, DATA_IHDR_SIZE);
        if (i == 1) b_ihdr = cur_ihdr;
        else{
            if (get_png_width(cur_ihdr) != get_png_width(b_ihdr)){
                printf("PNGs do not have the same width\n");
                free_png(cur_png);
                fclose(fp);
                return 1;
            }
        }
        
        // Decompress the IDAT data, then connect to combined data
        // First initialize the parameters for mem_inf
        U64 dest_len = 10*1024*1024;
        U8 *dest = malloc(dest_len); //  allocate destination memory
        if (!dest){
            perror("decompression malloc failed\n");
            free_png(cur_png);
            fclose(fp);
            return 1;
        }
        U64 source_len = ntohl(cur_png->p_IDAT->length); // define compressed data length
        if (mem_inf(dest, &dest_len, cur_png->p_IDAT->p_data, source_len) != 0){ // -3 is Z_DATA_ERROR
            fprintf(stderr, "Failed to decompress IDAT for %s\n", argv[i]);
            free(dest);
            free_png(cur_png);
            fclose(fp);
            return 1;
        }
        // reallocate combined data first, copy over
        combined_data = realloc(combined_data, combined_data_size + dest_len);
        if (!combined_data){
            perror("Failed to realloc for combined data\n");
            free_png(cur_png);
            free(dest);
            fclose(fp);
            return 1;
        }
        memcpy(combined_data + combined_data_size, dest, dest_len);
        combined_data_size += dest_len;
        total_height += ntohl(get_png_height(cur_ihdr));
        // now combined data contains all necessary (decompressed) data.
        fclose(fp);
        free_png(cur_png);
        if (i != 1) free(cur_ihdr);
        free(dest);
    }
    if (!b_ihdr){
        fprintf(stderr, "No valid PNG data processed\n");
        free_png(allPNG);
        return 1;
    }
    printf("total_height: %d, combined data size: %ld\n", total_height, combined_data_size);
    // Update the base IHDR height
    b_ihdr->height = htonl(total_height);

    // Now we start compressing everything
    U64 dest_len = combined_data_size + 1000;
    U8 *dest = malloc(dest_len); //  allocate destination memory
    if (!dest){
        perror("Compression malloc failed\n");
        free(combined_data);
        free_png(allPNG);
        return 1;
    }
    if (mem_def(dest, &dest_len, combined_data, combined_data_size, 6) != 0){
        fprintf(stderr, "Failed to compress IDAT \n");
        free(dest);
        free(combined_data);
        free_png(allPNG);
        return 1;
    }
    free(combined_data);
    // dest buffer contains final IDAT->p_data
    // Build all the chunks
    chunk_p IHDR_chunk = malloc(sizeof(struct chunk));
    chunk_p IDAT_chunk = malloc(sizeof(struct chunk));
    chunk_p IEND_chunk = malloc(sizeof(struct chunk));
    if (!IHDR_chunk || !IDAT_chunk || !IEND_chunk){
        perror("malloc failed");
        if (IHDR_chunk) free(IHDR_chunk);
        if (IDAT_chunk) free(IHDR_chunk);
        if (IEND_chunk) free(IHDR_chunk);
        free(dest);
        free_png(allPNG);
        return 1;
    }
    IHDR_chunk->length = DATA_IHDR_SIZE;
    memcpy(IHDR_chunk->type, "IHDR", 4);
    IHDR_chunk->p_data = (U8 *)b_ihdr; // cast this thing
    IHDR_chunk->crc = calculate_chunk_crc(IHDR_chunk);
    IDAT_chunk->length = dest_len;
    memcpy(IDAT_chunk->type, "IDAT", 4);
    IDAT_chunk->p_data = dest;
    IDAT_chunk->crc = calculate_chunk_crc(IDAT_chunk);
    IEND_chunk->length = 0;
    memcpy(IEND_chunk->type, "IEND", 4);
    IEND_chunk->p_data = NULL;
    IEND_chunk->crc = calculate_chunk_crc(IEND_chunk);
    printf("Debug check: IDAT chunk length: %ld\n", dest_len);
    // Finally, write to a whole png
    allPNG->p_IHDR = IHDR_chunk;
    allPNG->p_IDAT = IDAT_chunk;
    allPNG->p_IEND = IEND_chunk;
    write_PNG("./all.png", allPNG);
    free_png(allPNG);
    return 0;
}