#ifndef __FUNCTION_HPP__
#define __FUNCITON_HPP__

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
using namespace std;

enum Pipe_Type{
    ordinary,
    num_pipe,
    num_pipe_err
};

struct Num_Pipe_Info{
    Pipe_Type pipe_type;
    int in_pipe;
    int out_pipe;
    int count;
};

struct client{
    bool active = true;
    int fd;
    int ID;
    string NAME = "(no name)";
    string IP;
    int port;
    vector<Num_Pipe_Info> num_pipe_table;
    vector<pair<string, string> > env;
};

vector<client> client_list;
fd_set rfds, afds;

extern char **environ;
vector<pair<string, string> > ori_env;
void save_ori_env(){
    char *env = *environ;
    for(int i = 1; env; i++){
        string s = string(env);
        size_t pos = s.find("=");
        ori_env.push_back(make_pair(s.substr(0, pos), s.substr(pos + 1, s.length() - pos)));

        env = *(environ + i);
    }
}

void prompt(int fd){
    string init = "% ";
    write(fd, init.c_str(), init.length());
}

void welcome_msg(int fd){
    string wel_msg = "****************************************\n"\
                         "** Welcome to the information server. **\n"\
                         "****************************************\n";

    write(fd, wel_msg.c_str(), wel_msg.length());
}

string login_msg(string name, string ip, int port){
    return "*** User '" + name + "' entered from " + ip + ":" + to_string(port) + ". ***\n";
}

string logout_msg(string name){
    return "*** User '" + name + "' left. ***\n";
}

string user_error_msg(int id){
    return "*** Error: user #" + to_string(id) + " does not exist yet. ***\n";
}

string recv_user_pipe_msg(int sent_id, int recv_id, string cmd){
    return "*** " + client_list[recv_id-1].NAME + " (#" + to_string(recv_id) + 
           ") just received from " + client_list[sent_id-1].NAME + " (#" + to_string(sent_id) +
           ") by '" + cmd + "' ***\n";
}

string user_pipe_not_exist_msg(int sent_id, int recv_id){
    return "*** Error: the pipe #" + to_string(sent_id) + "->#" + to_string(recv_id) + " does not exist yet. ***\n";
}

string user_pipe_exist_msg(int sent_id, int recv_id){
    return "*** Error: the pipe #" + to_string(sent_id) + "->#" + to_string(recv_id) + " already exists. ***\n";
}

string sent_user_pipe_msg(int sent_id, int recv_id, string cmd){
    return "*** " + client_list[sent_id - 1].NAME + " (#" + to_string(sent_id) + ") just piped '" +
           cmd + "' to " + client_list[recv_id - 1].NAME + " (#" + to_string(recv_id) + ") ***\n";
}

void print_msg(int fd, string s){
    write(fd, s.c_str(), s.length());
}

void broadcast_msg(string msg){
    for(size_t i=0; i<client_list.size(); i++){
        if(client_list[i].active == true){
            int fd = client_list[i].fd;
            print_msg(fd, msg);
        }
    }
}

void who(int id){
    string s = "<ID>    <nickname>  <IP:port>   <indicate me>\n";
    print_msg(client_list[id-1].fd, s);
    for(size_t i=0; i<client_list.size(); i++){
        if(client_list[i].active == true){
            string tab = "    ";
            s = to_string(client_list[i].ID) + tab + client_list[i].NAME + 
                tab + client_list[i].IP + ":" + to_string(client_list[i].port);
            if((int)i == (id - 1))
                s += (tab + "<-me");
            s += "\n";
            print_msg(client_list[id-1].fd, s);
        }
    }
}

void tell(int id, int rec_id, vector<string> cmd){
    string who_tell = client_list[id-1].NAME;
    string s, msg = "";
    if(rec_id > (int)client_list.size() || client_list[rec_id - 1].active == false){
        s = "*** Error: user #" + to_string(rec_id) + " does not exist yet. ***\n";
        print_msg(client_list[id-1].fd, s);
        return;
    }
    for(size_t i=2; i<cmd.size(); i++){
        msg += cmd[i];
        if(i != cmd.size() - 1)
            msg += " ";
    }
    s = "*** " + who_tell + " told you ***: " + msg + "\n";
    print_msg(client_list[rec_id - 1].fd, s);
}

void yell(int id, vector<string> cmd){
    string msg = "";

    for(size_t i=1; i<cmd.size(); i++){
        msg += cmd[i];
        if(i != cmd.size() - 1)
            msg += " ";
    }

    string s = "*** " + client_list[id-1].NAME + " yelled ***: " + msg + "\n";
    broadcast_msg(s);
}

void name(int id, string input_name){
    for(size_t i=0; i<client_list.size(); i++){
        if(client_list[i].NAME == input_name && client_list[i].active == true){
            string s = "*** User '" + input_name + "' already exists. ***\n";
            print_msg(client_list[id-1].fd, s);
            return;
        }
    }
    client_list[id-1].NAME = input_name;
    string s = "*** User from " + client_list[id-1].IP + ":" + 
               to_string(client_list[id-1].port) + 
               " is named '" + input_name + "'. ***\n";
    broadcast_msg(s);
}

int build_in_2(vector<string> cmd, int id){
    if(cmd[0] == "who"){
        who(id);
        return 1;
    }
    else if(cmd[0] == "tell"){
        tell(id, stoi(cmd[1]), cmd);
        return 1;
    }
    else if(cmd[0] == "yell"){
        yell(id, cmd);
        return 1;
    }
    else if(cmd[0] == "name"){
        name(id, cmd[1]);
        return 1;
    }
    return 0;
}

void save_env_change(int id, string name, string env){
    bool exist = false;
    for(size_t i=0; i<client_list[id-1].env.size(); i++){
        if(client_list[id-1].env[i].first == name){
            exist = true;
            client_list[id-1].env[i].second = env;
        }
    }
    if(!exist)
        client_list[id-1].env.push_back(make_pair(name, env));
}

void exchange_env(int id){
    for(size_t i=0; i<client_list[id-1].env.size(); i++){
        setenv(client_list[id-1].env[i].first.c_str(), client_list[id-1].env[i].second.c_str(), 1);
    }
}

void remove_string_tail(string &s){
    s.erase(remove(s.begin(), s.end(), '\n'), s.end());
    s.erase(remove(s.begin(), s.end(), '\r'), s.end());
}

#endif