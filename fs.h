#define FUSE_USE_VERSION 31
#include<fuse.h>

// Get file's attributes: mode, type, size
static int fs_getattr(const char* path, struct stat* st, struct fuse_file_info* fi);
// Read directory
static int fs_readdir(const char* path, void* buf, fuse_fill_dir_t filler, 
                        off_t offset, struct fuse_file_info* fi, enum fuse_readdir_flags flags);
// Read file
static int fs_read(const char* path, char* buf, size_t size, off_t offset, struct fuse_file_info* fi);
