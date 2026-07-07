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

// Check if it's a directory
int is_directory(json_t* value) {
    if (!value)
        return 0;
    return json_is_object(value) || json_is_array(value);
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

    struct path_info* info = parse_path(path);
    if (!info)
        return -ENOENT;
    json_t* value = info->current;
    
    // if it's a directory
    if (is_directory(value)) {
        st->st_mode = S_IFDIR | 0775;
        st->st_nlink = 2;
    }
    // if it's a string (simple file)
    else if (json_is_string(value)) {
        st->st_mode = S_IFREG | 0644;
        st->st_nlink = 1;
        const char* content = json_string_value(value);
        if (content)
            st->st_size = strlen(content);
        else
            st->st_size = 0;
    }
    // if it's a number (also might be a file)
    else if (json_is_number(value)) {
        st->st_mode = S_IFREG | 0644;
        st->st_nlink = 1;
        char buf[64];
        if (json_is_integer(value))
            snprintf(buf, sizeof(buf), "%lld", (long long)json_integer_value(value));
        else
            snprintf(buf, sizeof(buf), "%f", json_real_value(value));
        st->st_size = strlen(buf);
    }
    // if it's a bool
    else if (json_is_boolean(value)) {
        st->st_mode = S_IFREG | 0644;
        st->st_nlink = 1;
        st->st_size = json_boolean_value(value) ? 4 : 5;
    }
    // if it's NULL
    else if (json_is_null(value)) {
        st->st_mode = S_IFREG | 0644;
        st->st_nlink = 1;
        st->st_size = 4;
    }
    else {
        free(info);
        return -ENOENT;
    }
    
    free(info);
    return 0;
}

// Read directory
int fs_readdir(const char* path, void* buf, fuse_fill_dir_t filler, 
                        off_t offset, struct fuse_file_info* fi, enum fuse_readdir_flags flags) {
    // Add . and ..
    filler(buf, ".", NULL, 0, 0);
    filler(buf, "..", NULL, 0, 0);
    
    // Determine the target
    json_t* target;
    if (strcmp(path, "/") == 0)
        target = root_json;
    else {
        struct path_info* info = parse_path(path);
        
        if (!info)
            return -ENOENT;
        target = info->current;
        free(info);
    }

    if (!target)
        return -ENOENT;
    
    if (json_is_object(target)) {
        const char* key;
        json_t* value;
        json_object_foreach(target, key, value) {
            filler(buf, key, NULL, 0, 0);
        }
    }
    else if (json_is_array(target)) {
        size_t size = json_array_size(target);
        char index_str[32];
        for (size_t i = 0; i < size; i++) {
            snprintf(index_str, sizeof(index_str), "%zu", i);
            filler(buf, index_str, NULL, 0, 0);
        }
    }
    else
        return -ENOTDIR;

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
