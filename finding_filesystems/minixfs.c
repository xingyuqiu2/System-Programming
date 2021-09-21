/**
 * finding_filesystems
 * CS 241 - Spring 2021
 */
#include "minixfs.h"
#include "minixfs_utils.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>

/**
 * Virtual paths:
 *  Add your new virtual endpoint to minixfs_virtual_path_names
 */
char *minixfs_virtual_path_names[] = {"info", /* add your paths here*/};

/**
 * Forward declaring block_info_string so that we can attach unused on it
 * This prevents a compiler warning if you haven't used it yet.
 *
 * This function generates the info string that the virtual endpoint info should
 * emit when read
 */
static char *block_info_string(ssize_t num_used_blocks) __attribute__((unused));
static char *block_info_string(ssize_t num_used_blocks) {
    char *block_string = NULL;
    ssize_t curr_free_blocks = DATA_NUMBER - num_used_blocks;
    asprintf(&block_string, "Free blocks: %zd\n"
                            "Used blocks: %zd\n",
             curr_free_blocks, num_used_blocks);
    return block_string;
}

// Don't modify this line unless you know what you're doing
int minixfs_virtual_path_count =
    sizeof(minixfs_virtual_path_names) / sizeof(minixfs_virtual_path_names[0]);

int minixfs_chmod(file_system *fs, char *path, int new_permissions) {
    // Thar she blows!
    inode* cur_inode = get_inode(fs, path);
    if (cur_inode) {
        cur_inode->mode = ((cur_inode->mode >> RWX_BITS_NUMBER) << RWX_BITS_NUMBER) | new_permissions;
        clock_gettime(CLOCK_REALTIME, &cur_inode->ctim);
        return 0;
    }
    //if path doesn't exist, set errno to ENOENT
    errno = ENOENT;
    return -1;
}

int minixfs_chown(file_system *fs, char *path, uid_t owner, gid_t group) {
    // Land ahoy!
    inode* cur_inode = get_inode(fs, path);
    if (cur_inode) {
        //If owner is ((uid_t)-1), then don't change the node's uid.
        if (owner != (uid_t)-1) {
            cur_inode->uid = owner;
        }
        //If group is ((gid_t)-1), then don't change the node's gid.
        if (group != (gid_t)-1) {
            cur_inode->gid = group;
        }
        clock_gettime(CLOCK_REALTIME, &cur_inode->ctim);
        return 0;
    }
    //if path doesn't exist, set errno to ENOENT
    errno = ENOENT;
    return -1;
}

inode *minixfs_create_inode_for_path(file_system *fs, const char *path) {
    // Land ahoy!
    if (get_inode(fs, path)) {
        //return NULL if inode already exists
        return NULL;
    }
    const char* filename = NULL;
    inode* parent_inode = parent_directory(fs, path, &filename);
    if (valid_filename(filename) != 1) {
        return NULL;
    }
    inode_number inode_num = first_unused_inode(fs);
    if (inode_num == -1) {
        return NULL;
    }
    
    data_block_number index = parent_inode->size / sizeof(data_block);
    size_t offset = parent_inode->size % sizeof(data_block);
    if (index >= NUM_DIRECT_BLOCKS && parent_inode->indirect == UNASSIGNED_NODE) {
        if (add_single_indirect_block(fs, parent_inode) == -1) return NULL;
    }
    
    inode* new_inode = fs->inode_root + inode_num;
    init_inode(parent_inode, new_inode);
    data_block* new_data_block;
    if (offset == 0) {
        data_block_number idx = 0;
        if (index < NUM_DIRECT_BLOCKS) {
            if ((idx = add_data_block_to_inode(fs, parent_inode)) == -1) return NULL;
        } else {
            data_block_number* data_block_num = (data_block_number*)(fs->data_root + parent_inode->indirect);
            if ((idx = add_data_block_to_indirect_block(fs, data_block_num)) == -1) return NULL;
        }
        new_data_block = (data_block*)(fs->data_root + idx);
    } else {
        if (index < NUM_DIRECT_BLOCKS) {
            new_data_block = (data_block*)(fs->data_root + parent_inode->direct[index]);
        } else {
            data_block_number* data_block_num = (data_block_number*)(fs->data_root + parent_inode->indirect);
            new_data_block = (data_block*)(fs->data_root + data_block_num[index - NUM_DIRECT_BLOCKS]);
        }
    }
    
    minixfs_dirent dirent;
    char filename_cpy[strlen(filename) + 1];
    memcpy(filename_cpy, filename, strlen(filename) + 1);
    dirent.name = filename_cpy;
    dirent.inode_num = inode_num;
    
    char* data_block_start_pos = new_data_block->data + offset;
    make_string_from_dirent(data_block_start_pos, dirent);
    parent_inode->size += FILE_NAME_ENTRY;
    clock_gettime(CLOCK_REALTIME, &parent_inode->atim);
    clock_gettime(CLOCK_REALTIME, &parent_inode->mtim);
    return new_inode;
}

