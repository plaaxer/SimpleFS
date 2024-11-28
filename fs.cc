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
			cout << "inode " << (i*INODES_PER_BLOCK+j+1) << ":\n"; // +1 pois lemos o read(i+1)
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
				fs_show_indirect_data_blocks(block.inode[j].indirect);
			}
		}
	}
}

int INE5412_FS::fs_mount()
{
	union fs_block superblock;
	disk->read(0, superblock.data);

	// caso o número mágico não seja encontrado, erro de mounting
	if (superblock.super.magic != FS_MAGIC) {
		cout << "\nFailed to mount the disk; magic number not found";
		return 0;
	}

	nblocks = superblock.super.nblocks;
	ninodeblocks = superblock.super.ninodeblocks;

	// dedicados ao sistema (superblock + ninodes)
	this->system_blocks = ninodeblocks + 1;

	// número de blocos do sistema direcionados para dados
	int datablocks = nblocks - this->system_blocks;

	// used_blocks_bitmap para controlar os blocos de dados livres (inicia todos como livres)
	used_blocks_bitmap = std::vector<bool>(datablocks, false);
	used_inodes_bitmap = std::vector<bool>(superblock.super.ninodes, false);

	// passa por todos os inodes válidos e preenche o used_blocks_bitmap
	for(int i=1; i<=superblock.super.ninodeblocks; i++) {
		union fs_block block;
		disk->read(i, block.data);

		// implementacão similar ao fs_debug
		for (int j=0; j < INODES_PER_BLOCK; j++) {

			if (block.inode[j].isvalid == 0) {
				continue;
			}

			cout << "inode " << (i-1)*INODES_PER_BLOCK+j+1 << " is occupied\n";

			// sinaliza o inode como utilizado
			used_inodes_bitmap[(i-1)*INODES_PER_BLOCK+j] = true;

			for (int k=0; k<POINTERS_PER_INODE; k++) {
				int blocknum = block.inode[j].direct[k];
				if (blocknum!=0) {
					used_blocks_bitmap[blocknum-this->system_blocks] = true; // indica que o bloco está sendo utilizado
				}
			}
			int indirectpointer = block.inode[j].indirect;

			if (indirectpointer != 0) {

				// itera sobre todos os blocos de dados apontados pelo bloco indireto de ponteiros
				std::vector<int> indirect_blocks = fs_get_indirect_data_blocks(indirectpointer);

				for (long unsigned int k=0; k<indirect_blocks.size(); k++) { // long unsiged int para evitar warning do compilador
					used_blocks_bitmap[indirect_blocks[k]-this->system_blocks] = true; // indica que o bloco está sendo utilizado
				}
			}
		}
	}
	//cout << "used_blocks_bitmap: ";
	//for (bool bit : used_blocks_bitmap) {
		//cout << bit << " ";
	//}
	//cout << "\n";
	cout << "used_inodes_bitmap: ";
	for (bool bit : used_inodes_bitmap) {
		cout << bit << " ";
	}
	cout << "\n";
	return 1;
}

int INE5412_FS::fs_create()
{
	if (used_inodes_bitmap.size() == 0) {
		cout << "Failed to create inode; disk not mounted";
		return 0;
	}

	// itera sobre os bitmap de inodes, procurando por um inode livre
	for (size_t i = 0; i < used_inodes_bitmap.size(); i++) {

		// inode livre encontrado
		if (used_inodes_bitmap[i] == 0) {

			// seta inode como utilizado no bitmap
			used_inodes_bitmap[i] = true;

			// lê o bloco de inode onde o inode livre foi encontrado
			union fs_block block;
			disk->read(i/INODES_PER_BLOCK+1, block.data);

			// seta o inode como válido
			block.inode[i%INODES_PER_BLOCK].isvalid = 1;

			// escreve o bloco de inode de volta no disco
			disk->write(i / INODES_PER_BLOCK + 1, block.data);

			return i + 1; // retorna o número do inode (começa em 1)
		}
	}
	return 0;
}

