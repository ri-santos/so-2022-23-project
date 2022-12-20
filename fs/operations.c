#include "operations.h"
#include "config.h"
#include "state.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "betterassert.h"


tfs_params tfs_default_params() {
    tfs_params params = {
        .max_inode_count = 64,
        .max_block_count = 1024,
        .max_open_files_count = 16,
        .block_size = 1024,
    };
    return params;
}

int tfs_init(tfs_params const *params_ptr) {
    tfs_params params;
    if (params_ptr != NULL) {
        params = *params_ptr;
    } else {
        params = tfs_default_params();
    }

    if (state_init(params) != 0) {
        return -1;
    }

    // create root inode
    int root = inode_create(T_DIRECTORY);
    if (root != ROOT_DIR_INUM) {
        return -1;
    }

    return 0;
}

int tfs_destroy() {
    if (state_destroy() != 0) {
        return -1;
    }
    return 0;
}

static bool valid_pathname(char const *name) {
    return name != NULL && strlen(name) > 1 && name[0] == '/';
}

/**
 * Looks for a file.
 *
 * Note: as a simplification, only a plain directory space (root directory only)
 * is supported.
 *
 * Input:
 *   - name: absolute path name
 *   - root_inode: the root directory inode
 * Returns the inumber of the file, -1 if unsuccessful.
 */
static int tfs_lookup(char const *name, inode_t const *root_inode) {
    // TODO: assert that root_inode is the root directory
    if (!valid_pathname(name)) {
        return -1;
    }

    // skip the initial '/' character
    name++;

    int handle = find_in_dir(root_inode, name);
    return handle;
}

int tfs_open(char const *name, tfs_file_mode_t mode) {
    // Checks if the path name is valid
    if (!valid_pathname(name)) {
        return -1;
    }

    inode_t *root_dir_inode = inode_get(ROOT_DIR_INUM);
    dir_entry_t* dir = (dir_entry_t*)data_block_get(root_dir_inode->i_data_block);
    ALWAYS_ASSERT(root_dir_inode != NULL,
                  "tfs_open: root dir inode must exist");
    if(lock_dir_read(dir) != 0){ return -1; }
    int inum = tfs_lookup(name, root_dir_inode);
    size_t offset;
    if(unlock_dir(dir) != 0) { return -1; }


    if (inum >= 0) {
        // The file already exists
        if(lock_dir_read(dir) != 0){ return -1; }
        inode_t *inode = inode_get(inum);
        ALWAYS_ASSERT(inode != NULL,
                    "tfs_open: directory files must have an inode");
        if(unlock_dir(dir) != 0){ return -1; }

        if (inode->i_node_type == T_SOFTLINK){
            if(lock_inode_read(inode) != 0){ return -1; }
            void *i_data_block = data_block_get(inode->i_data_block);
            if(unlock_inode(inode) != 0){ return -1; }
            return tfs_open(i_data_block, mode);
        }
        // Truncate (if requested)

        if (mode & TFS_O_TRUNC) {
            if (inode->i_size > 0) {
                if(lock_inode_write(inode) != 0){ return -1; }
                data_block_free(inode->i_data_block);
                inode->i_size = 0;
                if(unlock_inode(inode) != 0){ return -1; }
            }
        }
        // Determine initial offset
        if(lock_inode_read(inode) != 0){ return -1; }
        if (mode & TFS_O_APPEND) {
            offset = inode->i_size;
        } else {
            offset = 0;
        }
        if(unlock_inode(inode) != 0){ return -1; }
    } else if (mode & TFS_O_CREAT) {
        // The file does not exist; the mode specified that it should be created
        // Create inode
        if(lock_dir_write(dir) != 0){ return -1; }
        inum = inode_create(T_FILE);
        if(unlock_dir(dir) != 0){ return -1; }
        if (inum == -1) {
            return -1; // no space in inode table
        }

        // Add entry in the root directory
        if(lock_dir_write(dir) != 0){ return -1; }
        int entry = add_dir_entry(root_dir_inode, name + 1, inum);
        if(unlock_dir(dir) != 0){ return -1; }
        if (entry == -1) {
            if(lock_dir_write(dir) != 0){ return -1; }
            inode_delete(inum);
            if(unlock_dir(dir) != 0){ return -1; }
            return -1; // no space in directory
        }

        offset = 0;
    } else {
        return -1;
    }

    // Finally, add entry to the open file table and return the corresponding
    // handle

    if(lock_open_file_write() != 0){ return -1; }
    int handle = add_to_open_file_table(inum, offset);
    if(unlock_open_file() != 0){ return -1; }
    return handle;

    // Note: for simplification, if file was created with TFS_O_CREAT and there
    // is an error adding an entry to the open file table, the file is not
    // opened but it remains created
}

