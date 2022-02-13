#ifndef __MULTI_USER_PIPE_HPP__
#define __MULTI_USER_PIPE_HPP__

#include "shared_mem.hpp"

int get_id_n(string who){
    int id = 0;
    for(size_t i=1; i<who.length(); i++){
        id = id * 10 + (who[i] - '0');
    }
    return id;
}

bool ID_exist(int check_id){
    for(int i=0; i<MAXUSERS; i++){
        if(i == check_id && UserList[i].active){
            return true;
        }
    }
    cout << "*** Error: user #" << to_string(check_id + 1) << " does not exist yet. ***\n" << flush;
    return false;
}

int pipe_exist(int sent_id, int recv_id){
    for(int i=0; i<MAXUSERPIPE; i++){
        if(UserPipeTable[i].ID_sent == sent_id && UserPipeTable[i].ID_recv == recv_id && UserPipeTable[i].active == true){
            return i;
        }
    }
    return -2;
}

int create_user_pipe(int sent_id, int recv_id){
    int pipefd[2];
    if(pipe(pipefd) < 0){
        perror("pipe error");
    }

    UserPipeInfo up;
    up.ID_sent = sent_id;
    up.ID_recv = recv_id;
    up.in_pipe = pipefd[0];
    up.out_pipe = pipefd[1];
    up.active = true;
    string s = "./user_pipe/" + to_string(sent_id) + "_" + to_string(recv_id);
    // strcpy(up.fifo, strdup(s.c_str()));

    if ( (mknod(strdup(s.c_str()), S_IFIFO | 0666, 0) < 0) && (errno != EEXIST)){
        cout << s << "\n" << flush;
        perror("mknod");
    }

    for(int i=0; i<MAXUSERPIPE; i++){
        if(UserPipeTable[i].active == false){
            UserPipeTable[i] = up;
            return i;
        }
    }

    return -1;
}

int recv_msg(int recv_id, string from_who, string cmd){
    my_lock(semid);
    int sent_id = get_id_n(from_who) - 1;

    if(!ID_exist(sent_id)){
        my_unlock(semid);
        return -2;
    }

    int ret = pipe_exist(sent_id, recv_id);

    if(ret >= 0){
        // cout << "recv broadcast: " << recv_id+1 << "<-" << sent_id+1 << "\n" << flush;
        broadcast_msg(recv_user_pipe_msg(sent_id, recv_id, cmd));
        my_unlock(semid);
        return ret;
    }
    else{
        cout << "*** Error: the pipe #" << to_string(sent_id + 1) << "->#" 
             << to_string(recv_id + 1) << " does not exist yet. ***\n" << flush;
        my_unlock(semid);
        return -2;
    }
    my_unlock(semid);
}

int send_msg(int sent_id, string to_who, string cmd){
    my_lock(semid);
    int recv_id = get_id_n(to_who) - 1;

    if(!ID_exist(recv_id)){
        my_unlock(semid);
        return -2;
    }

    int ret = pipe_exist(sent_id, recv_id);

    if(ret >= 0){
        cout << "*** Error: the pipe #" << to_string(sent_id + 1) << "->#"
             << to_string(recv_id + 1) << " already exists. ***\n" << flush;
        my_unlock(semid);
        return -2;
    }
    else{
        // broadcast_msg(sent_user_pipe_msg(sent_id, recv_id, cmd));
        // create pipe & return index
        int index = create_user_pipe(sent_id, recv_id);
        broadcast_msg(sent_user_pipe_msg(sent_id, recv_id, cmd));
        my_unlock(semid);
        return index;
    }
    my_unlock(semid);
}

pair<int, int> user_pipe(int id, vector<string> &cmds, string s){
    int up_in = -1;
    int up_out = -1;
    // my_lock(semid);
    for(size_t i=0; i<cmds.size(); i++){
        if(cmds[i].size() > 1 && cmds[i][0] == '<'){
            up_in = recv_msg(id, cmds[i], s);
            cmds.erase(cmds.begin() + i);
            break;
        }
    }
    // my_unlock(semid);
    // my_lock(semid);
    for(size_t i=0; i<cmds.size(); i++){
        if(cmds[i].size() > 1 && cmds[i][0] == '>'){
            up_out = send_msg(id, cmds[i], s);
            cmds.erase(cmds.begin() + i);
            break;
        }
    }
    // my_unlock(semid);
    return make_pair(up_in, up_out);
}

void std_to_dev_null(int fd){
    int null_fd = open("/dev/null", O_RDWR);
    dup2(null_fd, fd);
    close(null_fd);
}

void close_sent_user_pipe(int id){
    for(int i=0; i<MAXUSERPIPE; i++){
        if(UserPipeTable[i].active){
            if(UserPipeTable[i].ID_sent == id){
                close(UserPipeTable[i].in_pipe);
                close(UserPipeTable[i].out_pipe);
            }
        }
    }
}