ssize_t minixfs_virtual_read(file_system *fs, const char *path, void *buf,
                             size_t count, off_t *off) {
    if (!strcmp(path, "info")) {
        // TODO implement the "info" virtual file here
        ssize_t num_used_blocks = 0;
        char* data_map = GET_DATA_MAP(fs->meta);
        for (uint64_t i = 0; i < fs->meta->dblock_count; i++) {
            if (data_map[i] == 1) {
                num_used_blocks++;
            }
        }
        char* block = block_info_string(num_used_blocks);
        //get the end of the file to read
        size_t end = *off + count;
        size_t filesize = strlen(block);
        if (filesize < end) {
            end = filesize;
        }
        if ((size_t)*off >= end) {
            //If *off is greater than the end of the file, do nothing and return 0 to indicate end of file.
            return 0;
        }
        size_t bytes_need_read = end - *off;
        memcpy(buf, block + *off, bytes_need_read);
        *off += bytes_need_read;
        return bytes_need_read;
    }

    errno = ENOENT;
    return -1;
}

ssize_t minixfs_write(file_system *fs, const char *path, const void *buf,
                      size_t count, off_t *off) {
    // X marks the spot
    //get the current inode
    inode* cur_inode = get_inode(fs, path);
    //if not exist, try to create one
    if (!cur_inode) {
        cur_inode = minixfs_create_inode_for_path(fs, path);
        if (!cur_inode) {
            //function fails because no more data blocks can be allocated, set errno to ENOSPC
            errno = ENOSPC;
            return -1;
        }
    }

    //get the block number needed
    int block_count = (*off + count) / sizeof(data_block);
    if ((*off + count) % sizeof(data_block) != 0) block_count++;
    //to ensure that there are enough blocks
    if (minixfs_min_blockcount(fs, path, block_count) == -1) {
        //function fails because no more data blocks can be allocated, set errno to ENOSPC
        errno = ENOSPC;
        return -1;
    }

    //now we are going to write
    size_t data_block_index = (*off) / sizeof(data_block);
    size_t block_offset = (*off) % sizeof(data_block);
    size_t total_bytes_written = 0;
    //for each data block, write bytes_to_write amount of bytes to (cur_block->data + block_offset) from (buf + total_bytes_written)
    while (total_bytes_written < count) {
        data_block* cur_block;  //current data block
        if (data_block_index < NUM_DIRECT_BLOCKS) {
            cur_block = (data_block*)(fs->data_root + cur_inode->direct[data_block_index]);
        } else {
            data_block_number* data_block_num = (data_block_number*)(fs->data_root + cur_inode->indirect);
            cur_block = (data_block*)(fs->data_root + data_block_num[data_block_index - NUM_DIRECT_BLOCKS]);
        }
        size_t bytes_to_write = sizeof(data_block) - block_offset;  //number of bytes to write for each block
        if (count - total_bytes_written < bytes_to_write) {
            //if remaining bytes is not enough
            bytes_to_write = count - total_bytes_written;
        }
        memcpy(cur_block->data + block_offset, buf + total_bytes_written, bytes_to_write);
        total_bytes_written += bytes_to_write;
        *off += bytes_to_write;
        block_offset = 0;
        data_block_index++;
    }
    cur_inode->size += total_bytes_written;
    //update the node's mtim and atim
    clock_gettime(CLOCK_REALTIME, &cur_inode->atim);
    clock_gettime(CLOCK_REALTIME, &cur_inode->mtim);
    return total_bytes_written;
}

ssize_t minixfs_read(file_system *fs, const char *path, void *buf, size_t count,
                     off_t *off) {
    const char *virtual_path = is_virtual_path(path);
    if (virtual_path)
        return minixfs_virtual_read(fs, virtual_path, buf, count, off);
    // 'ere be treasure!
    //get the current inode
    inode* cur_inode = get_inode(fs, path);
    if (!cur_inode) {
        //function fails because the path does not exist, set errno to ENOENT
        errno = ENOENT;
        return -1;
    }

    //get the end of the file to read
    size_t end = *off + count;
    if (cur_inode->size < end) {
        end = cur_inode->size;
    }
    if ((size_t)*off >= end) {
        //If *off is greater than the end of the file, do nothing and return 0 to indicate end of file.
        return 0;
    }
    size_t bytes_need_read = end - *off;
    //get the block number needed
    int block_count = end / sizeof(data_block);
    if (end % sizeof(data_block) != 0) block_count++;

    //now we are going to read
    size_t data_block_index = (*off) / sizeof(data_block);
    size_t block_offset = (*off) % sizeof(data_block);
    size_t total_bytes_read = 0;
    //for each data block, read bytes_to_read amount of bytes to (buf + total_bytes_read) from (cur_block->data + block_offset)
    while (total_bytes_read < bytes_need_read) {
        data_block* cur_block;  //current data block
        if (data_block_index < NUM_DIRECT_BLOCKS) {
            cur_block = (data_block*)(fs->data_root + cur_inode->direct[data_block_index]);
        } else {
            data_block_number* data_block_num = (data_block_number*)(fs->data_root + cur_inode->indirect);
            cur_block = (data_block*)(fs->data_root + data_block_num[data_block_index - NUM_DIRECT_BLOCKS]);
        }
        size_t bytes_to_read = sizeof(data_block) - block_offset;  //number of bytes to write for each block
        if (bytes_need_read - total_bytes_read < bytes_to_read) {
            //if remaining bytes is not enough
            bytes_to_read = bytes_need_read - total_bytes_read;
        }
        memcpy(buf + total_bytes_read, cur_block->data + block_offset, bytes_to_read);
        total_bytes_read += bytes_to_read;
        *off += bytes_to_read;
        block_offset = 0;
        data_block_index++;
    }
    //update the node's atim
    clock_gettime(CLOCK_REALTIME, &cur_inode->atim);
    return total_bytes_read;
}