int tfs_sym_link(char const *target, char const *link_name) {
    inode_t *root_dir_inode = inode_get(ROOT_DIR_INUM);
    if(root_dir_inode == NULL){ return -1; }

    dir_entry_t* dir = (dir_entry_t*)data_block_get(root_dir_inode->i_data_block);

    if(lock_dir_write(dir) != 0){ return -1; }
    int new_inumber = inode_create(T_SOFTLINK);
    inode_t *inode = inode_get(new_inumber);
    
    
    if(lock_inode_read(inode) != 0){ return -1; }
    if (tfs_lookup(target, root_dir_inode) == -1) { return -1; }
    void *data_block_pointer = data_block_get(inode->i_data_block);
    memcpy(data_block_pointer, target, strlen(target));
    if(unlock_inode(inode) != 0){ return -1; }
    
    int handle = add_dir_entry(root_dir_inode, link_name + 1, new_inumber);
    if(unlock_dir(dir) != 0){ return -1; }
    if(handle == -1){
        printf("handle\n");
        return -1;
    }
    return 0;
}

int tfs_link(char const *target, char const *link_name) {
    inode_t *root_dir_inode = inode_get(ROOT_DIR_INUM);
    if(root_dir_inode == NULL){ return -1; }

    dir_entry_t* dir = (dir_entry_t*)data_block_get(root_dir_inode->i_data_block);
    
    if(lock_dir_read(dir) != 0){ return -1; }
    int i_number = tfs_lookup(target, root_dir_inode);
    if(i_number == -1) { return -1; }
    if(unlock_dir(dir) != 0){ return -1; }
    if(i_number == -1){ return -1; }

    inode_t *inode = inode_get(i_number);
    if(inode->i_node_type == T_SOFTLINK){ return -1; }

    if(lock_dir_write(dir) != 0){ return -1; }
    int handle = add_dir_entry(root_dir_inode, link_name + 1, i_number);
    if(unlock_dir(dir) != 0){ return -1; }

    if(handle == -1){
        return -1;
    }
    if(lock_inode_write(inode) != 0){ return -1; }
    inode->i_counter++;
    if(unlock_inode(inode) != 0) { return -1; }
    
    return 0;
}

int tfs_close(int fhandle) {
    if(lock_open_file_read() != 0) { return -1; }
    open_file_entry_t *file = get_open_file_entry(fhandle);
    if(unlock_open_file() != 0) { return -1; }
    if (file == NULL) {
        return -1; // invalid fd
    }

    if(lock_open_file_write() != 0) { return -1; }
    remove_from_open_file_table(fhandle);
    if(unlock_open_file() != 0) { return -1; }

    return 0;
}

ssize_t tfs_write(int fhandle, void const *buffer, size_t to_write) {
    if(lock_open_file_read() != 0) { return -1; }
    open_file_entry_t *file = get_open_file_entry(fhandle);
    if(unlock_open_file() != 0) { return -1; }
    if (file == NULL) {
        return -1;
    }

    //  From the open file table entry, we get the inode
    if(lock_open_file_read() != 0) { return -1; }
    inode_t *inode = inode_get(file->of_inumber);
    ALWAYS_ASSERT(inode != NULL, "tfs_write: inode of open file deleted");

    // Determine how many bytes to write
    size_t block_size = state_block_size();
    if (to_write + file->of_offset > block_size) {
        to_write = block_size - file->of_offset;
    }
    if(unlock_open_file() != 0) { return -1; }
    if(lock_inode_write(inode) != 0) { return -1; }
    if (to_write > 0) {
        if (inode->i_size == 0) {
            // If empty file, allocate new block
            if(lock_datablock_write() != 0) { return -1; }
            int bnum = data_block_alloc();
            if (bnum == -1) {
                if(unlock_inode(inode) != 0) { return -1; }
                if(unlock_datablock() != 0) { return -1; }
                return -1; // no space
            }

            inode->i_data_block = bnum;
            if(unlock_datablock() != 0) { return -1; }
        }
        
        if(lock_datablock_read() != 0) { return -1; }
        void *block = data_block_get(inode->i_data_block);
        ALWAYS_ASSERT(block != NULL, "tfs_write: data block deleted mid-write");

        // Perform the actual write
        memcpy(block + file->of_offset, buffer, to_write);

        // The offset associated with the file handle is incremented accordingly
        if(lock_open_file_write() != 0) { return -1; }
        file->of_offset += to_write;
        if (file->of_offset > inode->i_size) {
            inode->i_size = file->of_offset;
        }
        if(unlock_datablock() != 0) { return -1; }
        if(unlock_open_file() != 0) { return -1; }
        
    }
    if(unlock_inode(inode) != 0) { return -1; }
    return (ssize_t)to_write;
}

