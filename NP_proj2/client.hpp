#ifndef __CLIENT_HPP__
#define __CLIENT_HPP__

#include "new_npshell.hpp"

// #define QLEN 30
// #define BUFSIZE 15000

void add_new_client(int fd, string IP, int port){
    client new_client;
    new_client.fd = fd;
    new_client.IP = IP;
    new_client.port = port;

    bool exist = false;
    for(size_t i=0; i<client_list.size(); i++){
        if(client_list[i].active == false){
            new_client.ID = i + 1;
            client_list[i] = new_client;
            exist = true;
            break;
        }
    }
    if(!exist){
        new_client.ID = client_list.size() + 1;
        client_list.push_back(new_client);
    }

    broadcast_msg(login_msg(new_client.NAME, new_client.IP, new_client.port));
}

void new_client_handle(int &msock, int &ssock, struct sockaddr_in &cli_addr){
    socklen_t clilen = sizeof(cli_addr);
    if((ssock = accept(msock, (struct sockaddr *) &cli_addr, &clilen)) < 0){
        perror("accept");
        return;
    }
    // cout << "after accept: " << ssock << endl;
    FD_SET(ssock, &afds);

    welcome_msg(ssock);

    add_new_client(ssock, inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
    
    prompt(ssock);
}

void close_connection(int id){
    client_list[id - 1].active = false;
    close(client_list[id - 1].fd);
    FD_CLR(client_list[id - 1].fd, &afds);

    broadcast_msg(logout_msg(client_list[id - 1].NAME));
}

void client_handle(int fd){
    int id;
    for(size_t i=0; i<client_list.size(); i++){
        if(client_list[i].fd == fd && client_list[i].active == true){
            id = client_list[i].ID;
            break;
        }
    }

    int ori_stdin = dup(STDIN_FILENO), ori_stdout = dup(STDOUT_FILENO), ori_stderr = dup(STDERR_FILENO);
    dup2(fd, STDIN_FILENO);
    dup2(fd, STDOUT_FILENO);
    dup2(fd, STDERR_FILENO);

    int res = npshell_single(id);
    
    if(res == 2){
        close_connection(id);
        close_user_pipe(id);
    }
    clearenv();
    prompt(fd);

    dup2(ori_stdin, STDIN_FILENO);
    dup2(ori_stdout, STDOUT_FILENO);
    dup2(ori_stderr, STDERR_FILENO);
    close(ori_stdin);
    close(ori_stdout);
    close(ori_stderr);
}

// void _client_handle(int fd) {
//     int id;
//     for(size_t i=0; i<client_list.size(); i++){
//         if(client_list[i].fd == fd && client_list[i].active == true){
//             id = client_list[i].ID;
//             break;
//         }
//     }

//     char buf[1024];
//     memset(buf, 0, sizeof(buf));
//     int len = read(fd, buf, sizeof(buf));
//     string cmd(buf);
//     cmd = cmd.substr(0, cmd.size() - 2);
//     if (cmd == "exit") {
//         close_connection(id);
//         close_user_pipe(id);
//     }
//     clearenv();
//     prompt(fd);
// }

#endif	/* __CLIENT_HPP__ */