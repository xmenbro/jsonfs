#ifndef FS_H
#define FS_H

#define FUSE_USE_VERSION 31
#include<fuse3/fuse.h>
#include<jansson.h>
#include<pthread.h>

// The root json
extern json_t* root_json;
// Mutex
extern pthread_mutex_t json_mutex;
// Path to json file
extern char* json_file_path;

// Fuse file operations
extern const struct fuse_operations fops;

// Path structure
struct path_info {
    json_t* parent;
    json_t* current;
    char* name;
};

// Get file's attributes: mode, type, size
int fs_getattr(const char* path, struct stat* st, struct fuse_file_info* fi);
// Read directory
int fs_readdir(const char* path, void* buf, fuse_fill_dir_t filler, 
                        off_t offset, struct fuse_file_info* fi, enum fuse_readdir_flags flags);
// Read file
int fs_read(const char* path, char* buf, size_t size, off_t offset, struct fuse_file_info* fi);
// Write data to file
int fs_write(const char* path, const char* buf, size_t size, off_t offset, struct fuse_file_info* fi);
// Truncate file
int fs_truncate(const char* path, off_t size, struct fuse_file_info* fi);
// Open file
int fs_open(const char* path, struct fuse_file_info* fi);
// Create file
int fs_mknod(const char* path, mode_t mode, dev_t dev);
// Set time labels
int fs_utimens(const char* path, const struct timespec ts[2], struct fuse_file_info* fi);
// Create directory
int fs_mkdir(const char* path, mode_t mode);

#endif
