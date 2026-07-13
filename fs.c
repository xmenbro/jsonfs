#include"fs.h"
#include<string.h>
#include<errno.h>

// The root json
json_t* root_json = NULL;
// Init mutex
pthread_mutex_t json_mutex = PTHREAD_MUTEX_INITIALIZER;
// Path to json file
char* json_file_path = NULL;

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
            if (info->name) {
                free(info->name);
            }
            info->name = strdup(token);
        }
        // if it's a json array
        else if (json_is_array(info->current)) {
            // Parse index
            char* endptr;
            long index = strtol(token, &endptr, 10);
            if (*endptr != '\0' || index < 0 || index >= json_array_size(info->current)) {
                free(path_copy);
                if (info->name) free(info->name);
                free(info);
                return NULL;
            }
            
            json_t* child = json_array_get(info->current, index);
            
            if (!child) {
                free(path_copy);
                if (info->name) free(info->name);
                free(info);
                return NULL;
            }
            
            info->current = child;
            if (info->name) {
                free(info->name);
            }
            info->name = strdup(token);
        }
        // if it isn't an array and it isn't an object
        else {
            free(path_copy);
            if (info->name) free(info->name);
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

// Replace node
int replace_node(struct path_info* info, json_t* node, json_t* new_node) {
    // Get parent
    json_t* parent = info->parent;

    if (!new_node)
        return -ENOMEM;
    // If it's a json object
    if (parent && json_is_object(parent)) {
        printf("DEBUG: Replacing in object, parent=%p, name=%s\n", parent, info->name);
        json_object_set(parent, info->name, new_node);
        json_decref(node);
        return 0;
    }
    // If it's an array
    else if (parent && json_is_array(parent)) {
        size_t idx;
        json_t* val;
        json_array_foreach(parent, idx, val) {
            if (val == node) {
                json_array_set(parent, idx, new_node);
                json_decref(node);
                return 0;
            }
        }
        json_decref(new_node);
        return -EINVAL;   
    }
    // If it's the root
    else {
        json_decref(root_json);
        root_json = new_node;
        return 0;
    }
}

// Get file's atrributes: mode, type, size
int fs_getattr(const char* path, struct stat* st, struct fuse_file_info* fi) {
    memset(st, 0, sizeof(struct stat));

    pthread_mutex_lock(&json_mutex);

    // Root is always the directory
    if (strcmp(path, "/") == 0) {
        st->st_mode = S_IFDIR | 0775;
        st->st_nlink = 2;
        pthread_mutex_unlock(&json_mutex);
        return 0;
    }

    struct path_info* info = parse_path(path);
    if (!info) {
        pthread_mutex_unlock(&json_mutex);
        return -ENOENT;
    }
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
        pthread_mutex_unlock(&json_mutex);
        return -ENOENT;
    }
    if (info->name)
        free(info->name);
    free(info);
    pthread_mutex_unlock(&json_mutex);
    return 0;
}

// Read directory
int fs_readdir(const char* path, void* buf, fuse_fill_dir_t filler, 
                        off_t offset, struct fuse_file_info* fi, enum fuse_readdir_flags flags) {
    // Add . and ..
    filler(buf, ".", NULL, 0, 0);
    filler(buf, "..", NULL, 0, 0);
    
    pthread_mutex_lock(&json_mutex);
    
    // Determine the target
    json_t* target;
    if (strcmp(path, "/") == 0)
        target = root_json;
    else {
        struct path_info* info = parse_path(path);
        
        if (!info) {
            pthread_mutex_unlock(&json_mutex);
            return -ENOENT;
        }
        target = info->current;
        if (info->name)
            free(info->name);
        free(info);
    }

    if (!target) {
        pthread_mutex_unlock(&json_mutex);
        return -ENOENT;
    }
    
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
    else {
        pthread_mutex_unlock(&json_mutex);
        return -ENOTDIR;
    }

    pthread_mutex_unlock(&json_mutex);
    return 0;
}

// Read file
int fs_read(const char* path, char* buf, size_t size, off_t offset, struct fuse_file_info* fi) {
    pthread_mutex_lock(&json_mutex);

    // Get the path info
    struct path_info* info = parse_path(path);
    if (!info) {
        pthread_mutex_unlock(&json_mutex);
        return -ENOENT;
    }

    json_t* value = info->current;
    if (info->name)
        free(info->name);
    free(info);
    
    // if it isn't a file
    if (!value || is_directory(value)) {
        pthread_mutex_unlock(&json_mutex);
        return -EISDIR;
    }

    const char* content = NULL;
    char temp_buf[128];
    
    // Define the type of content
    if (json_is_string(value))
        content = json_string_value(value);
    else if (json_is_integer(value)) {
        snprintf(temp_buf, sizeof(temp_buf), "%lld", (long long)json_integer_value(value));
        content = temp_buf;
    }
    else if (json_is_real(value)) {
        snprintf(temp_buf, sizeof(temp_buf), "%f", json_real_value(value));
        content = temp_buf;
    }
    else if (json_is_boolean(value))
        content = json_boolean_value(value) ? "true" : "false";
    else if (json_is_null(value))
        content = "null";
    else {
        pthread_mutex_unlock(&json_mutex);
        return -EIO;
    }

    if (!content) {
        pthread_mutex_unlock(&json_mutex);
        return -EIO;
    }
    
    size_t len = strlen(content);

    if (offset >= len) {
        pthread_mutex_unlock(&json_mutex);
        return 0;
    }

    if (offset + size > len)
        size = len - offset;

    memcpy(buf, content + offset, size);
    pthread_mutex_unlock(&json_mutex);
    return size;
}

