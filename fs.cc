#include "fs.h"

int INE5412_FS::fs_format()
{
    // Verifica se o disco já está montado
    if (is_mounted) {
        return 0; // Retorna falha se o disco já estiver montado
    }

    // Lê o número de blocos no disco
    int nblocks = disk->size();

    // Calcula o número de blocos reservados para inodes (10% do total)
    int ninodeblocks = nblocks / 10;

    // Calcula o número de inodes
    int ninodes = ninodeblocks * INODES_PER_BLOCK;

    // Cria e preenche o superbloco
    union fs_block superblock;
    superblock.super.magic = FS_MAGIC;
    superblock.super.nblocks = nblocks;
    superblock.super.ninodeblocks = ninodeblocks;
    superblock.super.ninodes = ninodes;

    // Escreve o superbloco no disco
    disk->write(0, superblock.data);

    // Libera os inodes inicializando os blocos de inode
    union fs_block inode_block;
    for (int i = 0; i < ninodeblocks; i++) {
        memset(inode_block.data, 0, sizeof(inode_block.data)); // Zera o bloco de inode
        disk->write(i + 1, inode_block.data); // Escreve no disco
    }
    return 1; // Retorna sucesso
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
    if (is_mounted) {
        cout << "Erro: Sistema de arquivos já montado.\n";
        return 0; // Retorna falha se já estiver montado
    }

    // Lê o superbloco
    union fs_block superblock;
    disk->read(0, superblock.data);

    // Verifica a validade do magic number
    if (superblock.super.magic != FS_MAGIC) {
        cout << "Erro: Magic number inválido.\n";
        return 0; // Retorna falha se o sistema de arquivos não for válido
    }

    // Inicializa o bitmap de blocos livres
    int nblocks = superblock.super.nblocks;
    free_blocks = fs_bitmap(nblocks);  // Aqui, agora o free_blocks é corretamente inicializado

    for (int i = 0; i < nblocks; i++) {
        free_blocks.lock(i); // Marca todos os blocos como ocupados inicialmente
    }

    // Libera blocos realmente livres
    for (int i = 0; i < superblock.super.ninodeblocks; i++) {
        union fs_block inode_block;
        disk->read(i + 1, inode_block.data);

        for (int j = 0; j < INODES_PER_BLOCK; j++) {
            if (inode_block.inode[j].isvalid) {
                for (int k = 0; k < POINTERS_PER_INODE; k++) {
                    if (inode_block.inode[j].direct[k]) {
                        free_blocks.lock(inode_block.inode[j].direct[k]);
                    }
                }

                if (inode_block.inode[j].indirect) {
                    free_blocks.lock(inode_block.inode[j].indirect);
                    union fs_block indirect_block;
                    disk->read(inode_block.inode[j].indirect, indirect_block.data);

                    for (int k = 0; k < POINTERS_PER_BLOCK; k++) {
                        if (indirect_block.pointers[k]) {
                            free_blocks.lock(indirect_block.pointers[k]);
                        }
                    }
                }
            }
        }
    }

    // Marca o sistema como montado
    is_mounted = true;
    return 1; // Retorna sucesso
}



int INE5412_FS::fs_create()
{
    // if (!is_mounted) {
    //     cout << "Erro: Sistema de arquivos não está montado.\n";
    //     return 0; // Falha se o sistema de arquivos não estiver montado
    // }

    // Lê o superbloco`
    union fs_block superblock;
    disk->read(0, superblock.data);

    int ninodeblocks = superblock.super.ninodeblocks; // Número de blocos de inodes
    int ninodes = superblock.super.ninodes;           // Número total de inodes

    // Itera pelos blocos de inodes
    union fs_block inode_block;
    for (int i = 0; i < ninodeblocks; i++) {
        disk->read(i + 1, inode_block.data); // Lê o bloco de inodes

        // Itera pelos inodes no bloco atual
        for (int j = 0; j < INODES_PER_BLOCK; j++) {
            if (inode_block.inode[j].isvalid == 0) { // Encontra um inode inválido (livre)
                // Inicializa o inode
                inode_block.inode[j].isvalid = 1;
                inode_block.inode[j].size = 0;

                for (int k = 0; k < POINTERS_PER_INODE; k++) {
                    inode_block.inode[j].direct[k] = 0;
                }
                inode_block.inode[j].indirect = 0;

                // Escreve o bloco de inodes atualizado no disco
                disk->write(i + 1, inode_block.data);

                // Retorna o número do inode (1-based index)
                return i * INODES_PER_BLOCK + j + 1;
            }
        }
    }

    // Se não houver inodes livres, retorna falha
    cout << "Erro: Não há inodes livres disponíveis.\n";
    return 0;
}

