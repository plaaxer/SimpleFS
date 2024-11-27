#include "fs.h"

int INE5412_FS::fs_format()
{
	return 0;
}

void INE5412_FS::fs_debug()
{
	union fs_block block;

	disk->read(0, block.data);

	// SUPERBLOCK

	cout << "superblock:\n";
	cout << "    " << (block.super.magic == FS_MAGIC ? "magic number is valid\n" : "magic number is invalid!\n");
 	cout << "    " << block.super.nblocks << " blocks\n";
	cout << "    " << block.super.ninodeblocks << " inode blocks\n";
	cout << "    " << block.super.ninodes << " inodes\n";

	// INODES

	// itera, lendo do disco, por NINODEBLOCKS blocos de inodes, a partir do segundo bloco (o primeiro é o superbloco)
	for(int i=0; i<block.super.ninodeblocks; i++) {
		disk->read(i+1, block.data);

		// itera por INODES_PER_BLOCK inodes (número de inodes por bloco)
		for(int j=0; j<INODES_PER_BLOCK; j++) {
			
			// skipa inodes inválidos
			if (block.inode[j].isvalid == 0) {
				continue;
			}

			// acessa o inode j do bloco i e imprime suas informações (se inode válido)
			cout << "inode " << (i*INODES_PER_BLOCK+j+1) << ":\n"; // +1 pois no debug mostrado os inodes começam em 1
			cout << "   size: " << block.inode[j].size << "\n";
			cout << "   direct blocks: ";

			// itera por POINTERS_PER_INODE (ponteiros diretos) do inode e os imprime (estes indicam blocos de dados)
			for(int k=0; k<POINTERS_PER_INODE; k++) {
				int blocknum = block.inode[j].direct[k];

				// naturalmente, deve ser diferente de 0 já que 0 indica nullptr praticamente
				if (blocknum != 0) {
					cout << blocknum << " ";
				}
			}

			cout << "\n";

			// ponteiro indireto pela definicão do enunciado
			int indirectpointer = block.inode[j].indirect;

			if (indirectpointer != 0) {

				// imprime o ponteiro indireto (este indica um bloco de ponteiros extras)
				cout << "   indirect block: " << block.inode[j].indirect << "\n";

				// obtém os blocos de dados indiretos através do indirect block
				fs_get_indirect_data_blocks(block.inode[j].indirect);
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
int INE5412_FS::fs_get_indirect_data_blocks(int indirect)
{
	// itera sobre o bloco de ponteiros dado, imprimindo cada um dos valores de ponteiro (números de blocos)

	union fs_block block;
	disk->read(indirect, block.data);
	cout << "   indirect data blocks: ";
	for (int i = 0; i < POINTERS_PER_BLOCK; i++) {
		if (block.pointers[i] != 0) {
			cout << block.pointers[i] << " ";
		}
	}
	cout << "\n";
	return 0;
}