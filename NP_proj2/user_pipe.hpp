#ifndef __USER_PIPE_HPP__
#define __USER_PIPE_HPP__

#include "function.hpp"

struct user_pipe_info{
    int ID_sent;
    int ID_recv;
    int in_pipe;
    int out_pipe;
    bool active = true;
};

vector<user_pipe_info> user_pipe_table;

int get_id_n(string to_who){
    int id = 0;
    for(size_t i=1; i<to_who.length(); i++){
        id = id * 10 + (to_who[i] - '0');
    }
    return id;
}

bool ID_exist(int id, int check_id){
    for(size_t i=0; i<client_list.size(); i++){
        if(client_list[i].ID == check_id && client_list[i].active){
            return true;
        }
    }
    print_msg(client_list[id-1].fd, user_error_msg(check_id));
    return false;
}

int pipe_exist(int sent_id, int recv_id){
    for(size_t i=0; i<user_pipe_table.size(); i++){
        if(user_pipe_table[i].active){
            if(user_pipe_table[i].ID_sent == sent_id && user_pipe_table[i].ID_recv == recv_id){
                return i;
            }
        }
    }
    return -2;
}

int creat_user_pipe(int sent_id, int recv_id){
    int pipefd[2];
    if(pipe(pipefd) < 0){
        perror("pipe error");
    }

    user_pipe_info upi;
    upi.ID_sent = sent_id;
    upi.ID_recv = recv_id;
    upi.in_pipe = pipefd[0];
    upi.out_pipe = pipefd[1];
    upi.active = true;

    size_t i;
    for(i=0; i<user_pipe_table.size(); i++){
        if(user_pipe_table[i].ID_sent == sent_id && user_pipe_table[i].ID_recv == recv_id){
            user_pipe_table[i] = upi;
            return i;
        }
    }
    user_pipe_table.push_back(upi);
    return i;
}

void close_user_pipe(int id){
    for(size_t i=0; i<user_pipe_table.size(); i++){
        if((user_pipe_table[i].ID_sent == id || user_pipe_table[i].ID_recv == id) && user_pipe_table[i].active == true){
            close(user_pipe_table[i].in_pipe);
            close(user_pipe_table[i].out_pipe);
            user_pipe_table[i].active = false;
        }
    }
}

// <n
// check ID exist or not -> if no -> print error
// check pipe exist or not
// if yes -> broadcast message
// if no -> print error
int recv_msg(int recv_id, string from_who, string cmd){
    int sent_id = get_id_n(from_who);

    if(!ID_exist(recv_id, sent_id))
        return -2;

    int ret = pipe_exist(sent_id, recv_id);

    if(ret >= 0){
        broadcast_msg(recv_user_pipe_msg(sent_id, recv_id, cmd));
        return ret;
    }
    else{
        print_msg(client_list[recv_id-1].fd, user_pipe_not_exist_msg(sent_id, recv_id));
        return -2;
    }
    
}

// >n
// check ID exist or not -> if no -> print error
// check pipe exist or not
// if yes -> print error
// if no -> broadcast -> creat pipe to user_pipe_table
int send_msg(int sent_id, string to_who, string cmd){
    int recv_id = get_id_n(to_who);

    if(!ID_exist(sent_id, recv_id))
        return -2;

    int ret = pipe_exist(sent_id, recv_id);

    if(ret >= 0){
        print_msg(client_list[sent_id-1].fd, user_pipe_exist_msg(sent_id, recv_id));

        return -2;
    }
    else{
        broadcast_msg(sent_user_pipe_msg(sent_id, recv_id, cmd));
        // create pipe & return index
        return creat_user_pipe(sent_id, recv_id);
    }
}

// check >n and <n
// and get index in user_pipe_table
pair<int, int> user_pipe(int id, vector<string> &cmds, string s){
    int up_in = -1;
    int up_out = -1;
    for(size_t i=0; i<cmds.size(); i++){
        if(cmds[i].size() > 1 && cmds[i][0] == '<'){
            up_in = recv_msg(id, cmds[i], s);
            cmds.erase(cmds.begin() + i);
            break;
        }
    }
    for(size_t i=0; i<cmds.size(); i++){
        if(cmds[i].size() > 1 && cmds[i][0] == '>'){
            up_out = send_msg(id, cmds[i], s);
            cmds.erase(cmds.begin() + i);
            break;
        }
    }
    // cout << "index in: " << up_in << endl;
    // cout << "index out: " << up_out << endl;
    return make_pair(up_in, up_out);
}

void std_to_dev_null(int fd){
    int null_fd = open("/dev/null", O_RDWR);
    dup2(null_fd, fd);
    close(null_fd);
}

void close_all_user_pipe(){
    for(size_t i=0; i<user_pipe_table.size(); i++){
        if(user_pipe_table[i].active == true){
            close(user_pipe_table[i].in_pipe);
            close(user_pipe_table[i].out_pipe);
            user_pipe_table[i].active = false;
        }
    }
}

void handle_user_pipe(int up_in, int up_out){
    // <n
    if(up_in >= 0){
        if(user_pipe_table[up_in].active == true){
            close(user_pipe_table[up_in].out_pipe);
            dup2(user_pipe_table[up_in].in_pipe, STDIN_FILENO);
            close(user_pipe_table[up_in].in_pipe);
            user_pipe_table[up_in].active = false;
        }
    }
    else if(up_in == -2){
        std_to_dev_null(STDIN_FILENO);
    }
    // >n
    if(up_out >= 0){
        close(user_pipe_table[up_out].in_pipe);
        dup2(user_pipe_table[up_out].out_pipe, STDOUT_FILENO);
        close(user_pipe_table[up_out].out_pipe);
    }
    else if(up_out == -2){
        std_to_dev_null(STDOUT_FILENO);
    }
    
    close_all_user_pipe();
}

void close_recv_user_pipe(int up_in){
    if(user_pipe_table[up_in].active == true){
        close(user_pipe_table[up_in].in_pipe);
        close(user_pipe_table[up_in].out_pipe);
        user_pipe_table[up_in].active = false;
    }
}

#endif