ssize_t tfs_read(int fhandle, void *buffer, size_t len) {
    open_file_entry_t *file = get_open_file_entry(fhandle);
    if (file == NULL) {
        return -1;
    }

    // From the open file table entry, we get the inode
    if(lock_open_file_read() != 0) { return -1; }
    inode_t *inode = inode_get(file->of_inumber);
    if(unlock_open_file() != 0) { return -1; }
    ALWAYS_ASSERT(inode != NULL, "tfs_read: inode of open file deleted");

    // Determine how many bytes to read
    if(lock_inode_write(inode) != 0) { return -1; }
    size_t to_read = inode->i_size - file->of_offset;
    if (to_read > len) {
        to_read = len;
    }

    if (to_read > 0) {
        if(lock_open_file_write() != 0) { return -1; }
        void *block = data_block_get(inode->i_data_block);
        ALWAYS_ASSERT(block != NULL, "tfs_read: data block deleted mid-read");

        // Perform the actual read
        memcpy(buffer, block + file->of_offset, to_read);
        // The offset associated with the file handle is incremented accordingly
        file->of_offset += to_read;
        if(unlock_open_file() != 0) { return -1; }
    }
    if(unlock_inode(inode) != 0) { return -1; }

    return (ssize_t)to_read;
}

int tfs_unlink(char const *target) {
    inode_t *root_dir_inode = inode_get(ROOT_DIR_INUM);
    if(root_dir_inode == NULL){ return -1; }

    dir_entry_t* dir = (dir_entry_t*)data_block_get(root_dir_inode->i_data_block);
    
    if(lock_dir_read(dir) != 0) { return -1; }
    int inumber = tfs_lookup(target, root_dir_inode);
    if(unlock_dir(dir) != 0) { return -1; }
    inode_t *inode = inode_get(inumber);
    if(lock_inode_write(inode) != 0) { return -1; }
    inode_type type = inode->i_node_type;



    if (type == T_FILE){
        inode->i_counter--;
        if (inode->i_counter == 0){
            inode_delete(inumber);
        }
    }
    if (type == T_SOFTLINK){
        data_block_free(inode->i_data_block);
        inode_delete(inumber);
    }
    if(lock_dir_write(dir) != 0) { return -1; }
    int handle = clear_dir_entry(root_dir_inode, target + 1);
    if(unlock_dir(dir) != 0) { return -1; }
    if(unlock_inode(inode) != 0) { return -1; }
    
    return handle;
}

int tfs_copy_from_external_fs(char const *source_path, char const *dest_path) {
    FILE* fd_in = fopen(source_path, "r");
    if(fd_in == NULL){
        return -1;
    }
    
    int fd_out = tfs_open(dest_path, TFS_O_CREAT);
    if(fd_out == -1){
        fclose(fd_in);
        return -1;
    }
    
    size_t block_size = state_block_size();
    char* buffer = (char*)malloc(sizeof(char) * block_size);
    memset(buffer, 0, sizeof(char));

    size_t bytes_read = 0;

    bytes_read = fread(buffer, sizeof(char), block_size, fd_in);
    if(ferror(fd_in)){
        fclose(fd_in);
        tfs_close(fd_out);
        free(buffer);
        return -1;
    }
    
    ssize_t bytes_written = tfs_write(fd_out, buffer, bytes_read);
    if(bytes_written < bytes_read){
        free(buffer);
        return -1;
    }

    fclose(fd_in);
    tfs_close(fd_out);
    free(buffer);
    return 0;
}