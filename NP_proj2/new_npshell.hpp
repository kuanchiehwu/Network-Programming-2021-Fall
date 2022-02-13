#ifndef __NEW_NPSHELL_HPP__
#define __NEW_NPSHELL_HPP__

// #include "function.hpp"
#include "user_pipe.hpp"

int ForkCount = 0;
int MaxFork;
int pre_cmd = -1;

struct Pipe_Info{
    int in_pipe;
    int out_pipe;
};

void init(){
    // set to ori_env
    for(size_t i=0; i<ori_env.size(); i++)
        setenv(ori_env[i].first.c_str(), ori_env[i].second.c_str(), 1);

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
        fflush(stdout);
        env = *(environ + i);
    }
}

int parse_build_in(vector<string> cmd, int id){
    if(cmd[0] == "setenv"){
        // save (cmd[1], default) to vector env
        save_env_change(id, cmd[1], cmd[2]);
        // char* tmp = getenv(cmd[1].c_str());
        // if(tmp != NULL)
        //     save_env_change(id, cmd[1], tmp);
        // else
        //     save_env_change(id, cmd[1], "");

        setenv(cmd[1].c_str(), cmd[2].c_str(), 1);
    }
    else if(cmd[0] == "printenv"){
        if(cmd.size() == 1)
            printenv_all();
        else{
            char* env = getenv(cmd[1].c_str());
            if(env != NULL) {
                printf("%s\n", env);
                fflush(stdout);
            }
        }
    }
    else if(cmd[0] == "exit"){
        return 2;
    }
    return 1;
}

int build_in(vector<string> cmd, int id){
    if(cmd[0] == "setenv" || cmd[0] == "printenv" || cmd[0] == "exit"){
        return parse_build_in(cmd, id);
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
    int fd = open(file.c_str(), O_RDWR | O_TRUNC | O_CREAT);
    if(fd < 0){
        perror("open");
        return;
    }
    chmod(file.c_str(), 0777);

    dup2(fd, STDOUT_FILENO);
    close(fd);

    // pid_t pid;
    // int pipefd[2];
    // if(pipe(pipefd) < 0){
    //     perror("pipe error");
    // }
    // if((pid = fork()) < 0){
    //     perror("fork error");
    //     return;
    // }
    // if(pid == 0){
    //     close(pipefd[1]);
    //     int fd = open(file.c_str(), O_RDWR | O_TRUNC | O_CREAT);
    //     char buf;
    //     while((read(pipefd[0], &buf, 1)) > 0)
    //         write(fd, &buf, 1);

    //     chmod(file.c_str(), 0777);

    //     close(pipefd[0]);
    //     close(fd);

    //     exit(0);
    // }
    // else{
    //     dup2(pipefd[1], STDOUT_FILENO);

    //     close(pipefd[0]);
    //     close(pipefd[1]);
    // }
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
    // if(zp != -1) num_pipe_table.erase(num_pipe_table.begin() + zp);
}

void close_pipe_table(vector<Pipe_Info> &pipe_table, int i){
    close(pipe_table[i].in_pipe);
    close(pipe_table[i].out_pipe);
}

void pipe_process(const int &i, const int &num_cmds, vector<Pipe_Info> &pipe_table){
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

int npshell_single(int client_id){
    init();
    // evn <- cilent.env
    exchange_env(client_id);

    // num_pipe_table <- client.num_pipe_table
    vector<Num_Pipe_Info> num_pipe_table = client_list[client_id - 1].num_pipe_table;
    int num_pipe_index;

    string s;
    getline(cin, s);
    // remove /r/n
    remove_string_tail(s);

    vector<vector<string> > cmds;
    cmds = _split_cmd(s);
    int num_cmds = cmds.size();

    if(num_cmds != 0){
        Pipe_Type pt;
        int cnt = check_pipe_type(cmds, pt);
        sub_num_pipe(num_pipe_table);

        pid_t pid;
        int pipefd[2];
        vector<Pipe_Info> pipe_table;
        string file = "";
        for(int i=0; i<num_cmds; i++){
            int res = build_in(cmds[i], client_id);
            if(res > 0){
                return res;
            }

            if(build_in_2(cmds[i], client_id) == 1){
                return 1;
            }

            // user pipe
            // -1: not user pipe, -2: user pipe error, or return index in user_pipe_table
            int up_in ,up_out;
            tie(up_in, up_out) = user_pipe(client_id, cmds[i], s);
            // cout << "index: " << up_in << ", " << up_out << endl;
            
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
                pipe_process(i, num_cmds, pipe_table);

                // handle user pipe
                handle_user_pipe(up_in, up_out);

                handle_num_pipe(num_pipe_table, pt, i, cmds[i], num_cmds, num_pipe_index);
                
                if(file != ""){ // redirection
                    handle_redir(file);
                }
                if (cmds.size() > 0) {
                    parse_cmd(cmds[i]);
                }
                exit(0);
            }
            else{ // parent
                if(i != 0){
                    close_pipe_table(pipe_table, i-1);
                }
                
                // close user pipe
                if(up_in >= 0){
                    close_recv_user_pipe(up_in);
                }

                close_num_pipe(num_pipe_table, i);
                // num_pipe_table -> client.num_pipe_table
                client_list[client_id - 1].num_pipe_table = num_pipe_table;

            }
        }
        if(pt == 0)
            waitpid(pid, NULL, 0); // wait for the last command
        ForkCount = 0;
        pre_cmd = -1;
    }
    return 0;
}

#endif	/* __NEW_NPSHELL_HPP__ */