// Write data to file
int fs_write(const char* path, const char* buf, size_t size, off_t offset, struct fuse_file_info* fi) {
    if (offset < 0)
        return -EINVAL;
    
    pthread_mutex_lock(&json_mutex);
    
    // Get the path   
    struct path_info* info = parse_path(path);
    if (!info) {
        pthread_mutex_unlock(&json_mutex);
        return -ENOENT;
    }
    
    // Get the current node
    json_t* node = info->current;
    if (is_directory(node)) {
        if (info->name) free(info->name);
        free(info);
        pthread_mutex_unlock(&json_mutex);
        return -EISDIR;
    }
    
    // Copy str
    char* str = strndup(buf, size);
    if (!str) {
        if (info->name) free(info->name);
        free(info);
        pthread_mutex_unlock(&json_mutex);
        return -ENOMEM;
    }
    
    // Initially result = size
    int result = size;
    
    // If it's a string
    if (json_is_string(node)) {
        // Get the old string and her length
        const char* old_str = json_string_value(node);
        size_t old_len = strlen(old_str);
        
        // Define a new length
        size_t new_len = (offset + size > old_len) ? offset + size : old_len;

        // Create a new string of new_len
        char* new_str = malloc(new_len + 1);
        if (!new_str) {
            free(str);
            if (info->name) free(info->name);
            free(info);
            pthread_mutex_unlock(&json_mutex);
            return -ENOMEM;
        }
        
        // Copy old string
        memcpy(new_str, old_str, old_len);
        
        // Rest of space will be filled by zeros
        if (offset > old_len)
            memset(new_str + old_len, 0, offset - old_len);

        // Write new values
        memcpy(new_str + offset, str, size);
        new_str[new_len] = '\0';
        
        json_string_set(node, new_str);
        free(new_str);
    }
    // If it's an integer
    else if (json_is_integer(node)) {
        char* endptr;
        long long val = strtoll(str, &endptr, 10);
        if (*endptr != '\0' && *endptr != '\n')
            result = -EINVAL;
        else
            json_integer_set(node, val);
    }
    // If it's a real number
    else if (json_is_real(node)) {
        char* endptr;
        double val = strtod(str, &endptr);
        if (*endptr != '\0' && *endptr != '\n')
            result = -EINVAL;
        else
            json_real_set(node, val);
    }
    // If it's a boolean
    else if (json_is_boolean(node)) {
        if (strcmp(str, "true") == 0 || strcmp(str, "1") == 0) {
            int ret = replace_node(info, node, json_true());
            result = (ret == 0) ? size : ret;
        }
        else if (strcmp(str, "false") == 0 || strcmp(str, "0") == 0) {
            int ret = replace_node(info, node, json_false());
            result = (ret == 0) ? size : ret;
        }
        else
            result = -EINVAL;
    }
    // If it'a the null
    else if (json_is_null(node)) {
        int ret = replace_node(info, node, json_string(str));
        result = (ret == 0) ? size : ret;
    }
    else {
        result = -EIO;
    }
    
    free(str);
    if (info->name) free(info->name);
    free(info);
    
    pthread_mutex_unlock(&json_mutex);
    return result;
}

// Truncate file
int fs_truncate(const char* path, off_t size, struct fuse_file_info* fi) {
    pthread_mutex_lock(&json_mutex);
    
    // Get the path
    struct path_info* info = parse_path(path);
    if (!info) {
        pthread_mutex_unlock(&json_mutex);
        return -ENOENT;
    }
    
    // Get the current node
    json_t* node = info->current;
    if (is_directory(node)) {
        if (info->name) free(info->name);
        free(info);
        pthread_mutex_unlock(&json_mutex);
        return -EISDIR;
    }

    int result = 0;

    // Only strings can be truncated
    if (json_is_string(node)) {
        const char* old_str = json_string_value(node);
        size_t old_len = strlen(old_str);
        size_t new_len = size;
        
        // Create new string of new_len
        char* new_str = malloc(new_len + 1);
        if (!new_str) {
            result = -ENOMEM;
        }
        else {
            if (new_len > old_len) {
                // Expand by zeros
                memcpy(new_str, old_str, old_len);
                memset(new_str + old_len, 0, new_len - old_len);
            }
            else {
                // Truncate
                memcpy(new_str, old_str, new_len);
            }

            // Create new json string and replace
            new_str[new_len] = '\0';
            json_t* new_json = json_string(new_str);
            free(new_str);
            if (!new_json)
                result = -ENOMEM;
            else
                result = replace_node(info, node, new_json);
        }
    }
    else if (size == 0) {
        // if it's not a string
        json_t* new_json = json_string("");
        if (!new_json)
            result = -ENOMEM;
        else
            result = replace_node(info, node, new_json);
    }
    // For non-string types truncate is not supported
    else {
        result = -EINVAL;
    }

    if (info->name) free(info->name);
    free(info);
    pthread_mutex_unlock(&json_mutex);
    return result;
}

// Open file
int fs_open(const char* path, struct fuse_file_info* fi) {
    if (fi->flags & O_TRUNC)
        return fs_truncate(path, 0, fi);
    return 0;
}

// Register fuse operations
const struct fuse_operations fops = {
    .getattr = fs_getattr,
    .readdir = fs_readdir,
    .read = fs_read,
    .write = fs_write,
    .truncate = fs_truncate,
    .open = fs_open,
};
