#include <operations.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

#define THREAD_COUNT 23


char const file_content[] = "content";
char buffer[sizeof(file_content)];


void* thread_test() {
    int f = tfs_open("/target1", 0);
    assert(f != -1);
    
    ssize_t r = tfs_read(f, buffer, sizeof(buffer));
    assert(r == strlen(file_content));
    assert(!memcmp(buffer, file_content, strlen(file_content)));

    assert(tfs_close(f) != -1);
    return NULL;
}


int main() {
    pthread_t tid[THREAD_COUNT];
    tfs_params params = tfs_default_params();
    
    assert(tfs_init(&params) != -1);

    int f = tfs_open("/target1", TFS_O_CREAT);
    assert(f != -1);

    ssize_t r = tfs_write(f, file_content, strlen(file_content));
    assert(r != -1);

    assert(tfs_close(f) != -1);

    for( int i = 0; i < THREAD_COUNT; i++) {
        if (pthread_create(&tid[i], NULL, thread_test, NULL) != 0) {
            return -1; 
        }
    }

    for( int i = 0; i < THREAD_COUNT; i++) {
        if (pthread_join(tid[i], NULL) != 0) { 
            return -1; 
        }
    }

    printf("\033[1m\033[92mSuccessful test.\033[0m\n");
    return 0;
}


