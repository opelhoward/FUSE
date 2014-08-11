#include "filesystem.hpp"

#include <fstream>
#include <cstdio>
#include <iostream>

#define BASE 256
#define ROOT "root"

// method implementation
std::string getFirstPath(std::string path){
	int found = path.find_first_of("/");
	if (found != (int) std::string::npos)
		path = path.substr(0, found);
	return path;
}
std::string getLastPath(std::string path){
	int found = path.find_last_of("/");
	if (found != (int) std::string::npos)
		path = path.substr(found+1);
	return path;
}
std::string removeFirstPath(std::string path){
	int found = path.find_first_of("/");
	if (found != (int) std::string::npos)
		path = path.substr(found+1);
	else
		path = "";
	return path;
}
std::string removeLastPath(std::string path){
	int found = path.find_last_of("/");
	if (found != (int) std::string::npos)
		path = path.substr(0, found);
	else
		path = "";
	return path;
}
int searchFile(const char* path) {
	std::string addr = std::string(path+1);
	
	int curr_idx_node = 0;
	while (addr != "") {
		curr_idx_node = file[curr_idx_node].getPointer(); // new idx position
		std::string curr_node = getFirstPath(addr);	// current directory
			
		while ((file[curr_idx_node].getName() != curr_node) && (curr_idx_node != INVALID_BLOCK))
			curr_idx_node = file[curr_idx_node].getNextPointer();
		
		if (curr_idx_node == INVALID_BLOCK) // invalid idx
			return -1;
		
		addr = removeFirstPath(addr);
	}
	return curr_idx_node;
}

int searchParentFolder(const char* path) {
	std::string addr = removeLastPath(std::string(path+1));
	
	int idx = searchFile(("/"+addr).c_str()); // up one level from the destination
	return idx;
}
int searchPrevFile(const char* path) {
	int idx = searchParentFolder(path); // up one level from the destination
	int ptr = file[idx].getPointer(); // get inside from the parent
	
	std::string name = getLastPath(std::string(path+1)); 
	if (name == file[ptr].getName()) // if the first pointer is already the file
		return INVALID_BLOCK;

	while (ptr != INVALID_BLOCK) { // if it gets the same name, it needs to stop
		if (name == file[ptr].getName())
			break;

		idx = ptr;
		ptr = file[idx].getNextPointer();
	}
	return idx;
}
void openFile(const char* filesystem_name) {
	FILE *filesystem = fopen(filesystem_name, "r");
	fread(handler, FILESYSTEM_SIZE, 1, filesystem);
}

void createFile(const char* filesystem_name) {
	int iterator;
	int it;

	FILE *filesystem = fopen(filesystem_name, "w");

	printf("Writing to %s\n", filesystem_name);

	fprintf(filesystem, "CCFS"); // Magic string “CCFS” (tanpa kutip). Magic string ini digunakan untuk melakukan validasi bahwa file yang dibaca adalah file .ccfs
	for (iterator = 0; iterator < 32; ++iterator) // Sebuah null-terminated string yang menyimpan nama dari volume. Karakter yang digunakan ditulis dalam format yang dapat dibaca oleh manusia. Jika tak diisikan maka secara default akan berisi “CCFS”.
		fprintf(filesystem, "%c", 0);
	fprintf(filesystem, "%c%c%c%c", 0x00, 0x00, 0x01, 0x00); // Kapasitas filesystem dalam blok, ditulis dalam integer 32-bit little endian.
	fprintf(filesystem, "%c%c%c%c", 0xFF, 0xFF, 0x00, 0x00); // Jumlah blok yang belum terpakai, ditulis dalam integer 32-bit little endian.
	fprintf(filesystem, "%c%c%c%c", 0x01, 0x00, 0x00, 0x00); // Indeks blok pertama yang bebas, ditulis dalam integer 32-bit little endian.
	for (iterator = 0; iterator < 460; ++iterator) // Tidak digunakan, diisi nilai 0 / karakter null.
		fprintf(filesystem, "%c", 0x00);
	fprintf(filesystem, "SFCC");

	// Nilai 0x0000 menandakan bahwa blok tersebut kosong, sedangkan nilai 0xFFFF menandakan bahwa blok tersebut tidak memiliki next.

	fprintf(filesystem, "%c%c", 0xFF, 0xFF); // Pointer pertama (ke-0) selalu digunakan untuk blok root (blok ke-0 pada data pool)
	for (iterator = 1; iterator < 65536; ++iterator) { // Nilai 0x00 menandakan bahwa blok tersebut kosong
		fprintf(filesystem, "%c%c", (char) ((iterator+1)%BASE), (char) ((iterator+1)/BASE));
	}
	// Untuk root folder
	fprintf(filesystem, "%s", ROOT); // Nama file atau direktori
	for (iterator = 0; iterator < 21-4; ++iterator) {
		fprintf(filesystem, "%c", 0x00);
	}
	fprintf(filesystem, "%c", 0x0F); // Atribut file atau direktori
	fprintf(filesystem, "%c%c", 0xFF, 0xFF); // Waktu pembuatan atau modifikasi terakhir
	fprintf(filesystem, "%c%c", 0xFF, 0xFF); // Tanggal pembuatan atau modifikasi terakhir
	fprintf(filesystem, "%c%c", 0xFF, 0xFF); // Indeks blok data pool pertama penyimpanan file / direktori tersebut, dalam integer 16-bit little endian
	fprintf(filesystem, "%c%c%c%c", 0x00, 0x00, 0x00, 0x00); // Ukuran file dalam byte, ditulis dalam integer 32-bit little endian
	for (it = 0; it < (512-32); ++it)
		fprintf(filesystem, "%c", 0x00);

	// Sisa dari Data Pool
	for (iterator = 1; iterator < 65536; ++iterator) {
		for (it = 0; it < 512; ++it)
			fprintf(filesystem, "%c", 0x00);
	}
	
	fclose(filesystem);
}