void writeFIFO(int signo){
    int msgBuf_id = UserList[id_now].msgBuf_id;
    char *msgBuf = (char *) shmat(msgBuf_id, NULL, 0);
    int index = atoi(msgBuf);
    // memset(msgBuf, 0, sizeof(char) * BUFSIZE);
    shmdt(msgBuf);

    close(UserPipeTable[index].out_pipe);
    char buf;
    string con = "";
    while(read(UserPipeTable[index].in_pipe, &buf, 1) > 0)
        con += buf;
    close(UserPipeTable[index].in_pipe);

    string s = "./user_pipe/" + to_string(UserPipeTable[index].ID_sent) + 
               "_" + to_string(UserPipeTable[index].ID_recv);

    if((UserPipeTable[index].write_fd = open(s.c_str(), 1)) < 0){
        perror("open write");
    }
    write(UserPipeTable[index].write_fd, con.c_str(), con.size());

    close(UserPipeTable[index].write_fd);
}

void readSignal(int signo) {
    int msgBuf_id = UserList[id_now].msgBuf_id;
    char *msgBuf = (char *) shmat(msgBuf_id, NULL, 0);
    int index = atoi(msgBuf);
    shmdt(msgBuf);

    cout << index << '\n';

    string s = "./user_pipe/" + to_string(UserPipeTable[index].ID_sent) + 
               "_" + to_string(UserPipeTable[index].ID_recv);
    UserPipeTable[index].read_fd = open(s.c_str(), O_RDONLY);
}

void readFIFO(int index){
    string s = "./user_pipe/" + to_string(UserPipeTable[index].ID_sent) + 
               "_" + to_string(UserPipeTable[index].ID_recv);

    if((UserPipeTable[index].read_fd = open(s.c_str(), 0)) < 0){
        // cout << UserPipeTable[index].fifo << "\n" << flush;
        perror("open read");
    }

    dup2(UserPipeTable[index].read_fd, STDIN_FILENO);

    close(UserPipeTable[index].read_fd);
    if (unlink(s.c_str()) < 0){
        perror("unlink");
    }
}

void handle_user_pipe(int up_in, int up_out, int id){
    // <n
    my_lock(semid);
    if(up_in >= 0){
        // strcpy(msgBuffer, int2char(up_in));
        // int msgBuf_id = UserList[UserPipeTable[up_in].ID_sent].msgBuf_id;
        // char *msgBuf = (char *) shmat(msgBuf_id, NULL, 0);
        // strcpy(msgBuf, int2char(up_in));
        // shmdt(msgBuf);
        
        // kill(UserList[UserPipeTable[up_in].ID_sent].pid, SIGUSR2);
        // readFIFO(up_in);
        // UserPipeTable[up_in].active = false;
        int read_fd = UserPipeTable[up_in].read_fd;
        dup2(read_fd, 0);
        close(read_fd);
        UserPipeTable[up_in].active = false;
        string s = "./user_pipe/" + to_string(UserPipeTable[up_in].ID_sent) + 
               "_" + to_string(UserPipeTable[up_in].ID_recv);
        if (unlink(s.c_str()) < 0){
            perror("unlink");
        }
    }
    else if(up_in == -2){
        std_to_dev_null(STDIN_FILENO);
    }
    // >n
    if(up_out >= 0){
        close(UserPipeTable[up_out].in_pipe);
        dup2(UserPipeTable[up_out].out_pipe, STDOUT_FILENO);
        close(UserPipeTable[up_out].out_pipe);
        // int recv_id = UserPipeTable[up_out].ID_recv;
        // int msgBuf_id = UserList[recv_id].msgBuf_id;
        // char *msgBuf = (char *) shmat(msgBuf_id, NULL, 0);
        // strcpy(msgBuf, int2char(up_out));
        // shmdt(msgBuf);

        // kill(UserList[recv_id].pid, SIGUSR2);

        // string s = "./user_pipe/" + to_string(UserPipeTable[up_out].ID_sent) + 
        //        "_" + to_string(UserPipeTable[up_out].ID_recv);
        // UserPipeTable[up_out].write_fd = open(s.c_str(), O_RDONLY);
        // int write_fd = UserPipeTable[up_out].write_fd;
        // dup2(write_fd, 1);
        // close(write_fd);
    }
    else if(up_out == -2){
        std_to_dev_null(STDOUT_FILENO);
    }
    
    // close_sent_user_pipe(id); //?????

    for(int i=0; i<MAXUSERPIPE; i++){
        if(UserPipeTable[i].active){
            if(UserPipeTable[i].ID_recv == id){
                close(UserPipeTable[i].read_fd);
            }
        }
    }
    
    my_unlock(semid);
}

#endif