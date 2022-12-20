#include <operations.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

#define THREAD_COUNT 23


char const str[] = "content\n";
char buffer[sizeof(str)];


void* thread_test() {
    int f = tfs_open("/target1", TFS_O_APPEND);
    assert(f != -1);
    
    ssize_t r = tfs_write(f, str, strlen(str));
    assert(r != -1);

    assert(tfs_close(f) != -1);
    return NULL;
}


int main() {
    pthread_t tid[THREAD_COUNT];
    tfs_params params = tfs_default_params();
    
    assert(tfs_init(&params) != -1);

    int f = tfs_open("/target1", TFS_O_CREAT);
    assert(f != -1);

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

    f = tfs_open("/target1", 0);
    assert(f != -1);
    
    ssize_t r = tfs_read(f, buffer, sizeof(buffer));
    assert( r == strlen(str) * THREAD_COUNT );
    // assert(!memcmp(buffer, str, strlen(str)));

    assert(tfs_close(f) != -1);

    printf("Successful test.\n");
    return 0;
}