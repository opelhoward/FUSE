#ifndef _VOLUME_INFORMATION
#define _VOLUME_INFORMATION

#include "header.hpp"
#include "file.hpp"

extern char handler[FILESYSTEM_SIZE];

class volume_information_class {
	private :
		char* magic_string;
		char* volume_name;
		int* capacity;
		int* free_block;
		int* ptr_free_block;
	public :
		void init();
		std::string getName();
		int getCapacity();
		int getNumbFreeBlock();
		ushort frontBlock();
		void popBlock();
		void pushBlock(ushort idx);
};
#endif
