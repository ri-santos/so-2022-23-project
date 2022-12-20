#include <operations.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>


int main(){
    char *str_ext_file = 
    "contentcontentcontentcontentcontentcontentcontentcontentcontent    contentcontentcontentcontentcontentcontentcontent contentcontentcontentcontentcontentcontentcontentcontentcontent    contentcontentcontentcontentcontentcontentcontent contentcontentcontentcontentcontentcontent   contentcontentcontentcontent"
    "contentcontentcontentcontentcontentcontentcontentcontentcontent    contentcontentcontentcontentcontentcontentcontent contentcontentcontentcontentcontentcontent   contentcontentcontentcontent"
    "contentcontentcontentcontentcontentcontentcontentcontentcontent    contentcontentcontentcontentcontentcontentcontent contentcontentcontentcontentcontentcontent   contentcontentcontentcontent"
    "contentcontentcontentcontentcontentcontentcontentcontentcontent    contentcontentcontentcontentcontentcontentcontent contentcontentcontentcontentcontentcontent   contentcontentcontentcontent"
    "contentcontentcontentcontentcontentcontentcontentcontentcontent    contentcontentcontentcontentcontentcontentcontent contentcontentcontentcontentcontentcontent   contentcontentcontentcontent"
    "contentcontentcontentcontentcontentcontentcontentcontentcontent    contentcontentcontentcontentcontentcontentcontent contentcontentcontentcontentcontentcontent   contentcontentcontentcontent"
    "contentcontentcontentcontentcontentcontentcontentcontentcontent    contentcontentcontentcontentcontentcontentcontent contentcontentcontentcontentcontentcontent   contentcontentcontentcontent"

    ;
    char *path_copied_file = "/f1";
    char *path_src = "tests/large_file.txt";
    char buffer[4000];

    assert(tfs_init(NULL) != -1);

    int f;
    ssize_t r;

    f = tfs_copy_from_external_fs(path_src, path_copied_file);
    assert(f != -1);

    f = tfs_open(path_copied_file, TFS_O_CREAT);
    assert(f != -1);

    r = tfs_read(f, buffer, sizeof(buffer));
    assert(r != strlen(str_ext_file));
    assert(memcmp(buffer, str_ext_file, strlen(buffer)));

    printf("\033[1m\033[92mSuccessful test.\033[0m\n");

    return 0;
}