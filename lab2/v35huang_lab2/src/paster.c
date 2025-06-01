#include "main_write_header_cb.h"
#include <stdio.h>
#include <stdlib.h>
#include <curl/curl.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <stdbool.h>
#include "../include/catpng.h"

#define NUM_PICTURES 50
#define IMG_URL_1 "http://ece252-1.uwaterloo.ca:2520/image?img="
#define IMG_URL_2 "http://ece252-2.uwaterloo.ca:2520/image?img="
#define IMG_URL_3 "http://ece252-3.uwaterloo.ca:2520/image?img="
int REMAINING = 50;
int global_array[NUM_PICTURES] = {0};
char **images;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

int getimg(int argc) {
    CURL *curl_handle;
    CURLcode res;
    char url[256];
    RECV_BUF recv_buf;
    char fname[256];

    recv_buf_init(&recv_buf, 1048576);
    srand(time(NULL));
    int index = rand() % 3;
    // Modified logic here
    if (index == 0) {
        strcpy(url, IMG_URL_1); 
    } else if (index == 1){
        strcpy(url, IMG_URL_2);
    } else {
        strcpy(url, IMG_URL_3);
    }
    size_t len = strlen(url);
    if (len + 1 < sizeof(url)){
        snprintf(url + len, sizeof(url) - len, "%d", argc);
    } else printf("Char buffer is full\n");
    printf("URL is %s\n", url);

    curl_global_init(CURL_GLOBAL_DEFAULT);

    /* init a curl session */
    curl_handle = curl_easy_init();

    if (curl_handle == NULL) {
        fprintf(stderr, "curl_easy_init: returned NULL\n");
        return 1;
    }

    /* specify URL to get */
    curl_easy_setopt(curl_handle, CURLOPT_URL, url);

    /* register write call back function to process received data */
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_cb_curl3); 
    /* user defined data structure passed to the call back function */
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&recv_buf);

    /* register header call back function to process received header data */
    curl_easy_setopt(curl_handle, CURLOPT_HEADERFUNCTION, header_cb_curl); 
    /* user defined data structure passed to the call back function */
    curl_easy_setopt(curl_handle, CURLOPT_HEADERDATA, (void *)&recv_buf);

    /* some servers requires a user-agent field */
    curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");

    
    /* get it! */
    res = curl_easy_perform(curl_handle);

    if( res != CURLE_OK) {
        fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        return 1;
    } else {
	printf("%lu bytes received in memory %p, seq=%d.\n",
        recv_buf.size, recv_buf.buf, recv_buf.seq);
    }
    bool exists = false;
    pthread_mutex_lock(&lock);
    if (global_array[recv_buf.seq] != 0){
        printf("Image already exists, skipping...\n");
        exists = true;
    } else {
        global_array[recv_buf.seq] = 1;
        REMAINING--;

        sprintf(fname, "./output_%d.png", recv_buf.seq);
        write_file(fname, recv_buf.buf, recv_buf.size);
        printf("%d remaining\n", REMAINING);
    }
    pthread_mutex_unlock(&lock);
    curl_easy_cleanup(curl_handle);
    curl_global_cleanup();
    recv_buf_cleanup(&recv_buf);
    return exists ? 1 : 0;
}

void* thread_exe(void* arg){
    int pic_num = *(int*) arg;
    free(arg);

    while (1) {
        // protect REMAINING read access
        pthread_mutex_lock(&lock);
        if (REMAINING <= 0) {
            pthread_mutex_unlock(&lock);
            break;
        }
        pthread_mutex_unlock(&lock);
        if (getimg(pic_num) != 0) {
            printf("error, continuing:\n");
        }
    }
    return NULL;
}

int main(int argc, char **argv){
    // tracks picture number and # of threads
    int pic = 1; int thr = 1;
    srand(time(NULL));
    if (argc == 3){
        if (strcmp(argv[1], "-t") == 0){
            thr = atoi(argv[2]);
        } else {
            pic = atoi(argv[2]);
        }
    } else if (argc >= 5){
        if (strcmp(argv[1], "-t") == 0){
            thr = atoi(argv[2]);
            pic = atoi(argv[4]);
        } else {
            pic = atoi(argv[2]);
            thr = atoi(argv[4]);
        }
    } else if (argc != 1){
        fprintf(stderr, "Too many/too little parameters present\n");
        return 1;
    }
    if (thr > 1) thr--;
    // tracks all thread ids
    pthread_t threads[thr];

    // tracks outer condition when image is completed
    for (int i = 0; i < thr; i++){
        int* arg = malloc(sizeof(int));
        if (!arg){
            fprintf(stderr, "malloc failed\n");
            return 1;
        }
        *arg = pic;
        if (pthread_create(&threads[i], NULL, thread_exe, arg) != 0){
            perror("pthread_create error");
            free(arg);
            return 1;
        }
    }
    // Waits for all threads to stop executing before continuing
    for (int i = 0; i < thr; i++){
        pthread_join(threads[i], NULL);
    }
    int imagenum = NUM_PICTURES + 1; // add 1 for calling the exe itself
    static char image_storage[NUM_PICTURES][20]; // 20 is the space for the strings
    char *images[NUM_PICTURES + 2];
    for (int i = 0; i < NUM_PICTURES; i++) {
        snprintf(image_storage[i], sizeof(image_storage[i]), "output_%d.png", i);
        images[i+1] = image_storage[i];
    }
    images[0] = "./paster";
    images[NUM_PICTURES + 1] = NULL;
    if (cat(imagenum, images) != 0){
        printf("catpng failed, returning...\n");
        return 1;
    }
    return 0;
}