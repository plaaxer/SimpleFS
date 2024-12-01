#include "fs.h"


int INE5412_FS::fs_format()
{
    if (mounted) {
        cout << "Cannot format a mounted disk!" << endl;
        return 0;
    }

    class fs_superblock superblock;
    superblock.magic = FS_MAGIC;
    superblock.nblocks = disk->size();
    superblock.ninodeblocks = superblock.nblocks / 10;
    superblock.ninodes = INODES_PER_BLOCK * superblock.ninodeblocks;

    disk->write(0, (char*)&superblock);

	// inicializa todos os inodes como inválidos
	// tratamento de size, direct e indirect é feito no fs_create
	class fs_inode inode;
	inode.isvalid = 0;
	for (int i=1; i<=superblock.ninodes; i++) {
		inode_save(i, &inode);
	}	
    return 1;
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

			// obtém os blocos de dados diretos através de funcao auxiliar
			std::vector<int> direct_blocks = fs_get_direct_data_blocks((i*INODES_PER_BLOCK+j+1));

			// itera e imprime os blocos de dados diretos
			for (long unsigned int k=0; k<direct_blocks.size(); k++) {
				cout << direct_blocks[k] << " ";

			}

			cout << "\n";

			// ponteiro indireto pela definicão do enunciado
			int indirectpointer = block.inode[j].indirect;

			if (indirectpointer != 0) {

				// imprime o ponteiro indireto (este indica um bloco de ponteiros extras)
				cout << "   indirect block: " << block.inode[j].indirect << "\n";

				// obtém os blocos de dados indiretos através do indirect block
				std::vector<int> indirect_blocks = fs_get_indirect_data_blocks(indirectpointer);

				// itera e imprime os blocos de dados indiretos (warning do compiler... long unsigned int)
				cout << "   indirect data blocks: ";
				for (long unsigned int k=0; k<indirect_blocks.size(); k++) {
					cout << indirect_blocks[k] << " ";
				}
				cout << "\n";
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
				
				// indica que o bloco de ponteiros indiretos está sendo utilizado
				used_blocks_bitmap[indirectpointer-this->system_blocks] = true;

				// itera sobre todos os blocos de dados apontados pelo bloco indireto de ponteiros
				std::vector<int> indirect_blocks = fs_get_indirect_data_blocks(indirectpointer);

				for (long unsigned int k=0; k<indirect_blocks.size(); k++) { // long unsiged int para evitar warning do compilador
					used_blocks_bitmap[indirect_blocks[k]-this->system_blocks] = true; // indica que o bloco está sendo utilizado
				}
			}
		}
	}
	cout << "used_blocks_bitmap: ";
	for (bool bit : used_blocks_bitmap) {
		cout << bit << " ";
	}
	cout << "\n";
	cout << "used_inodes_bitmap: ";
	for (bool bit : used_inodes_bitmap) {
		cout << bit << " ";
	}
	cout << "\n";
	mounted = 1;
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

			class fs_inode inode;

			// carrega inode do disco
			inode_load(i+1, &inode);

			// seta o inode como válido e com tamanho 0
			inode.isvalid = 1;
			inode.size = 0;

			// esvazia ponteiros diretos
			for (int j=0; j<POINTERS_PER_INODE; j++) {
				inode.direct[j] = 0;
			}

			// esvazia ponteiro indireto
			inode.indirect = 0;

			// salva inode de volta ao disco
			inode_save(i+1, &inode);

			return i + 1; // retorna o número do inode (começa em 1)
		}
	}

	cout << "Error: there's not enough space (free inode not found)";
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

	// abre o inode referenciado
	class fs_inode inode;
	inode_load(inumber, &inode);

	// seta o inode como inválido e com tamanho 0
	inode.isvalid = 0;
	inode.size = 0;

	// apaga todos os ponteiros diretos do inode
	for (int i=0; i<POINTERS_PER_INODE; i++) {
		inode.direct[i] = 0;
	}

	// obtém o bloco de ponteiros indiretos
	int indirect = inode.indirect;
	if (indirect != 0) {
		std::vector<int> indirect_pointers = fs_get_indirect_data_blocks(indirect);
		
		// apaga todos os ponteiros indiretos do inode
		for (long unsigned int i=0; i<indirect_pointers.size(); i++) {
			used_blocks_bitmap[indirect_pointers[i]-this->system_blocks] = false;
		}

		// apaga o bloco de ponteiros indiretos
		inode.indirect = 0;
	}

	// escreve o bloco de inode de volta ao disco
	inode_save(inumber, &inode);

	return 1;
}

