#ifndef __HANDLE_USER_HPP__
#define __HANDLE_USER_HPP__

#include "multi_npshell.hpp"

void readBuf(int signo){
    int msgBuf_id = UserList[id_now].msgBuf_id;
    char *msgBuf = (char *) shmat(msgBuf_id, NULL, 0);
    string message(msgBuf);
    // memset(msgBuf, 0, sizeof(char) * BUFSIZE);
    cout << message << flush;
    shmdt(msgBuf);
    return;
}

void handle_user(int pid, struct sockaddr_in cli_addr){
    signal(SIGUSR1, readBuf);
    signal(SIGUSR2, readSignal);

    cout << wel_msg << flush;

    id_now = add_user(cli_addr); // add user to user_list and save user id
    // cout << id_now << "\n" << flush;
    my_lock(semid);
    broadcast_msg(login_msg(id_now));
    my_unlock(semid);

    multi_npshell(id_now);
}

#endif