#include "header.hpp"
#define FUSE_USE_VERSION 26

#include <fuse.h>
#include "filesystem.hpp"
#include "file.hpp"
#include <algorithm>
#include <string>
#include <vector>
#include <iostream>

#include <cstdio>

using std::string;
using std::vector;

// global variables
volume_information_class volume_information;
file_class file[BLOCK_AMOUNT];
char handler[FILESYSTEM_SIZE];

time_t mount_time;

std::fstream file_handler;			// file
void implement_usage() {
	std::cerr << "Usage: mountPoint CCFS_file [--new]" << std::endl;
}

/** Get file attributes.
 *
 * Similar to stat().  The 'st_dev' and 'st_blksize' fields are
 * ignored.  The 'st_ino' field is ignored except if the 'use_ino'
 * mount option is given.
 */
int implement_getattr(const char *path, struct stat *stbuf) {
    log_msg("\nimplement_getattr(path=\"%s\")\n", path);
    
	int idx = searchFile(path);
	
	if (idx < 0) // the file/folder doesn't exist
		return -ENOENT;
	
	stbuf->st_nlink = 1;
	stbuf->st_mode = file[idx].getAttr();
	stbuf->st_mtime = file[idx].getDateTime();
	stbuf->st_atime = file[idx].getDateTime();
	stbuf->st_size = file[idx].getSize();
	log_msg("Size : %d\n", stbuf->st_size);
	

	return 0;
}

//int implement_readlink(const char *path, char *buf, size_t size) {return 0;}

/** Create a file node
 *
 * There is no create() operation, mknod() will be called for
 * creation of all non-directory, non-symlink nodes.
 */
int implement_mknod(const char *path, mode_t mode, dev_t dev) {
    log_msg("\nimplement_mknod(path=\"%s\")\n", path);
	
	if (getLastPath(std::string(path+1)).length() > 20) // the length of the name is longer than 20 char, it may be invoked by terminal
		return -ENAMETOOLONG;
	if (volume_information.getNumbFreeBlock() == 0) // check whether there is a block left
		return -EDQUOT;
	
	int idx = searchFile(path);
	
	if (idx >= 0) // the file already exists
		return -EEXIST;
		
	// alocate new block
	idx = volume_information.frontBlock();
	volume_information.popBlock();
	
	// edit the previous entry pointer
	int prev_idx = searchFile(std::string("/"+removeLastPath(std::string(path+1))).c_str());
	if (file[prev_idx].getPointer() != INVALID_BLOCK) { // if inside pointer is already filled, add to allocation table
		prev_idx = file[prev_idx].getPointer();
		while (file[prev_idx].getNextPointer() != INVALID_BLOCK)
			prev_idx = file[prev_idx].getNextPointer();
		file[prev_idx].setNextPointer(idx);
	}
	else
		file[prev_idx].setPointer(idx);
	
	// set up the file entry
	file[idx].setName(getLastPath(std::string(path+1)));
	file[idx].setSize(0);
	file[idx].setPointer(0xFFFF);
	file[idx].setNextPointer(0xFFFF);
	file[idx].setAttr(S_IFREG | 0777);
	file[idx].setDateTime(time(NULL));
	log_msg("The file's index : %d, pointing to %d\n", idx, file[idx].getPointer());
	
	return 0;
}

