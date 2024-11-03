# File System (ext2)
A CLI application that takes an ext2 image file as an input and allows you to navigate and read its contents without modifying it.

Trial [disk](https://drive.google.com/file/d/1seTKnyT4z6Gsuv9KhgS1QLhxa11tovGe/view?usp=drive_link).

## Installing
* Pre compiled binaries for [linux](bin/FileSystem) and [windows](bin/FileSystem.exe).

    or
* Bulid using the process mentioned [below](#building).

## Building
### Requirements
1. [Git](https://git-scm.com)
2. [CMake](https://cmake.org)
3. C++ compiler
    * GCC: ^13.2.0 (Default with 24.04.1 LTS)
    * MVC: ^19.42.34321.1 (Visual Studio 2022 17.12.0 preview 2.1)
    * Clang: ^18.1.3 (Default with 24.04.1 LTS)

### Build Process
1. Clone the repoitory.
    ```bash 
    git clone https://github.com/NishantAS/FileSystem.git
    ```
2. Generate build files using cmake CLI
    ```bash
    cmake -DCMAKE_INSTALL_PREFIX:PATH="<install dir>" -S FileSystem -B FileSystem/build
    ```
3. Build the project
    ```bash
    cmake --build FileSystem/build --config Release
    ```
4. Install the binary file and Starting using it!
    ```bash
    cmake --install FileSystem/build
    ```

Thats it you are ready to start using the program.

## Using
**Run :**
```bash
FileSystem <path to image>
```

### Commands
1. `exit` : Exits the program.
2. `pwd` : Prints the current working directory.

    **Usage :**
    ```bash
    user@machine:/dir1/innerdir1$ pwd
    /dir1/innerdir1
    user@machine:/dir1/innerdir1$
    ```

3. `ls` : List the content of a directory.

    **Usage :** `ls [path to directory]`
    ```bash
    user@machine:/$ ls 
    .  ..  dir1  dir2  dir3  dir4  dir5  lost+found  readthis.txt
    user@machine:/$ ls /dir4/innerdir6
    .  ..  rice.webp
    ```
  
4. `cd` : Change the working directory.

    **Usage :** `cd <path to directory>`
    ```bash
    user@machine:/$ cd dir1
    user@machine:/dir1$ cd innderdir1
    user@machine:/dir1/innerdir1$ cd ../innerdir2
    user@machine:/dir1/innerdir2$ cd /dir3
    user@machine:/dir3$
    ```

5. `cat` : Print the contents of a file.

    **Usage :** `cat <path to file>`
    ```bash
    user@machine:/$ cat readthis.txt
    This is the second file from the task
    uesr@machine:/$
    ```

6. `touch` : Create a text file

    **Usage :** `touch <path to file>`
    ```bash
    user@machine:/$ touch temp
    user@machine:/$ ls
    .  ..  dir1  dir2  dir3  dir4  dir5  lost+found  readthis.txt temp
    ```

7. `mkdir` : Create a dir
    
    **Usage :**`mkdir <path to dir>`
    ```bash
    user@machine:/$ mkdir test
    user@machine:/$ ls test
    .  ..
    ```

8. `dumpe2fs` : Gives the details about the super block and group descriptors (similar to dumpe2fs from linux).
  
    **Usage :**
    ```bash
    user@machine:/$ dumpe2fs
    Filesystem volume name: <none>
    Last mounted on: /mnt
    Filesystem UUID: 3ddb92de-ca2-4884-80da-bb59a5d178ac
    FileSystem Magic Number: 0xef53
    Filesystem revision #: 1
    Filesystem features: ext_attr resize_inode dir_index filetype sparse_super large_file
    Filesystem state: clean
    Errors behavior: ignore
    Filesystem OS type: Linux
    Inode count: 3072
    Block count: 12288
    Reserved block count: 614
    Overhead clusters: 871
    Free blocks: 7582
    Free inodes: 3022
    First block: 1
    Block size: 1024
    Fragment size: 1024
    Reserved GDT blocks: 47
    Blocks per group: 8192
    Fragments per group: 8192
    Inodes per group: 1536
    Inode blocks per group: 384
    Last mount time: Sun Sep 29 09:20:55.0000000 2024
    Last write time: Sun Sep 29 09:57:12.0000000 2024
    Mount count: 2
    Maximum mount count: 65535
    Last checked: Sun Sep 29 09:19:37.0000000 2024
    Check interval: 0
    Reserved blocks uid: 0
    Reserved blocks gid: 0
    First inode: 11
    Inode size: 256


    Group 0: (Blocks 1-8192)
            Primary superblock at 1, Group descriptors at 2-2
            Reserved GDT blocks at 3-49
            Block bitmap at 50 (+49)
            Inode bitmap at 51 (+50)
            Inode table at 52-435 (+51)
            4954 free blocks, 1494 free inodes, 29 directories
            Free blocks: 487-496, 514-1024, 1073-1552, 1601-1616, 1729-1792, 1921-1952, 2017-2048, 2669-2688, 2817-4224, 4481-4608, 5121-5248, 5505-5632, 6145-6656, 6708-8192
            Free inodes: 1579-3072
    Group 1: (Blocks 8193-12288)
            Backup superblock at 8193, Group descriptors at 8194-8194
            Reserved GDT blocks at 8195-8241
            Block bitmap at 8242 (+49)
            Inode bitmap at 8243 (+50)
            Inode table at 8244-8627 (+51)
            2628 free blocks, 1528 free inodes, 8 directories
            Free blocks: 8632, 8637-10752, 11777-12287
            Free inodes: 1540, 1543-1549, 1553-3072
    user@machine:/$ 
    ```
---