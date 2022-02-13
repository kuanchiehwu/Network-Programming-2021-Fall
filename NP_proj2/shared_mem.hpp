#ifndef __SHARED_MEM_HPP__
#define __SHARED_MEM_HPP__

#include "sem.hpp"

void broadcast_msg(char* msg){
    for(int i=0; i<MAXUSERS; i++){
        if(UserList[i].active == true){
            int msgBuf_id = UserList[i].msgBuf_id;
            char *msgBuf = (char *) shmat(msgBuf_id, NULL, 0);
            strcpy(msgBuf, msg);
            shmdt(msgBuf);

            kill(UserList[i].pid, SIGUSR1);
        }
    }
}

void who(int id){
    cout << "<ID>    <nickname>  <IP:port>   <indicate me>\n" << flush;

    for(int i=0; i<MAXUSERS; i++){
        if(UserList[i].active == true){
            cout << (i + 1) << "    " << UserList[i].name << "    " <<
                UserList[i].ip << ":" << UserList[i].port << flush;
            if(i == id)
                cout << "    <-me" << flush;
            cout << "\n" << flush;
        }
    }
}

void tell(int id, int recv_id, vector<string> cmd){
    my_lock(semid);
    if(recv_id > 30 || UserList[recv_id-1].active == false){
        cout << "*** Error: user #" << recv_id << " does not exist yet. ***\n" << flush;
        my_unlock(semid);
        return;
    }
    // strcpy(msgBuffer, tell_msg(id, cmd));
    int msgBuf_id = UserList[recv_id - 1].msgBuf_id;
    char *msgBuf = (char *) shmat(msgBuf_id, NULL, 0);
    strcpy(msgBuf, tell_msg(id, cmd));
    shmdt(msgBuf);

    kill(UserList[recv_id-1].pid, SIGUSR1);
    
    my_unlock(semid);
}

void yell(int id, vector<string> cmd){
    my_lock(semid);
    broadcast_msg(yell_msg(id, cmd));
    my_unlock(semid);
}

void name(int id, char* input_name){
    my_lock(semid);
    for(int i=0; i<MAXUSERS; i++){
        if(strcmp(UserList[i].name, input_name) == 0 && UserList[i].active == true){
            cout << "*** User '" << flush;
            cout << input_name << flush;
            cout << "' already exists. ***\n" << flush;
            my_unlock(semid);
            return;
        }
    }

    strcpy(UserList[id].name, input_name);
    
    broadcast_msg(change_name_msg(id, input_name));
    my_unlock(semid);
}

void create_shm(){
    UserList_id = shmget(IPC_PRIVATE, sizeof(User) * MAXUSERS, IPC_CREAT | IPC_EXCL | 0666);
    if(UserList_id < 0)
        perror("shmget");
    UserList = (User *) shmat(UserList_id, NULL, 0);
    for(int i=0; i<MAXUSERS; i++){
        UserList[i].active = false;
        strcpy(UserList[i].name, "(no name)");
    }

    // msgBuffer_id = shmget(IPC_PRIVATE, BUFSIZE, IPC_CREAT | IPC_EXCL | 0666);
    // msgBuffer = (char *) shmat(msgBuffer_id, NULL, 0);

    UserPipeTable_id = shmget(IPC_PRIVATE, sizeof(UserPipeInfo) * MAXUSERPIPE, IPC_CREAT | IPC_EXCL | 0666);
    UserPipeTable = (UserPipeInfo *) shmat(UserPipeTable_id, NULL, 0);
    for(int i=0; i<MAXUSERPIPE; i++){
        UserPipeTable[i].ID_sent = -1;
        UserPipeTable[i].ID_recv = -1;
        UserPipeTable[i].active = false;
    }

    if ((semid = sem_create(SEMKEY, 1)) < 0) 
        perror("sem_create");
    // cout << "semid: " << semid << "\n" << flush;
}

int add_user(struct sockaddr_in cli_addr){
    int i;
    for(i=0; i<MAXUSERS; i++){
        if(UserList[i].active == false){
            UserList[i].active = true;
            strcpy(UserList[i].ip, inet_ntoa(cli_addr.sin_addr));
            UserList[i].port = ntohs(cli_addr.sin_port);
            UserList[i].pid = getpid();
            UserList[i].msgBuf_id = shmget(IPC_PRIVATE, sizeof(char) * BUFSIZE, IPC_CREAT | IPC_EXCL | 0666);
            char *msgBuf = (char *) shmat(UserList[i].msgBuf_id, NULL, 0);
            // memset(msgBuf, 0, sizeof(char) * BUFSIZE);
            shmdt(msgBuf);
            
            break;
        }
    }
    return i;
}

void close_user_pipe(int id){
    for(int i=0; i<MAXUSERPIPE; i++){
        if(UserPipeTable[i].active){
            if(UserPipeTable[i].ID_sent == id || UserPipeTable[i].ID_recv == id){
                close(UserPipeTable[i].in_pipe);
                close(UserPipeTable[i].out_pipe);
                UserPipeTable[i].active = false;
            }
        }
    }
}

void user_left(int id){
    my_lock(semid);
    UserList[id].active = false;

    int msgBuf_id = UserList[id].msgBuf_id;
    shmctl(msgBuf_id, IPC_RMID, 0);

    broadcast_msg(logout_msg(id));
    strcpy(UserList[id].name, "(no name)");

    // close user pipe about id
    for(int i=0; i<MAXUSERPIPE; i++){
        if(UserPipeTable[i].active){
            if(UserPipeTable[i].ID_sent == id || UserPipeTable[i].ID_recv == id){
                close(UserPipeTable[i].in_pipe);
                close(UserPipeTable[i].out_pipe);
                UserPipeTable[i].active = false;
            }
        }
    }
    my_unlock(semid);
}

void remove_shm(int signo){
    for(int i=0; i<MAXUSERS; i++){
        if(UserList[i].active){
            int msgBuf_id = UserList[i].msgBuf_id;
            shmctl(msgBuf_id, IPC_RMID, 0);
        }
    }

    shmctl(UserList_id, IPC_RMID, 0);
    shmctl(UserPipeTable_id, IPC_RMID, 0);

    sem_close(semid);
    exit(0);
}

#endif