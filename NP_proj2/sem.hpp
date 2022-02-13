#ifndef __SEM_HPP__
#define __SEM_HPP__

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include "multi_utility.hpp"

static struct sembuf op_lock[2] = {
    2, 0, 0,
    2, 1, SEM_UNDO	
};

static struct sembuf op_endcreate[2] = {
    1, -1, SEM_UNDO,
    2, -1, SEM_UNDO	
};

static sembuf opopen[1] = {
    1, -1, SEM_UNDO
};

static struct sembuf op_close[3] = {
	2, 0, 0,
	2, 1, SEM_UNDO,
	1, 1, SEM_UNDO
};

static struct sembuf op_unlock[1] = {
	2, -1, SEM_UNDO
};

static struct sembuf op_op[1] = {
    0, 99, SEM_UNDO	
};

int sem_create(key_t key, int initval){
    register int id, semval;
    /* not intended for private semaphores */
    // if (key == IPC_PRIVATE) return(-1);  
    /* probably an ftok() error by caller
    else */ if (key == (key_t) -1)   return -1;	

    again:
    if ( (id = semget(key, 3, 0666 | IPC_CREAT)) < 0) 
        return -1;	
    if (semop(id, &op_lock[0], 2) < 0) {
        if (errno == EINVAL) goto again;
        perror("semop 1");
    }
    if ( (semval = semctl(id, 1, GETVAL, 0)) < 0)
        perror("semctl 1");
    if (semval == 0) {
        if (semctl(id, 0, SETVAL, initval) < 0)
            perror("semctl 2");
        if (semctl(id, 1, SETVAL, MAXUSERPIPE) < 0)
            perror("semctl 3");
    }
    if (semop(id, &op_endcreate[0], 2) < 0)
        perror("semop 2");

    return id;
}

int sem_open(key_t key) {
    register int id;
    if (key == IPC_PRIVATE) {
        return -1;
    } else if (key == (key_t) -1) {
        return -1;

    }
    if ((id = semget(key, 3, 0)) < 0) {
        return -1;
    } 
    if (semop(id, &opopen[0], 1) , 0) {
        cerr << "can't open.\n";
    }
    return id;
}

void sem_rm(int id){
	if (semctl(id, 0, IPC_RMID, 0) < 0)
		perror("semctl 4");
}

void sem_close(int id){
	register int semval;
	if (semop(id, &op_close[0], 3) < 0) 
        perror("semop 3");

	if ( (semval = semctl(id, 1, GETVAL, 0)) < 0) 
        perror("semctl 5");
	if (semval > MAXUSERPIPE) 
        cout << "> MAXUSERPIPE\n" << flush;
	else if (semval == MAXUSERPIPE) 
        sem_rm(id);
	else if (semop(id, &op_unlock[0], 1) < 0) 
        perror("unlock");  /* unlock */
}



void sem_op(int id, int value){
	if ((op_op[0].sem_op = value) == 0)  
        perror("value == 0");
	if (semop(id, &op_op[0], 1) < 0)  
        perror("semop 4");
}

void sem_wait(int id){
	sem_op(id, -1);
}

void sem_signal(int id){
	sem_op(id, 1);
}

void my_lock(int fd){
	if (semid < 0) {
		if ( (semid = sem_create(SEMKEY, 1)) < 0) 
            perror("sem_create");
	}
	sem_wait(semid);
}

void my_unlock(int fd){
	sem_signal(semid);
}

#endif