/** Create a directory */
int implement_mkdir(const char *path, mode_t mode) {
    log_msg("\nimplement_mkdir(path=\"%s\")\n", path);
    
    if (getLastPath(std::string(path+1)).length() > 20) // the length of the name is longer than 20 char, it may be invoked by terminal
		return -ENAMETOOLONG;
	if (volume_information.getNumbFreeBlock() == 0) // check whether there is a block left
		return -EDQUOT;
	
	int idx = searchFile(path);
	
	if (idx >= 0) // the folder already exists
		return -EEXIST;
		
	// alocate new block
	idx = volume_information.frontBlock();
	volume_information.popBlock();
	
	// edit the previous entry pointer
	int prev_idx = searchFile(std::string("/"+removeLastPath(std::string(path+1))).c_str());
	if (file[prev_idx].getPointer() != INVALID_BLOCK) { // if inside pointer is already filled, add to allocation table
		prev_idx = file[prev_idx].getPointer();
		while (file[prev_idx].getNextPointer() != INVALID_BLOCK)
			prev_idx = file[prev_idx].getNextPointer();
		file[prev_idx].setNextPointer(idx);
	}
	else
		file[prev_idx].setPointer(idx);
	
	// set up the folder entry
	file[idx].setName(getLastPath(std::string(path+1)));
	file[idx].setSize(0);
	file[idx].setPointer(INVALID_BLOCK);
	file[idx].setNextPointer(INVALID_BLOCK);
	file[idx].setAttr(S_IFDIR | 0777);
	file[idx].setDateTime(time(NULL));
	log_msg("Folder's index : %d, pointing to %d\n", idx, file[idx].getPointer());
	
	return 0;
}

/** Remove a file */
int implement_unlink(const char *path) {
    log_msg("\nimplement_unlink(path=\"%s\")\n", path);
    
    int idx = searchFile(path);
    log_msg("Index's file : %d, file's inside : %d\n", idx, file[idx].getPointer());
	int ptr = searchPrevFile(path); // make no block point to the data again
	if (ptr != INVALID_BLOCK) {
		log_msg("Previous file : %d\n", ptr);
		file[ptr].setNextPointer(file[idx].getNextPointer());
	}
	else {
		ptr = searchParentFolder(path);
		log_msg("Previous folder : %d\n", ptr);
		file[ptr].setPointer(file[idx].getNextPointer());
	}
	
	std::stack<int> data;
	data.push(idx);
	idx = file[idx].getPointer();
	
	while (idx != INVALID_BLOCK) { // add all data from a file
		data.push(idx);
		log_msg("Push to pop : %d\n", idx);
		idx = file[idx].getNextPointer();
	}
	while (!data.empty()) { // add to free block pointer
		volume_information.pushBlock(data.top());
		data.pop();
	}
	
	return 0;
}

int implement_rmdir(const char *path) {
    log_msg("\nimplement_rmdir(path=\"%s\")\n", path);
    
    int idx = searchFile(path);
    
	if (file[idx].getPointer() != INVALID_BLOCK) // the folder is not empty
		return -ENOTEMPTY;
	
	if (idx >= 0) {
		int ptr = searchPrevFile(path);
		if (ptr != INVALID_BLOCK) {
			file[ptr].setNextPointer(file[idx].getNextPointer());
		}
		else {
			ptr = searchParentFolder(path);
			file[ptr].setPointer(file[idx].getNextPointer());
		}
		volume_information.pushBlock(idx);
	}
	
	return 0;
}

//int implement_symlink(const char *path, const char *) {return 0;}

int implement_rename(const char *path, const char *path_new) {
    log_msg("\nimplement_rename(path=\"%s\", path_new=\"%s\")\n", path, path_new);
    
    std::string rename = getLastPath(std::string(path_new+1));
	if (rename.length() > 20) // the length of the name is longer than 20 char
		return -ENAMETOOLONG;
	
    int idx = searchFile(path); // the current block
    int prev = searchPrevFile(path);
    if (prev != INVALID_BLOCK) {
    	log_msg("Prev file : %d\n", prev);
    	file[prev].setNextPointer(file[idx].getNextPointer());
    }
    else {
    	prev = searchParentFolder(path);
    	log_msg("Prev folder : %d\n", prev);
    	file[prev].setPointer(file[idx].getNextPointer());
    }

    int prev_new = searchParentFolder(path_new);
    log_msg("Parent folder : %d\n", prev_new);
    file[idx].setNextPointer(file[prev_new].getPointer()); // change the next pointer
    file[prev_new].setPointer(idx); // set to first pointer only
	
	file[idx].setName(rename); // rename it
	return 0;
}

