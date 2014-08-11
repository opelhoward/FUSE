#ifndef _FILESYSTEM
#define _FILESYSTEM

#include "header.hpp"
#include "log.h"
#include "volume_information.hpp"
#include "file.hpp"

#include <string>

extern volume_information_class volume_information;
extern file_class file[BLOCK_AMOUNT];
extern char handler[FILESYSTEM_SIZE];

// char handler[FILESYSTEM_SIZE];
std::string getFirstPath(std::string);
std::string getLastPath(std::string);
std::string removeFirstPath(std::string);
std::string removeLastPath(std::string);
int searchFile(const char*);
int searchParentFolder(const char*);
int searchPrevFile(const char*);
void openFile(const char*);
void createFile(const char*);
#endif