int INE5412_FS::fs_delete(int inumber)
{
    // // Verifica se o sistema de arquivos está montado
    // if (!is_mounted) {
    //     cout << "Erro: Sistema de arquivos não está montado.\n";
    //     return 0; // Falha se o sistema de arquivos não estiver montado
    // }

    // Leitura do superbloco (para verificar o número de inodes, etc.)
    union fs_block superblock;
    disk->read(0, superblock.data); // Apenas leitura sem checar retorno

    int ninodes = superblock.super.ninodes;

    // Valida o número do inode
    if (inumber <= 0 || inumber > ninodes) {
        cout << "Erro: Inode inválido.\n";
        return 0; // Retorna falha se o inode não for válido
    }

    // Cálculo do bloco de inode
    int inode_block_index = (inumber - 1) / INODES_PER_BLOCK + 1;
    int inode_index = (inumber - 1) % INODES_PER_BLOCK;

    // Leitura do bloco de inodes
    union fs_block inode_block;
    disk->read(inode_block_index, inode_block.data); // Apenas leitura sem checar retorno

    // Verifica se o inode está válido
    if (inode_block.inode[inode_index].isvalid == 0) {
        cout << "Erro: Inode não está válido.\n";
        return 0;
    }

    // Liberação dos blocos diretos
    for (int i = 0; i < POINTERS_PER_INODE; i++) {
        if (inode_block.inode[inode_index].direct[i] != 0) {
            // Verifica se o bloco de dados direto é válido antes de liberar
            int direct_block = inode_block.inode[inode_index].direct[i];
            if (direct_block >= 0) {
                free_blocks.free(direct_block); // Libera o bloco direto
                inode_block.inode[inode_index].direct[i] = 0; // Zera o ponteiro
            } else {
                cout << "Erro: Ponteiro de bloco direto inválido: " << direct_block << "\n";
            }
        }
    }

    // Liberação do bloco indireto, se houver
    if (inode_block.inode[inode_index].indirect != 0) {
        // Leitura do bloco indireto
        union fs_block indirect_block;
        disk->read(inode_block.inode[inode_index].indirect, indirect_block.data); // Apenas leitura sem checar retorno

        // Libera os blocos apontados pelo bloco indireto
        for (int i = 0; i < POINTERS_PER_BLOCK; i++) {
            if (indirect_block.pointers[i] != 0) {
                // Verifica se o ponteiro de bloco indireto é válido
                int indirect_block_pointer = indirect_block.pointers[i];
                if (indirect_block_pointer >= 0) {
                    free_blocks.free(indirect_block_pointer);
                    indirect_block.pointers[i] = 0;
                } else {
                    cout << "Erro: Ponteiro de bloco indireto inválido: " << indirect_block_pointer << "\n";
                }
            }
        }

        // Libera o bloco indireto
        free_blocks.free(inode_block.inode[inode_index].indirect);
        inode_block.inode[inode_index].indirect = 0;
    }

    // Zera o inode
    inode_block.inode[inode_index].isvalid = 0;
    inode_block.inode[inode_index].size = 0;

    // Grava o bloco de volta
    disk->write(inode_block_index, inode_block.data); // Apenas gravação sem checar retorno

    // Retorna sucesso
    return 1;
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