//int implement_link(const char *path, const char *) {return 0;}
//int implement_chmod(const char *path, mode_t) {return 0;}
//int implement_chown(const char *path, uid_t, gid_t) {return 0;}

/* Change the size of the file */
int implement_truncate(const char *path, off_t newsize) {
	log_msg("\nimplement_truncate(path=\"%s\", offset = %d)\n", path, (int) newsize);
	
	int idx = searchFile(path);
	
	file[idx].setSize(newsize);
	int ptr = file[idx].getPointer();
	
	int count = 0;
	while (count*BLOCK_SIZE < newsize) {
		if (ptr == INVALID_BLOCK) {
			ptr = volume_information.frontBlock();
			volume_information.popBlock();
			file[idx].setNextPointer(ptr);
			file[ptr].setNextPointer(INVALID_BLOCK);
		}
		idx = ptr;
		ptr = file[idx].getNextPointer();
		count++;
	}

	if (count == 0) // set to INVALID_BLOCK the previous block
		file[idx].setPointer(INVALID_BLOCK);
	else
		file[idx].setNextPointer(INVALID_BLOCK);

	std::stack<int> data;
	while (ptr != INVALID_BLOCK) {
		data.push(ptr);
		log_msg("Push to stack : %d\n", ptr);
		ptr = file[ptr].getNextPointer();
	}
	while (!data.empty()) {
		volume_information.pushBlock(data.top());
		data.pop();
	}

	return newsize;
}
	
/** File open operation
 *
 * No creation, or truncation flags (O_CREAT, O_EXCL, O_TRUNC)
 * will be passed to open().  Open should check if the operation
 * is permitted for the given flags.  Optionally open may also
 * return an arbitrary filehandle in the fuse_file_info structure,
 * which will be passed to all file operations.
 *
 */
int implement_open(const char *path, struct fuse_file_info *fi) {
    log_msg("\nimplement_open(path=\"%s\")\n", path);
    
	int idx = searchFile(path);
	
	if (idx < 0) // error no such file
		return -ENOENT;
	return 0;
}

/** Read data from an open file
 *
 * Read should return exactly the number of bytes requested except
 * on EOF or error, otherwise the rest of the data will be
 * substituted with zeroes.  An exception to this is when the
 * 'direct_io' mount option is specified, in which case the return
 * value of the read system call will reflect the return value of
 * this operation.
 *
 */
int implement_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
	log_msg("\nimplement_read(path=\"%s\")\n", path);
	log_msg("Size = %d, Offset = %d\n", size, offset);
	
	int idx = searchFile(path);
	if (idx < 0) // error no such file
		return -ENOENT;
	
	int file_size = file[idx].getSize(); // the size of the file
	
	if (offset + size > file_size) // if offset+size is larger than file size, then size needs to be reduced
		size = file_size - offset;
	int size_read = size; // the expected of byte that is going to be read
	
	idx = file[idx].getPointer(); // idx is now currently pointing the data
	while (offset >= BLOCK_SIZE) {
		idx = file[idx].getNextPointer();
		offset -= BLOCK_SIZE;
	}

	log_msg("First data read : %d\n", idx);
	
	int pos = 0;
	while (size > 0) {
		log_msg("Size left = %d, Index : %d\n", size, idx);
		if (size+offset > BLOCK_SIZE) {
			memcpy(buf+pos, file[idx].currentPosHandler()+offset, BLOCK_SIZE-offset);
			pos += BLOCK_SIZE-offset;
			size -= BLOCK_SIZE-offset;
		}
		else {
			memcpy(buf+pos, file[idx].currentPosHandler()+offset, size);
			pos += size;
			size = 0;
		}
		offset = 0;
		idx = file[idx].getNextPointer();
	}
	log_msg("End of read\n");
	
	return size_read;
}

/** Write data to an open file
 *
 * Write should return exactly the number of bytes requested
 * except on error.  An exception to this is when the 'direct_io'
 * mount option is specified (see read operation).
 *
 */
