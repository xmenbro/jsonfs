# jsonfs

The C implementation of JSON file system using FUSE (Filesystem in Userspace).

## Description

`jsonfs` is a FUSE-based file system that mounts a JSON file as a virtual directory structure. It allows you to explore and navigate JSON data as if it were a traditional file system hierarchy.

## Features

- Mount a JSON file as a file system
- Navigate JSON objects and arrays as directories
- Access JSON values as files
- Support for nested JSON structures

## Requirements

- Linux operating system
- FUSE3 library (libfuse)
- GCC compiler
- Make (for building)
- jansson library (for JSON parsing)

### Install Dependencies

```bash
# Ubuntu/Debian
sudo apt install libfuse3-dev libjansson-dev make build-essential

# Fedora/RHEL
sudo dnf install fuse3-devel jansson-devel make gcc

# Arch Linux
sudo pacman -S fuse3 jansson make
```

# Building
```bash
git clone https://github.com/xmenbro/jsonfs.git
cd jsonfs
make
```

# Usage
```bash
# Mounting
./fs.out -j /full/path/to/file.json /mount/point -f

# Unmounting
fusermount -u /mount/point
```

### Options:
- j: Path to the JSON file to mount
- f: Run in foreground (optional, useful for debugging)

# File System Structure
The JSON file is mapped to directories and files as follows:

- JSON objects → directories

- JSON arrays → directories (with numeric indices as filenames)

- JSON strings, numbers, booleans, null → files containing the value

# Example
Given this JSON:
```json
{
  "users": [
    {"name": "Alice", "age": 30},
    {"name": "Bob", "age": 25}
  ],
  "config": {
    "debug": true,
    "version": "1.0"
  }
}
```
The mounted file system will look like:
```
/mnt/jsonfs/
├── users/
│   ├── 0/
│   │   ├── name    (file containing "Alice")
│   │   └── age     (file containing "30")
│   └── 1/
│       ├── name    (file containing "Bob")
│       └── age     (file containing "25")
└── config/
    ├── debug       (file containing "true")
    └── version     (file containing "1.0")
```
