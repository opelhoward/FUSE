#include "file.hpp"

// file  implementation
void file_class::init() {
	char* data_pool;
	data_pool = &handler[BLOCK_SIZE+BLOCK_AMOUNT*2];
	for (int idx = 0; idx < BLOCK_AMOUNT; ++idx) {
		file[idx].name = &data_pool[idx*BLOCK_SIZE];
		file[idx].attr = &data_pool[idx*BLOCK_SIZE+21];
		file[idx].date = (ushort*)&data_pool[idx*BLOCK_SIZE+22];
		file[idx].time = (ushort*)&data_pool[idx*BLOCK_SIZE+24];
		file[idx].ptr = (ushort *) &data_pool[idx*BLOCK_SIZE+26];
		file[idx].size = (int *) &data_pool[idx*BLOCK_SIZE+28];
		file[idx].next_ptr = (ushort *) &handler[BLOCK_SIZE+idx*2];
	}
}
std::string file_class::getName() {
	std::string _name = "";
	for (int it = 0; name[it] != 0; ++it)
		_name += name[it];
	return _name;
}
void file_class::setName(std::string _name) {
	for (uint it = 0; it < 21; ++it)
		name[it] = 0x00;
	for (uint it = 0; it < _name.length(); ++it)
		name[it] = _name[it];
}
ushort file_class::getAttr() {
	short type = (*attr >> 3) & 1;
	short read = (*attr >> 2) & 1;
	short write = (*attr >> 1) & 1;
	short execute = *attr & 1;
	short _attr = 0;
	if (type == 1)
		_attr = S_IFDIR;
	else
		_attr = S_IFREG;
	_attr = _attr | (read << 2) | (write << 1) | (execute << 0) | (read << 5) | (write << 4) | (execute << 3) | (read << 8) | (write << 7) | (execute << 6);
	return _attr;
}
void file_class::setAttr(ushort _attr) {
	char type = (_attr >> 14) & 1;
	char read = (_attr >> 2) & 1;
	char write = (_attr >> 1) & 1;
	char execute = _attr & 1;
	*attr = (type << 3) |  (read << 2) | (write << 1) | execute;
}
time_t file_class::getDateTime() {
	// dalam bentuk integer..
	int d = (int)(*date & DAY_MASK); // tanggal
	int m = (int)((*date & MONTH_MASK) >> 5); // bulan
	int y = (int)((*date & YEAR_MASK) >> 9); // tahun
	
	int hh = (int)((*time & HH_MASK) >> 11); // jam
	int mm = (int)((*time & MM_MASK) >> 5); // menit
	int ss = (int)(*time & SS_MASK) << 1; // detik * 2
	
	// manggil struktur waktu C
	struct tm temp_time;
	
	temp_time.tm_sec = ss; // detik
	temp_time.tm_min = mm; // menit
	temp_time.tm_hour = hh; // jam
	
	temp_time.tm_mday = d; // hari
	temp_time.tm_mon = m-1; // x bulan dari Januari
	temp_time.tm_year = y+100; // x tahun dari epoch (1900)
	
	// sisanya ngga kepake
	temp_time.tm_wday = 0; // hari ke-x dalam minggu (Senin, 0-Minggu,6)
	temp_time.tm_yday = 0; // hari ke-x dari tahun
	temp_time.tm_isdst = 0; // daylight saving time flag
	
	return mktime(&temp_time);
}
void file_class::setDateTime(time_t t) {
	// buat struktur waktunya
	struct tm *temp_time = localtime(&t);
	
	// dalam bentuk ushort...
	ushort d = (ushort)(temp_time->tm_mday); // hari
	ushort m = (ushort)(temp_time->tm_mon+1); // bulan
	ushort y = (ushort)(temp_time->tm_year-100); // 2000 + tahun
	
	ushort hh = (ushort)(temp_time->tm_hour); // jam
	ushort mm = (ushort)(temp_time->tm_min); // menit
	ushort ss = (ushort)(temp_time->tm_sec >> 1); // detik/2
	
	// pasang!
	*date = ((y << 9) & YEAR_MASK) |
			((m << 5) & MONTH_MASK) |
			(d);
	*time = ((hh << 11) & HH_MASK) |
			((mm << 5) & MM_MASK) |
			(ss);
}
int file_class::getPointer() {
	return *ptr;
}
void file_class::setPointer(ushort _ptr) {
	*ptr = _ptr;
}
int file_class::getSize() {
	return *size;
}
void file_class::setSize(int _size) {
	*size = _size;
}
ushort file_class::getNextPointer() {
	return *next_ptr;
}
void file_class::setNextPointer(ushort _ptr) {
	*next_ptr = _ptr;
}
bool file_class::isEmpty() {
	if (name[0] == 0)
		return true;
	return false;
}
char* file_class::currentPosHandler() {
	return name;
}
