#ifndef _FILE
#define _FILE

#include "header.hpp"

#define DAY_MASK (ushort)0b0000000000011111
#define MONTH_MASK (ushort)0b0000000111100000
#define YEAR_MASK (ushort)0b1111111000000000

#define SS_MASK (ushort)0b0000000000011111
#define MM_MASK (ushort)0b0000011111100000
#define HH_MASK (ushort)0b1111100000000000

extern char handler[FILESYSTEM_SIZE];

class file_class {
	public:
		char* name;
		char* attr;
		ushort* date;
		ushort* time;
		ushort* ptr;
		int* size;
		ushort* next_ptr;
	public:
		static void init();
		std::string getName();
		void setName(std::string);
		ushort getAttr();
		void setAttr(ushort);
		time_t getDateTime();
		void setDateTime(time_t t);
		int getPointer();
		void setPointer(ushort);
		int getSize();
		void setSize(int);
		ushort getNextPointer();
		void setNextPointer(ushort);
		bool isEmpty();
		char* currentPosHandler();
};

extern file_class file[BLOCK_AMOUNT];
#endif
