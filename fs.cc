#include "fs.h"

int INE5412_FS::fs_format()
{
    // Verificar se o disco já foi montado
    if (!disk) {
        cout << "ERROR: No disk mounted.\n";
        return 0;
    }

    // Configurar superbloco
    union fs_block block;
    block.super.magic = FS_MAGIC;
    block.super.nblocks = disk->size();
    block.super.ninodeblocks = block.super.nblocks / 10; // 10% dos blocos para inodes
    block.super.ninodes = block.super.ninodeblocks * INODES_PER_BLOCK;

    // Escrever superbloco no primeiro bloco
    disk->write(0, block.data);

    // Inicializar todos os blocos de inodes como vazios
    for (int i = 0; i < block.super.ninodeblocks; i++) {
        for (int j = 0; j < INODES_PER_BLOCK; j++) {
            block.inode[j].isvalid = 0; // Inode inválido
            block.inode[j].size = 0;
            block.inode[j].indirect = 0;
            for (int k = 0; k < POINTERS_PER_INODE; k++) {
                block.inode[j].direct[k] = 0;
            }
        }
        disk->write(i + 1, block.data);
    }

    cout << "Filesystem formatted successfully.\n";
    return 1; // Sucesso
}


void INE5412_FS::fs_debug()
{
	union fs_block block;

	disk->read(0, block.data);

	cout << "superblock:\n";
	cout << "    " << (block.super.magic == FS_MAGIC ? "magic number is valid\n" : "magic number is invalid!\n");
 	cout << "    " << block.super.nblocks << " blocks\n";
	cout << "    " << block.super.ninodeblocks << " inode blocks\n";
	cout << "    " << block.super.ninodes << " inodes\n";

	for (size_t i = 0; i < (size_t)block.super.ninodeblocks; i++) {
		disk->read(i + 1, block.data); // ler o inode

		// percorrendo os inodes do block
		for (size_t j = 0; j < (size_t)INODES_PER_BLOCK; j++) {
			if (block.inode[j].isvalid) {
				cout << "inode " << i * INODES_PER_BLOCK + j + 1 << endl;
				cout << "    " << "size: " << block.inode[j].size << " bytes" << endl;
				cout << "    " << "direct blocks:";

				// mostrar blocos diretos válidos
				for (size_t k = 0; k < (size_t)POINTERS_PER_INODE; k++) {
					if (block.inode[j].direct[k]) cout << " " << block.inode[j].direct[k];
				}
				cout << endl;
			}
			if (block.inode[j].indirect) {
				cout << "    " << "indirect block: " << block.inode[j].indirect << endl;
				union fs_block indirect;
				disk->read(block.inode[j].indirect, indirect.data);
				cout << "    " << "indirect data blocks:";
				for (size_t k = 0; k < POINTERS_PER_BLOCK; k++) {
					if (indirect.pointers[k]) cout << " " << indirect.pointers[k];
				}
				cout << endl;
			}
		}
	}
}

int INE5412_FS::fs_mount()
{
    return 0;
}



int INE5412_FS::fs_create()
{
    return 0;
}

int INE5412_FS::fs_delete(int inumber)
{
    return 0;
}


int INE5412_FS::fs_getsize(int inumber)
{
	return -1;
}

int INE5412_FS::fs_read(int inumber, char *data, int length, int offset)
{
	return 0;
}

int INE5412_FS::fs_write(int inumber, const char *data, int length, int offset)
{
	return 0;
}
