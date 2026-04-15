#include "linux_shm.h"
#include <stdio.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <errno.h>

#define PROJ_ID    0x03

int create_shm(char *name, unsigned int size)
{
    int shmid;
    key_t shm_key;

    if ((shm_key = ftok(name, PROJ_ID)) == (key_t) -1) {
        perror(name);
        return -1;
    }

    shmid = shmget(shm_key, size, 0666|IPC_CREAT);
    if(shmid < 0 ) {
        perror(name);
        if (errno != EEXIST){
            return -1;
        }else {
            //shm already exists
            if((shmid = shmget(shm_key, size, SHM_R | SHM_W)) < 0) {
                perror(name);
                return -1;
            }
        }
    }

    return shmid;
}

char* map_shm(int shmid)
{
    char* pt;
    pt = (char *)shmat((int)shmid, 0, SHM_RND);
    if(pt == NULL) {
        perror("map error");
    }

    return pt;
}

void unmap_shm(char *shmaddr)
{
    shmdt(shmaddr);
    return ;
}

void close_shm(int shmid)
{
    struct shmid_ds buf;
    shmctl((int)shmid, IPC_RMID, &buf);
    return ;
}