int INE5412_FS::fs_getsize(int inumber)
{	

	if (used_inodes_bitmap.size() == 0 || inumber < 1 || inumber > used_inodes_bitmap.size()) {
		return -1;
	}

	int size;

	// carrega o inode do disco
	class fs_inode inode;
	inode_load(inumber, &inode);

	// obtém o tamanho do inode referenciado
	size = inode.size;

	return size;
}

int INE5412_FS::fs_read(int inumber, char *data, int length, int offset)
{
	if (used_inodes_bitmap.size() == 0 || inumber < 1 || inumber > used_inodes_bitmap.size() || length <= 0 || offset < 0) {
		cout << "Invalid parameters when reading inode";
		return -1;
	}
	class fs_inode inode;
	inode_load(inumber, &inode);

	if (inode.isvalid == 0) {
		cout << "Inode is not valid!";
		return -1;
	}
	int read_bytes = 0;
	/* 
	obs: read_bytes é o número de bytes que são adicionados a char *data, não o total de bytes efetivamente acessados no disco. Isso é
	feito para que read_bytes não supere length, por exemplo.
	*/
	int block_offset = 0; // número de blocos a andar para comecar a ler
	int bytes_offset = 0; // número de bytes a andar para comecar a ler
	char* buffer = new char[disk->DISK_BLOCK_SIZE]; // buffer para armazenar os bytes lidos do disco
	int current_length = 0; // tamanho a ser lido no bloco atual

	// obtém todos os blocos de dados do arquivo dado
	vector<int> direct_blocks = fs_get_direct_data_blocks(inumber);
	vector<int> indirect_blocks = fs_get_indirect_data_blocks(inode.indirect);

	// junta os blocos diretos e indiretos em um único vetor
	std::vector<int> blocks(direct_blocks.begin(), direct_blocks.end());
    blocks.insert(blocks.end(), indirect_blocks.begin(), indirect_blocks.end());

	// calcular offset para descobrir por qual bloco (e byte) comecar ler
	block_offset = offset/disk->DISK_BLOCK_SIZE;
	bytes_offset = offset%disk->DISK_BLOCK_SIZE;

	cout << "direct blocks: ";
	for (int i=0; i<direct_blocks.size(); i++) {
		cout << direct_blocks[i] << " ";
	}
	cout << "\nblock offset: " << block_offset << "\nbytes offset:" << bytes_offset << "\n" << "length: " << length << "\n";

	// comeca a leitura a partir do offset de bloco
	for (size_t block_num = block_offset; block_num < blocks.size(); block_num++) {

		// adiciona os bytes lidos ao buffer (sempre de 4kb em 4kb)
		disk->read(blocks[block_num], buffer);
		//cout << "block num: " << blocks[block_num] << "\n" << "buffer: " << buffer << "\n" << "length: " << length << "\n" << "block_num: " << block_num << "\n";
		// se estivermos na primeira iteracão, ainda há byte offset
		if (block_num == block_offset) {

			// cálculo para o primeiro bloco a ser lido (se length for menor do que falta para o fim do bloco, lê length, senão lê o que falta para o fim do bloco)
			current_length = (length < (disk->DISK_BLOCK_SIZE-bytes_offset)) ? length : disk->DISK_BLOCK_SIZE-bytes_offset;
			//cout << "current length: " << current_length << "\n";
			// faz a cópia dos bytes lidos para o *data
			memcpy(data, buffer+bytes_offset, current_length);

		} else {

			// cálculo para demais casos do length a ser lido (se length for menor que o tamanho do bloco, lê length, senão lê o tamanho do bloco)
			current_length = (length < disk->DISK_BLOCK_SIZE) ? length : disk->DISK_BLOCK_SIZE;
			//cout << "current length: " << current_length << "\n";
			// faz a cópia dos bytes lidos para o *data
			memcpy(data+read_bytes, buffer, current_length);

		}

		// incrementa contador de bytes lidos
		read_bytes += current_length; 
		
		// decrementa o length para saber quantos bytes ainda faltam ser lidos
		length -= current_length;

		// se length for 0, todos os bytes foram lidos
		if (length <= 0) {
			delete[] buffer;
			return read_bytes;
		}
	}
	// arquivo lido completamente
	return read_bytes;
}

