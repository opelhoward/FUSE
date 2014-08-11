#include "volume_information.hpp"

// volume information implementation
void volume_information_class::init() {
	volume_name = &handler[4];
	capacity = (int *) &handler[36];
	free_block = (int *) &handler[40];
	ptr_free_block = (int *) &handler[44];
}
std::string volume_information_class::getName() {
	return std::string(magic_string);
}
int volume_information_class::getCapacity() {
	return *capacity;
}
int volume_information_class::getNumbFreeBlock() {
	return (*free_block);
}
ushort volume_information_class::frontBlock() {
	return (ushort) (*ptr_free_block); 
}
void volume_information_class::popBlock() {
	(*free_block)--;
	ushort* temp = (ushort *) &handler[BLOCK_SIZE+frontBlock()*2];
	*ptr_free_block = *temp;
	*temp = INVALID_BLOCK;
}
void volume_information_class::pushBlock(ushort idx) {
	(*free_block)++;
	ushort* temp = (ushort *) &handler[BLOCK_SIZE+idx*2];
	*temp = frontBlock();
	*ptr_free_block = idx;
}
