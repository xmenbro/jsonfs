#include"fs.h"
#include<string.h>
#include<errno.h>

// The root json
json_t* root_json = NULL;

// Parse the path
struct path_info* parse_path(const char* path) {
    struct path_info* info = malloc(sizeof(struct path_info));
    
    if (!info)
        return NULL;

    // Initialize struct
    info->parent = NULL;
    info->current = root_json;
    info->name = NULL;
    
    // Skip '/'
    if (path[0] == '/')
        path++;

    if (strlen(path) == 0) {
        info->current = root_json;
        return info;
    }

    // Split path into segments
    char* path_copy = strdup(path);
    char* saveptr;
    char* token = strtok_r(path_copy, "/", &saveptr);

    // Parse all segments
    while (token != NULL) {
        info->parent = info->current;
        
        // if it's a json object
        if (json_is_object(info->current)) {
            json_t* child = json_object_get(info->current, token);
            
            if (!child) {
                free(path_copy);
                free(info);
                return NULL;
            }

            info->current = child;
            info->name = token;
        }
        // if it's a json array
        else if (json_is_array(info->current)) {
            // Parse index
            char* endptr;
            long index = strtol(token, &endptr, 10);
            if (*endptr != '\0' || index < 0 || index >= json_array_size(info->current)) {
                free(path_copy);
                free(info);
                return NULL;
            }
            
            json_t* child = json_array_get(info->current, index);
            
            if (!child) {
                free(path_copy);
                free(info);
                return NULL;
            }
            
            info->current = child;
            info->name = token;
        }

        //  if is isn't an array and it isn't an object
        else {
            free(path_copy);
            free(info);
            return NULL;
        }
        token = strtok_r(NULL, "/", &saveptr);
    }
    
    free(path_copy);
    return info;
}

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
