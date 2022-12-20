#include <operations.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>


#define THREAD_COUNT 100
#define MAX_OPEN_FILES 1000


void *test(){
    char *str_ext_file = "BBB!";
    char *path_copied_file = "/f1";
    char *path_src = "tests/file_to_copy.txt";
    char buffer[40];


    int f;
    ssize_t r;

    assert(tfs_copy_from_external_fs(path_src, path_copied_file)!= -1);

    f = tfs_open(path_copied_file, TFS_O_CREAT);
    assert(f != -1);

    r = tfs_read(f, buffer, sizeof(buffer) - 1);
    assert(r == strlen(str_ext_file));
    assert(!memcmp(buffer, str_ext_file, strlen(str_ext_file)));

    // Repeat the copy to the same file
    f = tfs_copy_from_external_fs(path_src, path_copied_file);
    assert(f != -1);

    f = tfs_open(path_copied_file, TFS_O_CREAT);
    assert(f != -1);

    // Contents should be overwriten, not appended
    r = tfs_read(f, buffer, sizeof(buffer) - 1);
    assert(r == strlen(str_ext_file));
    assert(!memcmp(buffer, str_ext_file, strlen(str_ext_file)));

    return NULL;
}

int main(){
    tfs_params params = tfs_default_params();
    params.max_open_files_count = MAX_OPEN_FILES;
    pthread_t thread[THREAD_COUNT];
    assert(tfs_init(&params) != -1);

    for (int i = 0; i < 100; i++){
        if (pthread_create(&thread[i], NULL, test, NULL) != 0) {return -1;}
    }

    for (int i = 0; i < 100; i++){
        if (pthread_join(thread[i], NULL) != 0) {return -1;}
    }
    printf("Successful test.\n");
    return 0;
}