int implement_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
	log_msg("\nimplement_write(path=\"%s\")\n", path);
	log_msg("Size : %d, Offset : %d\n", size, offset);
	
	int init_size = size;
	
	int idx = searchFile(path); // search the file
	file[idx].setSize(file[idx].getSize()+size); // add to the file's size first
	
	if (idx < 0) // error no such file
		return -ENOENT;
	
	int ptr = file[idx].getPointer(); // point to file's data
	log_msg("Index mula-mula : %d, Pointer mula-mula : %d\n", idx, ptr);
	if (ptr == INVALID_BLOCK) { // new file needs a new block
		ptr = volume_information.frontBlock();
		volume_information.popBlock();

		file[idx].setPointer(ptr);
		file[ptr].setNextPointer(INVALID_BLOCK); // set new data's next pointer to INVALID_BLOCK
	}
	
	// switch block until offset is less than a BLOCK_SIZE 
	while (offset >= BLOCK_SIZE) {
		idx = ptr;
		ptr = file[idx].getNextPointer();
		offset -= BLOCK_SIZE;
	}

	log_msg("Pointing to data : %d w/ Pointer : %d\n", idx, ptr);
	
	int pos = 0;
	while (size > 0) {
		if (ptr == INVALID_BLOCK) { // if the next block hasn't been created yet
			if (volume_information.getNumbFreeBlock() != 0) {
				ptr = volume_information.frontBlock();
				volume_information.popBlock();
				
				log_msg("Create Index %d pointing to %d\n", idx, ptr);

				file[idx].setNextPointer(ptr);
				file[ptr].setNextPointer(INVALID_BLOCK);
			}
			else
				return -EFBIG; // File too large 
		}
		
		idx = ptr; // switch to idx
		ptr = file[idx].getNextPointer(); // ptr still becomes idx's next pointer
		log_msg("Index %d pointing to %d\n", idx, ptr);

		if (size+offset >= BLOCK_SIZE) {
			memcpy(file[idx].currentPosHandler()+offset, buf+pos, BLOCK_SIZE-offset);
			pos += BLOCK_SIZE-offset;
			size -= BLOCK_SIZE-offset;
		}
		else {
			memcpy(file[idx].currentPosHandler()+offset, buf+pos, size);
			pos += size;
			size = 0;
		}
		offset = 0;
	}
	log_msg("End of write\n");
	return init_size;
}
//int implement_statfs(const char *path, struct statvfs *) {return 0;}
//int implement_flush(const char *path, struct fuse_file_info *fi) {return 0;}
//int implement_release(const char *path, struct fuse_file_info *fi) {return 0;}
//int implement_fsync(const char *path, int, struct fuse_file_info *fi) {return 0;}
//int implement_setxattr(const char *path, const char *, const char *, size_t size, int) {return 0;}
//int implement_getxattr(const char *path, const char *, char *buf, size_t size) {return 0;}
//int implement_listxattr(const char *path, char *buf, size_t size) {return 0;}
//int implement_removexattr(const char *path, const char *) {return 0;}
//int implement_opendir(const char *path, struct fuse_file_info *fi) {return 0;}

/** Read directory
 *
 * This supersedes the old getdir() interface.  New applications
 * should use this.
 *
 * The filesystem may choose between two modes of operation:
 *
 * 1) The readdir implementation ignores the offset parameter, and
 * passes zero to the filler function's offset.  The filler
 * function will not return '1' (unless an error happens), so the
 * whole directory is read in a single readdir operation.  This
 * works just like the old getdir() method.
 *
 * 2) The readdir implementation keeps track of the offsets of the
 * directory entries.  It uses the offset parameter and always
 * passes non-zero offset to the filler function.  When the buffer
 * is full (or an error happens) the filler function will return
 * '1'.
 *
 */
