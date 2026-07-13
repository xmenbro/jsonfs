#include<string.h>
#include<errno.h>
#include"funcs.h"

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

// Create entry - creates file or dir
int create_entry(const char* path, json_t* new_node) {
    if (!new_node) 
        return -ENOMEM;

    pthread_mutex_lock(&json_mutex);

    // Copy path
    char* path_copy = strdup(path);
    if (!path_copy) {
        json_decref(new_node);
        pthread_mutex_unlock(&json_mutex);
        return -ENOMEM;
    }

    // Delete slashes
    size_t len = strlen(path_copy);
    while (len > 0 && path_copy[len-1] == '/') {
        path_copy[len-1] = '\0';
        len--;
    }

    if (len == 0) { // The path is "/"
        free(path_copy);
        json_decref(new_node);
        pthread_mutex_unlock(&json_mutex);
        return -EPERM; // Can't create root
    }

    char* last_slash = strrchr(path_copy, '/');
    if (!last_slash) {
        free(path_copy);
        json_decref(new_node);
        pthread_mutex_unlock(&json_mutex);
        return -EINVAL;
    }

    char* name = last_slash + 1;
    if (*name == '\0') {
        free(path_copy);
        json_decref(new_node);
        pthread_mutex_unlock(&json_mutex);
        return -EINVAL;
    }

    // Define the parent's path
    struct path_info* parent_info = NULL;
    json_t* parent = NULL;

    if (last_slash == path_copy) {
        // Parent - root
        parent_info = parse_path("/");
        if (!parent_info) {
            free(path_copy);
            json_decref(new_node);
            pthread_mutex_unlock(&json_mutex);
            return -ENOENT;
        }
        parent = parent_info->current;
    } else {
        *last_slash = '\0';
        parent_info = parse_path(path_copy);
        if (!parent_info) {
            free(path_copy);
            json_decref(new_node);
            pthread_mutex_unlock(&json_mutex);
            return -ENOENT;
        }
        parent = parent_info->current;
    }

    // Parent must be an object
    if (!json_is_object(parent)) {
        if (parent_info) {
            if (parent_info->name) free(parent_info->name);
            free(parent_info);
        }
        free(path_copy);
        json_decref(new_node);
        pthread_mutex_unlock(&json_mutex);
        return -ENOTDIR;
    }

    // It hasn't existed yet
    if (json_object_get(parent, name)) {
        if (parent_info) {
            if (parent_info->name) free(parent_info->name);
            free(parent_info);
        }
        free(path_copy);
        json_decref(new_node);
        pthread_mutex_unlock(&json_mutex);
        return -EEXIST;
    }

    // Add a new node into a parent object
    json_object_set_new(parent, name, new_node);

    // Free resources
    if (parent_info) {
        if (parent_info->name) free(parent_info->name);
        free(parent_info);
    }
    free(path_copy);
    pthread_mutex_unlock(&json_mutex);
    return 0;
}
