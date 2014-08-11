make:
	g++ -Wall implement.cpp filesystem.cpp volume_information.cpp file.cpp log.c `pkg-config fuse --cflags --libs` -o ccfs-mount
