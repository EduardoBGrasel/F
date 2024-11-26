#ifndef FS_H
#define FS_H

#include <cstring>
#include "disk.h"
#include <vector>

class INE5412_FS
{
public:
    static const unsigned int FS_MAGIC = 0xf0f03410;
    static const unsigned short int INODES_PER_BLOCK = 128;
    static const unsigned short int POINTERS_PER_INODE = 5;
    static const unsigned short int POINTERS_PER_BLOCK = 1024;

    class fs_superblock {
        public:
            unsigned int magic;
            int nblocks;
            int ninodeblocks;
            int ninodes;
    }; 

    class fs_inode {
        public:
            int isvalid;
            int size;
            int direct[POINTERS_PER_INODE];
            int indirect;
    };

    class fs_bitmap {
    public:
    fs_bitmap() : bitmap(0, false) {}
    fs_bitmap(int size) : bitmap(size < 0 ? 0 : size, false) {
        if (size < 0) {
            throw std::invalid_argument("O tamanho do bitmap não pode ser negativo.");
        }
    }

    std::vector<bool> bitmap;

    bool test_free(int i) {
        if (i < 0 || i >= bitmap.size()) {
            throw std::out_of_range("Índice fora dos limites do bitmap.");
        }
        return !bitmap[i];
    }

    void lock(int i) {
        if (i < 0 || i >= bitmap.size()) {
            throw std::out_of_range("Índice fora dos limites do bitmap.");
        }
        if (!bitmap[i])
            bitmap[i] = true;
    }

    void free(int i) {
        if (i < 0 || i >= bitmap.size()) {
            throw std::out_of_range("Índice fora dos limites do bitmap.");
        }
        if (bitmap[i])
            bitmap[i] = false;
    }
};

#endif

    union fs_block {
        public:
            fs_superblock super;
            fs_inode inode[INODES_PER_BLOCK];
            int pointers[POINTERS_PER_BLOCK];
            char data[Disk::DISK_BLOCK_SIZE];
    };

public:

    INE5412_FS(Disk *d) {
        disk = d;
    }

    void fs_debug();
    int  fs_format();
    int  fs_mount();

    int  fs_create();
    int  fs_delete(int inumber);
    int  fs_getsize(int inumber);

    int  fs_read(int inumber, char *data, int length, int offset);
    int  fs_write(int inumber, const char *data, int length, int offset);


private:
    Disk *disk;
    bool is_mounted = true; // Estado de montagem do sistema de arquivos
    fs_bitmap free_blocks;   // Bitmap de blocos livres (membro persistente)
};