int INE5412_FS::fs_write(int inumber, const char *data, int length, int offset)
{
	if (used_inodes_bitmap.size() == 0 || inumber < 1 || inumber > used_inodes_bitmap.size() || length <= 0 || offset < 0) {
		cout << "Invalid parameters when reading inode";
		return 0;
	}

	class fs_inode inode;
	inode_load(inumber, &inode);

	if (inode.isvalid == 0) {
		cout << "Inode is not valid!";
		return 0;
	}

	int written_bytes = 0;
	int current_length = 0;
	char* buffer = new char[disk->DISK_BLOCK_SIZE];


	// obtém todos os blocos de dados do arquivo dado
	vector<int> direct_blocks = fs_get_direct_data_blocks(inumber);
	vector<int> indirect_blocks = fs_get_indirect_data_blocks(inode.indirect);

	// junta os blocos diretos e indiretos em um único vetor
	std::vector<int> blocks(direct_blocks.begin(), direct_blocks.end());
    blocks.insert(blocks.end(), indirect_blocks.begin(), indirect_blocks.end());

	// calcular offset para descobrir por qual bloco (e byte) comecar a escrever
	int block_offset = offset/disk->DISK_BLOCK_SIZE;
	int bytes_offset = offset%disk->DISK_BLOCK_SIZE;

	// primeiro, percorre o arquivo através dos blocos de dados já existentes enquanto "gasta" o offset

	for (size_t block_num = block_offset; block_num < blocks.size(); block_num++) {

		// primeira iteracao, talvez haja byte offset
		if (block_num == block_offset) {
			
			// calcula length de escrita
			current_length = (length < (disk->DISK_BLOCK_SIZE-bytes_offset)) ? length : disk->DISK_BLOCK_SIZE-bytes_offset;

			// lê conteúdo atual do disco
			disk->read(block_num, buffer);

			// altera conteúdo atual do disco com conteúdo de data (a partir do offset de bytes)
			memcpy(buffer+bytes_offset, data, current_length);

			// retorna ao disco
			disk->write(block_num, buffer);
		}

		else {

			current_length = (length < disk->DISK_BLOCK_SIZE) ? length : disk->DISK_BLOCK_SIZE;

			disk->read(block_num, buffer);

			memcpy(buffer, data+written_bytes, current_length);

			disk->write(block_num, buffer);

		}
		
		written_bytes += current_length;
		length -= current_length;

		if (length == 0) {
			inode.size += written_bytes;
			inode_save(inumber, &inode);
			delete[] buffer;
			return written_bytes;
		}

	}

	// se ainda houver bytes a serem escritos, é hora de alocar novos blocos de dados

	union fs_block aux_block;
	int free_block;
	char* second_buffer = new char[disk->DISK_BLOCK_SIZE];
	int block_referenced;

	// itera preenchendo blocos livres até terminar de escrever o length ou não haver mais espaco
	while(length>0) {

		block_referenced = false;
		free_block = find_free_block();
		cout << "free block was found and is: " << free_block << "\n";
		if (free_block<0) {
			cout << "Not enough space to continue writing; data free block not found";
			break;
		}

		current_length = (length < disk->DISK_BLOCK_SIZE) ? length : disk->DISK_BLOCK_SIZE;

		memcpy(second_buffer, data+written_bytes, current_length);

		disk->write(free_block, second_buffer);

		written_bytes += current_length;
		length -= current_length;

		// agora é hora de criar a referência do inode ao bloco. se ainda há espaço nos blocos diretos:
		for (int i=0; i<POINTERS_PER_INODE; i++) {
			if (inode.direct[i] == 0) {
				inode.direct[i] = free_block;
				cout << "inode direct block of index " << i << " is now: " << free_block << "\n";
				block_referenced = true;
				break;
			}
		}

		if (block_referenced) {
			used_blocks_bitmap[free_block-this->system_blocks] = true;
			continue;
		}

		// se não houver mais espaço nos blocos diretos, obtém o bloco de ponteiros indiretos (se não houver, cria um)
		if (inode.indirect == 0) {
			inode.indirect = create_new_indirect_block();
			if (inode.indirect < 0) {
				cout << "Not enough space to continue writing; new block could not be pointed by inode (failed to create indirect block!)";
				written_bytes -= current_length;
				break;
			}
		}
		
		// carrega o bloco de ponteiros indiretos e adiciona o bloco de dados
		disk->read(inode.indirect, aux_block.data);
		for (int i=0; i<POINTERS_PER_BLOCK; i++) {
			if (aux_block.pointers[i] == 0) {
				aux_block.pointers[i] = free_block;
				disk->write(inode.indirect, aux_block.data);
				block_referenced = true;
				break;
			}
		}

		// se não houver espaço no bloco de ponteiros indiretos, inode está cheio
		if (!block_referenced) {
			cout << "Not enough space to continue writing; new block could not be pointed by inode (indirect block is full!)";
			written_bytes -= current_length;
			break;
		}
		// sinaliza que o bloco está sendo utilizado
		used_blocks_bitmap[free_block-this->system_blocks] = true;
	}

	inode.size += written_bytes;
	inode_save(inumber, &inode);
	delete[] second_buffer;
	return written_bytes;
	
}