int INE5412_FS::fs_delete(int inumber)
{
	if (used_inodes_bitmap.size() == 0 || inumber < 1 || inumber > used_inodes_bitmap.size()) {
		cout << "Failed to delete inode";
		return 0;
	}

	// seta o inode como livre no bitmap
	used_inodes_bitmap[inumber-1] = false;

	// abre o bloco do inode referenciado
	union fs_block block;
	disk->read((inumber-1)/INODES_PER_BLOCK+1, block.data);

	// seta o inode como inválido e com tamanho 0
	block.inode[(inumber-1)%INODES_PER_BLOCK].isvalid = 0;
	block.inode[(inumber-1)%INODES_PER_BLOCK].size = 0;

	// apaga todos os ponteiros diretos do inode
	for (int i=0; i<POINTERS_PER_INODE; i++) {
		block.inode[(inumber-1)%INODES_PER_BLOCK].direct[i] = 0;
	}

	// obtém o bloco de ponteiros indiretos
	int indirect = block.inode[(inumber-1)%INODES_PER_BLOCK].indirect;
	if (indirect != 0) {
		std::vector<int> indirect_pointers = fs_get_indirect_data_blocks(indirect);
		
		// apaga todos os ponteiros indiretos do inode
		for (long unsigned int i=0; i<indirect_pointers.size(); i++) {
			used_blocks_bitmap[indirect_pointers[i]-this->system_blocks] = false;
		}

		// apaga o bloco de ponteiros indiretos
		block.inode[(inumber-1)%INODES_PER_BLOCK].indirect = 0;
	}

	// escreve o bloco de inode de volta ao disco
	disk->write((inumber-1)/INODES_PER_BLOCK+1, block.data);

	return 1;
}

int INE5412_FS::fs_getsize(int inumber)
{
	if (used_inodes_bitmap.size() == 0 || inumber < 1 || inumber > used_inodes_bitmap.size()) {
		cout << "Failed to get inode size";
		return -1;
	}
	int size;
	union fs_block block;

	// lê o bloco de inode onde o inode referenciado está
	disk->read((inumber-1)/INODES_PER_BLOCK+1, block.data);

	// obtém o tamanho do inode referenciado
	size = block.inode[(inumber-1)%INODES_PER_BLOCK].size;

	return size;
}

int INE5412_FS::fs_read(int inumber, char *data, int length, int offset)
{
	if (used_inodes_bitmap.size() == 0 || inumber < 1 || inumber > used_inodes_bitmap.size()) {
		cout << "Failed to read inode";
		return 0;
	}

	return 0;
}

int INE5412_FS::fs_write(int inumber, const char *data, int length, int offset)
{
	return 0;
}

int INE5412_FS::fs_show_indirect_data_blocks(int indirect)
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

std::vector<int> INE5412_FS::fs_get_indirect_data_blocks(int indirect) 
{

	// itera sobre o bloco de ponteiros dado, retornando um vetor com os valores de ponteiro (números de blocos)

    union fs_block block;
    disk->read(indirect, block.data);
    
    std::vector<int> valid_pointers;
    
    for (int i = 0; i < POINTERS_PER_BLOCK; i++) {
        if (block.pointers[i] != 0) {
            valid_pointers.push_back(block.pointers[i]);
        }
    }

    return valid_pointers;
}

// ultima implementada
std::vector<int> INE5412_FS::fs_get_direct_data_blocks(int inumber) 
{
	// itera sobre o bloco de inode dado, retornando um vetor com os valores de ponteiro (números de blocos)

	union fs_block block;
	disk->read((inumber-1)/INODES_PER_BLOCK+1, block.data);
	
	std::vector<int> valid_pointers;
	
	for (int i = 0; i < POINTERS_PER_INODE; i++) {
		if (block.inode[(inumber-1)%INODES_PER_BLOCK].direct[i] != 0) {
			valid_pointers.push_back(block.inode[(inumber-1)%INODES_PER_BLOCK].direct[i]);
		}
	}

	return valid_pointers;
}