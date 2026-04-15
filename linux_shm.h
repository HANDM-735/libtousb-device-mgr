#ifndef LINUX_SHM__H_H
#define LINUX_SHM__H_H

#include <sys/types.h>
#include <unistd.h>

int create_shm(char *name, unsigned int size);
char* map_shm(int shmid);
void unmap_shm(char *shmaddr);
void close_shm(int shmid);


#endif
