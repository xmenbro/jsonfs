#include"fs.h"
#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<jansson.h>
#include<errno.h>

// The root json
json_t* root_json = NULL;

// Get file's atrributes: mode, type, size
static int fs_getattr(const char* path, struct stat* st, struct fuse_file_info* fi) {
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

int main(int argc, char* argv[]) {
    return 0;
}
