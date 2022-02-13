#ifndef __NPSHELL_HPP__
#define __NPSHELL_HPP__

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <signal.h>
#include <math.h>
using namespace std;

int ForkCount = 0;
int MaxFork;
int pre_cmd = -1;

extern char **environ;

enum Pipe_Type{
    ordinary,
    num_pipe,
    num_pipe_err
};

struct Pipe_Info{
    int in_pipe;
    int out_pipe;
};

struct Num_Pipe_Info{
    Pipe_Type pipe_type;
    int in_pipe;
    int out_pipe;
    int count;
};

void init(){
    setenv("PATH", "bin:.", 1);
}

void sig_chld(int signo){
    wait(NULL);
    ForkCount--;
    return;
}

vector<string> split_pipe(const string &s){
    vector<string> cmds;
    string tmp = "";
    for(size_t i=0; i<s.length(); i++){
        if(s[i] == '|' && i < s.length()-2 && s[i+1] == ' '){
            cmds.push_back(tmp);
            tmp = "";
        }
        else if(s[i] != ' '){
            if(i > 0 && s[i-1] == ' ' && tmp!="")
                tmp += ' ';
            tmp += s[i];
        }
    }
    cmds.push_back(tmp);
    return cmds;
}

vector<vector<string> > split_cmd(const string &s){
    vector<vector<string> > cmds;
    cmds.clear();
    if (s.size() == 0) {
        return cmds;
    }
    vector<string> cmd = split_pipe(s);

    for(size_t i=0; i<cmd.size(); i++){
        vector<string> t;
        string tmp = "";
        for(size_t j=0; j<cmd[i].size(); j++){
            if(cmd[i][j] == ' '){
                t.push_back(tmp);
                tmp = "";
            }
            else
                tmp += cmd[i][j];
        }
        t.push_back(tmp);
        cmds.push_back(t);
    }
    return cmds;
}

void printenv_all(){
    char *env = *environ;
    for(int i = 1; env; i++){
        printf("%s\n", env);
        env = *(environ + i);
    }
}

void parse_build_in(vector<string> cmd){
    if(cmd[0] == "setenv")
        setenv(cmd[1].c_str(), cmd[2].c_str(), 1);
    else if(cmd[0] == "printenv"){
        if(cmd.size() == 1)
            printenv_all();
        else{
            char* env = getenv(cmd[1].c_str());
            if(env != NULL)
                printf("%s\n", env);
        }
    }
    else if(cmd[0] == "exit"){
        exit(0);
    }
}

bool build_in(vector<string> cmd){
    if(cmd[0] == "setenv" || cmd[0] == "printenv" || cmd[0] == "exit"){
        parse_build_in(cmd);
        return true;
    }
    return false;
}

void creat_pipe(int *pipefd, vector<Pipe_Info> &pipe_table){
    if(pipe(pipefd) < 0){
        perror("pipe error");
    }
    struct Pipe_Info pi;
    pi.in_pipe = pipefd[0];
    pi.out_pipe = pipefd[1];
    pipe_table.push_back(pi);
}

void preprocessing_redir(vector<string> &cmds, string &file){
    for(size_t j=0; j<cmds.size(); j++){
        if(cmds[j] == ">"){
            if(j == cmds.size() - 1){
                cmds.erase(cmds.begin() + j);
                cout << "error\n";
                break;
            }
            file = cmds[j+1];
            cmds.erase(cmds.begin() + j+1);
            cmds.erase(cmds.begin() + j);
        }
    }
}

void handle_redir(string file){
    pid_t pid;
    int pipefd[2];
    if(pipe(pipefd) < 0){
        perror("pipe error");
    }
    if((pid = fork()) < 0){
        perror("fork error");
        return;
    }
    if(pid == 0){
        close(pipefd[1]);
        int fd = open(file.c_str(), O_RDWR | O_TRUNC | O_CREAT);
        char buf;
        while((read(pipefd[0], &buf, 1)) > 0)
            write(fd, &buf, 1);

        chmod(file.c_str(), 0777);

        close(pipefd[0]);
        close(fd);

        exit(0);
    }
    else{
        dup2(pipefd[1], STDOUT_FILENO);

        close(pipefd[0]);
        close(pipefd[1]);
    }
}

