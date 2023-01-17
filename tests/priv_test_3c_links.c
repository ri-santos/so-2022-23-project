#include <operations.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#define THREAD_COUNT 10
#define BLOCK_SIZE 3000

/*
    Creates multiple threads that write in a file, create a hard link to that inode, then delete the initial hard link 
    and finally create a soft link to the second hard link and checks if the content is obtainable through the soft link.
*/


char const str[] = "content\n";
char buffer[sizeof(str)];

char const target_path1[] = "/original";
char const link_path1[] = "/hardlink";
char const link_path2[] = "/softlink";

void* thread_test() {
    int f = tfs_open(target_path1, TFS_O_CREAT);
    assert(f != -1);

    assert(tfs_write(f, str, strlen(str)) != -1);

    assert(tfs_link(target_path1, link_path1) != -1);

    assert(tfs_close(f) != -1);

    assert(tfs_unlink(target_path1) != -1);

    assert(tfs_sym_link(link_path1, link_path2) != -1);

    return NULL;
}


int main() {
    pthread_t tid[THREAD_COUNT];
    tfs_params params = tfs_default_params();
    params.block_size = BLOCK_SIZE;
    
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

    int f1 = tfs_open(link_path1, 0);
    assert(f1 != -1);
    
    int f2 = tfs_open(link_path2, 0);
    assert(f2 != -1);

    ssize_t r1 = tfs_read(f1, buffer, sizeof(buffer));
    ssize_t r2 = tfs_read(f2, buffer, sizeof(buffer));
    assert( r1 == r2 );

    assert(tfs_close(f1) != -1);
    assert(tfs_close(f2) != -1);

    printf("\033[1m\033[92mSuccessful test.\033[0m\n");
    return 0;
}    