int implement_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
    log_msg("\nimplement_readdir(path=\"%s\", buf=0x%08x, filler=0x%08x, offset=%lld, fi=0x%08x)\n", path, buf, filler, offset, fi);
	
	filler(buf, ".", NULL, 0); // current directory
	filler(buf, "..", NULL, 0); // parent directory
		
	int idx = searchFile(path);
	
	idx = file[idx].getPointer(); // inside directory
	while (idx != INVALID_BLOCK) {
		filler(buf, file[idx].getName().c_str(), NULL, 0);
		idx = file[idx].getNextPointer();
	}
	
	return 0;
}
//int implement_releasedir(const char *path, struct fuse_file_info *fi) {return 0;}
//int implement_fsyncdir(const char *path, int, struct fuse_file_info *fi) {return 0;}
//void *implement_init(struct fuse_conn_info *conn) {return IMPLEMENT_DATA;}
void implement_destroy(void *) {
	log_msg("\nimplement_destroy(void *)\n");
	file_handler.write(handler, FILESYSTEM_SIZE);
	file_handler.close();
}
//int implement_access(const char *path, int) {return 0;}
//int implement_create(const char *path, mode_t, struct fuse_file_info *fi) {return 0;}
//int implement_ftruncate(const char *path, off_t offset, struct fuse_file_info *fi) {return 0;}
//int implement_fgetattr(const char *path, struct stat *stbuf, struct fuse_file_info *fi) {return 0;}
//int implement_lock(const char *path, struct fuse_file_info *fi, int cmd, struct flock *) {return 0;}
//int implement_utimens(const char *path, const struct timespec tv[2]) {return 0;}
//int implement_bmap(const char *path, size_t size blocksize, uint64_t *idx) {return 0;}
//int implement_ioctl(const char *path, int cmd, void *arg, struct fuse_file_info *fi, unsigned int flags, void *data) {return 0;}
//int implement_poll(const char *path, struct fuse_file_info *fi, struct fuse_pollhandle *ph, unsigned *reventsp) {return 0;}
//int implement_write_buf(const char *path, struct fuse_bufvec *buf, off_t offset off, struct fuse_file_info *fi) {return 0;}
//int implement_read_buf(const char *path, struct fuse_bufvec **bufp, size_t size size, off_t offset off, struct fuse_file_info *fi) {return 0;}
//int implement_flock(const char *path, struct fuse_file_info *fi, int op) {return 0;}
//int implement_fallocate(const char *path, int, off_t offset, off_t offset, struct fuse_file_info *fi) {return 0;}

static struct fuse_operations implement_oper;
void function_pointer() {
	implement_oper.getattr = implement_getattr;
	implement_oper.mknod = implement_mknod;
	implement_oper.mkdir = implement_mkdir;
	implement_oper.unlink = implement_unlink;
	implement_oper.rmdir = implement_rmdir;
	implement_oper.rename = implement_rename;
	implement_oper.truncate = implement_truncate;
	implement_oper.open = implement_open;
	implement_oper.read = implement_read;
	implement_oper.write = implement_write;
	implement_oper.readdir = implement_readdir;
	implement_oper.destroy = implement_destroy;
}

int main(int argc, char *argv[]) {
	if (not ((3 <= argc) && (argc <= 4))) {
		implement_usage();
		return 0;
	}
	
	if (argc == 4)
		createFile(argv[2]);
	openFile(argv[2]);
	
	time(&mount_time);
	function_pointer();
	
	volume_information.init();
	file_class::init();
	
	if (volume_information.getName() != "CCFS") {
		std::cerr << "Failed to load" << std::endl;
		return 0;
	}

	// for log
    struct implement_state *implement_data;
    implement_data = (implement_state *) malloc(sizeof(struct implement_state));
    implement_data->logfile = log_open();
	
	// inisialisasi fuse
	argc = 2;

	file_handler.open(argv[2], std::fstream::in | std::fstream::out | std::fstream::binary | std::fstream::trunc);
	//return 0;
	return fuse_main(argc, argv, &implement_oper, implement_data);
}