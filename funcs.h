#ifndef FUNCS_H
#define FUNCS_H

#include"fs.h"

// Parse the path - path info
struct path_info* parse_path(const char* path);
// Check if it's a directory
int is_directory(json_t* value);
// Replace node
int replace_node(struct path_info* info, json_t* node, json_t* new_node);
// Create entry - creates file or dir
int create_entry(const char* path, json_t* new_node);

#endif
