#include <operations.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>


/*
    Creates two hard links to the same inode, then a soft link to the first hard link, unlinks the first hard link
    and checks if the content is still obtainable through the soft link (shouldn't be ).
*/


char const file_content[] = "content";
char const target_path1[] = "/f1";
char const link_path1[] = "/l1";
char const link_path2[] = "/l2";
char buffer[sizeof(file_content)];


int main() {
    assert(tfs_init(NULL) != -1);

    int f = tfs_open(target_path1, TFS_O_CREAT);
    assert(f != -1);

    assert(tfs_write(f, file_content, strlen(file_content)) != -1);

    assert(tfs_sym_link(target_path1, link_path1) != -1);

    int slink = tfs_open(link_path1, 0);
    assert(slink != -1);

    ssize_t r1 = tfs_read(slink, buffer, sizeof(buffer));
    assert(r1 != -1);

    assert(tfs_close(f) != -1);

    int hlink = tfs_open(link_path2, TFS_O_CREAT);
    assert(hlink != -1);

    assert(tfs_link(target_path1, link_path2) != -1);

    assert(tfs_close(hlink) != -1);

    assert(tfs_unlink(target_path1) != -1);

    ssize_t r2 = tfs_read(slink, buffer, sizeof(buffer));
    assert(r1 != r2);

    assert(tfs_close(slink) != -1);



    printf("\033[1m\033[92mSuccessful test.\033[0m\n");
    return 0;
}