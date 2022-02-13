#ifndef __MULTI_NPSHELL_HPP__
#define __MULTI_NPSHELL_HPP__

// #include "multi_function.hpp"
// #include "shared_mem.hpp"
#include "multi_user_pipe.hpp"

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

// s = "ls -l ok bye"
// ("ls -l ok bye")
// "ls"
// "-l"
// "ok"
// "bye"

vector<vector<string> > _split_cmd(string s) {
    vector<vector<string> > cmds;
    cmds.clear();

    string buf;
    stringstream ss(s);
    vector<string> cmd;
    while (ss >> buf) {
        if (buf == "|") {
            if (!cmd.empty()) {
                cmds.push_back(cmd);
                cmd.clear();
                continue;
            }
        }
        cmd.push_back(buf);
    }
    if (!cmd.empty()) {
        cmds.push_back(cmd);
        cmd.clear();
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

void parse_build_in(vector<string> cmd, int id){
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
        user_left(id);
        exit(0);
    }
}

bool build_in(vector<string> cmd, int id){
    if(cmd[0] == "setenv" || cmd[0] == "printenv" || cmd[0] == "exit"){
        parse_build_in(cmd, id);
        return true;
    }
    return false;
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
        name(id, strdup(cmd[1].c_str()));
        return 1;
    }
    return 0;
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

int check_pipe_type(vector<vector<string> > cmds, Pipe_Type &pt){
    int count = 0, index = 0;
    // 要拿最後一個cmds的最後一個指令判斷
    size_t n1 = cmds.size() - 1; // n1: 最後的cmds的最後一個cmd
    size_t n2 = cmds[n1].size() - 1; // n2: cmd的最後一個參數
    string cmd = cmds[n1][n2];
    // cout << "last: " << cmd << endl;
    for(int i = cmd.length()-1; i>=0; i--){
        if(cmd[i] > '9' || cmd[i] < '0'){
            if(cmd[i] == '|') pt = num_pipe;
            else if(cmd[i] == '!') pt = num_pipe_err;
            else{
                pt = ordinary;
                return -1;
            }
            return count;
        }
        else{
            count += (cmd[i] - '0') * pow(10, index);
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

void close_num_pipe(vector<Num_Pipe_Info> &num_pipe_table, int i){
    for(size_t j=0; j<num_pipe_table.size(); j++){
        if(num_pipe_table[j].count == 0 && i == 0){
            close(num_pipe_table[j].in_pipe);
            close(num_pipe_table[j].out_pipe);
            num_pipe_table.erase(num_pipe_table.begin() + j);
        }
    }
}

void handle_num_pipe(vector<Num_Pipe_Info> &num_pipe_table, Pipe_Type &pt, int i, vector<string> &cmds, int num, int &index){
    int zp = -1;
    for(size_t j=0; j<num_pipe_table.size(); j++){
        if(num_pipe_table[j].count == 0){
            close(num_pipe_table[j].out_pipe);
            zp = j;
        } else if (num_pipe_table[j].count > 0 && (int)j != index) {
            close(num_pipe_table[j].in_pipe);
            close(num_pipe_table[j].out_pipe);
        }
    }

    if (zp != -1 && i == 0) {
        dup2(num_pipe_table[zp].in_pipe, STDIN_FILENO);
        
        close(num_pipe_table[zp].in_pipe);
    }
    
    if(pt != 0 && i == (num - 1)){ // out
        cmds.pop_back();
        close(num_pipe_table[index].in_pipe);
        // cout << "redirect out " << index << '\n';
        if(pt == 2)
            dup2(num_pipe_table[index].out_pipe, STDERR_FILENO);
        dup2(num_pipe_table[index].out_pipe, STDOUT_FILENO);
        close(num_pipe_table[index].out_pipe);
    }
}

void close_pipe_table(vector<Pipe_Info> &pipe_table, int i){
    close(pipe_table[i].in_pipe);
    close(pipe_table[i].out_pipe);
}

void child_process(const int &i, const int &num_cmds, vector<Pipe_Info> &pipe_table){
    if(i != 0){
        close(pipe_table[i-1].out_pipe);
        dup2(pipe_table[i-1].in_pipe, STDIN_FILENO);
        close(pipe_table[i-1].in_pipe);
    }
    if(i != (num_cmds - 1)){
        close(pipe_table[i].in_pipe);
        dup2(pipe_table[i].out_pipe, STDOUT_FILENO);
        close(pipe_table[i].out_pipe);
    }
}

void multi_npshell(int id){
    init();
    vector<vector<string> > cmds;

    vector<Num_Pipe_Info> num_pipe_table;
    int num_pipe_index;

    while(1){
        printf("%% ");
        string s;
        getline(cin, s);
        if(cin.eof()) break;
        remove_string_tail(s);
        

        cmds = _split_cmd(s);
        int num_cmds = cmds.size();

        if(num_cmds == 0)
            continue;
        
        Pipe_Type pt;
        int cnt = check_pipe_type(cmds, pt);
        sub_num_pipe(num_pipe_table);

        pid_t pid;
        int pipefd[2];
        vector<Pipe_Info> pipe_table;
        string file = "";
        for(int i=0; i<num_cmds; i++){
            if(build_in(cmds[i], id)) break;

            if(build_in_2(cmds[i], id)) break;

            // user pipe
            // -1: not user pipe, -2: user pipe error, or return index in user_pipe_table
            int up_in ,up_out;
            tie(up_in, up_out) = user_pipe(id, cmds[i], s);
            // cout << "in up: " << up_in << " out up: " << up_out << '\n' << flush;
            
            if(i != (num_cmds - 1) && i != pre_cmd){ // 如果是最後一個cmd，就不用建pipe
                creat_pipe(pipefd, pipe_table);
            }

            preprocessing_redir(cmds[i], file);

            if(i == (num_cmds - 1) && pt != ordinary){ // numbered pipe
                num_pipe_index = creat_num_pipe(pt, cnt, num_pipe_table);
            }
            // if((pid = fork()) < 0){ // need to handel
            //     MaxFork = ForkCount;
            //     // perror("fork error");
            //     while(ForkCount >= MaxFork){}
            //     pre_cmd = i;
            //     i--;
            //     continue;
            // }
            // else ForkCount++;
            while((pid = fork()) < 0) {}
            if(pid == 0){ // child
                child_process(i, num_cmds, pipe_table);

                handle_num_pipe(num_pipe_table, pt, i, cmds[i], num_cmds, num_pipe_index);

                // handle user pipe
                handle_user_pipe(up_in, up_out, id);

                if(file != ""){ // redirection
                    handle_redir(file);
                }
                if (cmds.size() > 0) {
                    parse_cmd(cmds[i]);
                }
                exit(0);
            }
            else{ // parent
                if(i != 0)
                    close_pipe_table(pipe_table, i-1);

                close_num_pipe(num_pipe_table, i);
            }
        }
        if(pt == 0)
            waitpid(pid, NULL, 0); // wait for the last command
        ForkCount = 0;
        pre_cmd = -1;
    }
}

#endif	/* __NEW_NPSHELL_HPP__ */