void parse_cmd(vector<string> cmd){
    if (cmd[0] == "") {
        return;
    }

    vector<char *> args;
    for(size_t i=0; i<cmd.size(); i++){
        args.push_back(const_cast<char *>(cmd[i].c_str()));
    }
    args.push_back(nullptr);

    errno = 0;
    execvp(cmd[0].c_str(), args.data());
    if(errno != 0){
        fprintf(stderr, "Unknown command: [%s].\n", cmd[0].c_str());
        exit(0);
    }
}

int check_pipe_type(vector<vector<string> > cmds, string s, Pipe_Type &pt){
    int count = 0, index = 0;
    for(int i = s.length()-1; i>=0; i--){
        if(s[i] > '9' || s[i] < '0'){
            if(s[i] == '|') pt = num_pipe;
            else if(s[i] == '!') pt = num_pipe_err;
            else{
                pt = ordinary;
                return -1;
            }
            return count;
        }
        else{
            count += (s[i] - '0') * pow(10, index);
            index++;
        }
    }
    return -1;
}

void sub_num_pipe(vector<Num_Pipe_Info> &num_pipe_table){
    if(!num_pipe_table.empty()){
        vector<Num_Pipe_Info>::iterator it;
        for(it=num_pipe_table.begin(); it!=num_pipe_table.end(); it++){
            it->count--;
        }
    }
}

int check_num_pipe_table(int cnt, vector<Num_Pipe_Info> &num_pipe_table){
    for(size_t i=0; i<num_pipe_table.size(); i++){
        if(num_pipe_table[i].count == cnt){
            return i;
        }
    }
    return -1;
}

int creat_num_pipe(Pipe_Type pt, int cnt, vector<Num_Pipe_Info> &num_pipe_table){
    int index = check_num_pipe_table(cnt, num_pipe_table);
    if(index == -1){
        int pipefd[2];
        if(pipe(pipefd) < 0){
            perror("pipe error");
        }
        struct Num_Pipe_Info npi;
        npi.pipe_type = pt;
        npi.in_pipe = pipefd[0];
        npi.out_pipe = pipefd[1];
        npi.count = cnt;
        num_pipe_table.push_back(npi);
        return num_pipe_table.size() - 1;
    }
    return index;
}

void close_num_pipe(vector<Num_Pipe_Info> &num_pipe_table, bool is_child){
    vector<Num_Pipe_Info>::iterator it;
    for(it=num_pipe_table.begin(); it!=num_pipe_table.end(); it++){
        if(is_child){
            close(it->in_pipe);
            close(it->out_pipe);
        }
        else{
            if(it->count == 0){
                close(it->in_pipe);
                close(it->out_pipe);
            }
        }
    }
}

void handle_num_pipe(vector<Num_Pipe_Info> &num_pipe_table, Pipe_Type &pt, int i, vector<string> &cmds, int num, int &index){
    if(pt != 0 && i == (num - 1)){ // out
        cmds.pop_back();
        close(num_pipe_table[index].in_pipe);
        // cout << "redirect out " << index << '\n';
        if(pt == 2)
            dup2(num_pipe_table[index].out_pipe, STDERR_FILENO);
        dup2(num_pipe_table[index].out_pipe, STDOUT_FILENO);
        close(num_pipe_table[index].out_pipe);
    }
    for(size_t i=0; i<num_pipe_table.size(); i++){
        if(num_pipe_table[i].count == 0){
            close(num_pipe_table[i].out_pipe);
            // cout << "redirect in " << i << '\n';
            dup2(num_pipe_table[i].in_pipe, STDIN_FILENO);
            close(num_pipe_table[i].in_pipe);
        }
    }
}

void close_pipe_table(vector<Pipe_Info> &pipe_table, int i){
    close(pipe_table[i].in_pipe);
    close(pipe_table[i].out_pipe);
}

void child_process(const int &i, const int &num_cmds, vector<Pipe_Info> &pipe_table){
    if(i != 0)
        dup2(pipe_table[i-1].in_pipe, STDIN_FILENO);
    if(i != (num_cmds - 1)){
        dup2(pipe_table[i].out_pipe, STDOUT_FILENO);
        close_pipe_table(pipe_table, i);
    }
    if(i != 0){
        close_pipe_table(pipe_table, i-1);
    }
}

#endif	/* __NPSHELL_HPP__ */