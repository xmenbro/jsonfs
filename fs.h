#define FUSE_USE_VERSION 31
#include<fuse3/fuse.h>
#include<jansson.h>

// The root json
extern json_t* root_json;

// Fuse file operations
extern const struct fuse_operations fops;

// Get file's attributes: mode, type, size
int fs_getattr(const char* path, struct stat* st, struct fuse_file_info* fi);
// Read directory
int fs_readdir(const char* path, void* buf, fuse_fill_dir_t filler, 
                        off_t offset, struct fuse_file_info* fi, enum fuse_readdir_flags flags);
// Read file
int fs_read(const char* path, char* buf, size_t size, off_t offset, struct fuse_file_info* fi);
