#ifndef _HEADER
#define _HEADER

#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <string>
#include <stack>
#include <string.h>
#include <fstream>      // std::ifstream, std::ofstream
#include "params.h"

#define FILESYSTEM_SIZE 33686016
#define BLOCK_AMOUNT 65536
#define BLOCK_SIZE 512
#define INVALID_BLOCK (ushort) 0xFFFF

#endif
