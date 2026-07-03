#include"fs.h"
#include<string.h>
#include<errno.h>

// The root json
json_t* root_json = NULL;

// Get file's atrributes: mode, type, size
int fs_getattr(const char* path, struct stat* st, struct fuse_file_info* fi) {
    memset(st, 0, sizeof(struct stat));
    // Root is always the directory
    if (strcmp(path, "/") == 0) {
        st->st_mode = S_IFDIR | 0775;
        st->st_nlink = 2;
        return 0;
    }
    // Find a key in JSON skipping "/"
    json_t* value = json_object_get(root_json, path + 1);
    if (json_is_string(value)) {
        st->st_mode = S_IFREG | 0644;
        st->st_nlink = 1;
        st->st_size = strlen(json_string_value(value));
        return 0;
    }

    // The file wasn't found
    return -ENOENT;
}

// Read directory
int fs_readdir(const char* path, void* buf, fuse_fill_dir_t filler, 
                        off_t offset, struct fuse_file_info* fi, enum fuse_readdir_flags flags) {
    // Add . and ..
    filler(buf, ".", NULL, 0, 0);
    filler(buf, "..", NULL, 0, 0);
    
    // Add files from JSON
    const char* key;
    json_t* value;
    json_object_foreach(root_json, key, value) {
        filler(buf, key, NULL, 0, 0);
    }

    return 0;
}

// Read file
int fs_read(const char* path, char* buf, size_t size, off_t offset, struct fuse_file_info* fi) {
    json_t* value = json_object_get(root_json, path + 1);

    // Get a content and lenght
    const char* content = json_string_value(value);
    size_t len = strlen(content);

    if (offset >= len)
        return 0;

    if (offset + size > len)
        size = len - offset;

    memcpy(buf, content + offset, size);
    return size;
}

// Register fuse operations
const struct fuse_operations fops = {
    .getattr = fs_getattr,
    .readdir = fs_readdir,
    .read = fs_read,
};