std::vector<int> INE5412_FS::fs_get_indirect_data_blocks(int indirect) 
{

	// itera sobre o bloco de ponteiros dado, retornando um vetor com os valores de ponteiro que estão no bloco indireto (números de blocos)
    std::vector<int> valid_pointers;

	// se não há bloco de ponteiros, retorna vetor vazio
	if (indirect == 0) {
		return valid_pointers;
	}

	// carrega o bloco de ponteiros indiretos
    union fs_block block;
    disk->read(indirect, block.data);
    
    // itera e adiciona os ponteiros válidos ao vetor
    for (int i = 0; i < POINTERS_PER_BLOCK; i++) {
        if (block.pointers[i] != 0) {
            valid_pointers.push_back(block.pointers[i]);
        }
    }

    return valid_pointers;
}

std::vector<int> INE5412_FS::fs_get_direct_data_blocks(int inumber) 
{
	// itera sobre o bloco de ponteiros dado, retornando um vetor com os bloco de dados diretos (números de blocos)
	class fs_inode inode;

	// acessa o inode referenciado passando um objeto fs_inode
	inode_load(inumber, &inode);
	
	std::vector<int> valid_pointers;
	
	for (int i = 0; i < POINTERS_PER_INODE; i++) {
		if (inode.direct[i] != 0) {
			valid_pointers.push_back(inode.direct[i]);
		}
	}

	return valid_pointers;
}

void INE5412_FS::inode_load( int inumber, class fs_inode *inode ) 
{
	// carrega o inode com o inumber referenciado
	union fs_block block;

	// lembrando que o primeiro bloco é o superbloco e que inumber começa em 1, então se acessa por [inumber-1]
	disk->read((inumber-1) / INODES_PER_BLOCK + 1, block.data );
	*inode = block.inode[(inumber-1) % INODES_PER_BLOCK];
	return;
}

void INE5412_FS::inode_save( int inumber, class fs_inode *inode ) 
{
	// atualiza o inode no disco
	union fs_block block;

	// lembrando do mesmo detalhe do inode_load
	disk->read( (inumber-1) / INODES_PER_BLOCK + 1, block.data );
	block.inode[(inumber-1) % INODES_PER_BLOCK] = *inode;
	disk->write( (inumber-1) / INODES_PER_BLOCK + 1, block.data );
	return;
}

// retorna o primeiro bloco livre encontrado
int INE5412_FS::find_free_block() {
	for (size_t i = 0; i < used_blocks_bitmap.size(); i++) {
		if (!used_blocks_bitmap[i]) {
			return i + system_blocks;
		}
	}
	return -1;
}

int INE5412_FS::create_new_indirect_block(){

	// obtém bloco livre
	int free_block = find_free_block();
	if (free_block < 0) {
		return -1;
	}
	union fs_block block;

	// inicializa o bloco de ponteiros indiretos com 0 (possuía números aparentemente aleatórios antes)
	for (int i=0; i<POINTERS_PER_BLOCK; i++) {
		block.pointers[i] = 0;
	}

	// escreve o bloco de ponteiros indiretos no disco
	disk->write(free_block, block.data);
	return free_block;
}