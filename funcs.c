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
