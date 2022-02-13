#ifndef __MULTI_FUNCTION_HPP__
#define __MULTI_FUNCITON_HPP__

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <vector>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <signal.h>
#include <math.h>
#include <sstream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <tuple>
#include <algorithm>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <cstring>
using namespace std;

#define MAXUSERS 30 + 5
#define BUFSIZE 15000
#define MAXUSERPIPE 900

struct User{
    bool active = false;
    int pid;
    char name[30] = "(no name)";
    char ip[30];
    int port;
    int msgBuf_id;
};

struct UserPipeInfo{
    int ID_sent;
    int ID_recv;
    int in_pipe;
    int out_pipe;
    bool active = false;
    int read_fd;
    int write_fd;
};

int UserList_id;
User *UserList;

int id_now = -1;

// int msgBuffer_id;
// char *msgBuffer;

int UserPipeTable_id;
UserPipeInfo *UserPipeTable;

#define	SEMKEY	123456L	/* key value for sem_create() */
int semid = -1;	/* semaphore id */

const string wel_msg = "****************************************\n"\
                       "** Welcome to the information server. **\n"\
                       "****************************************\n";

char* login_msg(int id){
    string s = "*** User '" + string(UserList[id].name) + "' entered from " +
               string(UserList[id].ip) + ":" + to_string(UserList[id].port) +
               ". ***\n";
    return strdup(s.c_str());
}

char* logout_msg(int id){
    string s = "*** User '" + string(UserList[id].name) + "' left. ***\n";
    return strdup(s.c_str());
}

char* change_name_msg(int id, char* input_name){
    string s = "*** User from " + string(UserList[id].ip) + ":" +
               to_string(UserList[id].port) + " is named '" +
               string(input_name) + "'. ***\n";
    return strdup(s.c_str());
}

char* tell_msg(int sent_id, vector<string> cmd){
    string msg = "";
    for(size_t i=2; i<cmd.size(); i++){
        msg += cmd[i];
        if(i != cmd.size() - 1)
            msg += " ";
    }
    string s = "*** " + string(UserList[sent_id].name) + " told you ***: " + msg + "\n";
    return strdup(s.c_str());
}

char* yell_msg(int sent_id, vector<string> cmd){
    string msg = "";
    for(size_t i=1; i<cmd.size(); i++){
        msg += cmd[i];
        if(i != cmd.size() - 1)
            msg += " ";
    }
    string s = "*** " + string(UserList[sent_id].name) + " yelled ***: " + msg + "\n";
    return strdup(s.c_str());
}

char* recv_user_pipe_msg(int sent_id, int recv_id, string cmd){
    string s = "*** " + string(UserList[recv_id].name) + " (#" + to_string(recv_id + 1) + 
               ") just received from " + UserList[sent_id].name + " (#" + 
               to_string(sent_id + 1) + ") by '" + cmd + "' ***\n";
    return strdup(s.c_str());
}

char* sent_user_pipe_msg(int sent_id, int recv_id, string cmd){
    string s = "*** " + string(UserList[sent_id].name) + " (#" + to_string(sent_id + 1) +
               ") just piped '" + cmd + "' to " + string(UserList[recv_id].name) + 
               " (#" + to_string(recv_id + 1) + ") ***\n";
    return strdup(s.c_str());
}

char* int2char(int i){
    string s = to_string(i);
    return strdup(s.c_str());
}

void remove_string_tail(string &s){
    s.erase(remove(s.begin(), s.end(), '\n'), s.end());
    s.erase(remove(s.begin(), s.end(), '\r'), s.end());
}

#endif