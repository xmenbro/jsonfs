#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include "fs.h"

int main(int argc, char* argv[]) {
    json_error_t error;
    char** fuse_argv = NULL;
    int fuse_argc = 0;
    char* json_file = NULL;

    // Allocate memory for fuse_argv
    fuse_argv = malloc(argc * sizeof(char*));
    if (!fuse_argv) {
        perror("Memory allocation failed\n");
        return 1;
    } 
    
    // Copy name of program
    fuse_argv[fuse_argc++] = argv[0];

    // Parse args
    for (int i = 1; i < argc; i++) {
        // if we found an -j option which means that the next arg will be a json file
        if (strcmp(argv[i], "-j") == 0 && i + 1 < argc)
            json_file = argv[++i];
        else
            fuse_argv[fuse_argc++] = argv[i];
    }
    
    if (!json_file) {
        fprintf(stderr, "Usage: %s -j <json_file> [fuse_options] mountpoint\n", argv[0]);
        free(fuse_argv);
        return 1;
    }

    // Parse JSON document
    root_json = json_load_file(json_file, 0, &error);
    if (!root_json) {
        fprintf(stderr, "Can't parse json %d: %s\n", error.line, error.text);
        free(fuse_argv);
        return 1;
    }
    
    // Save path to json file
    json_file_path = strdup(json_file);
    if (!json_file_path) {
        fprintf(stderr, "Memory allocation failed for json_file_path\n");
        json_decref(root_json);
        free(fuse_argv);
        return 1;
    }

    // Start fuse
    int status = fuse_main(fuse_argc, fuse_argv, &fops, NULL);

    // Free memory
    if (json_file_path) {
        free(json_file_path);
        json_file_path = NULL;
    }
    json_decref(root_json);
    free(fuse_argv);
    return status;
}
