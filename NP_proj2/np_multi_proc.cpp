#include "handle_user.hpp"

void np_socket_multi(const char* argv){
    int port = atoi(argv);
    struct sockaddr_in serv_addr, cli_addr;
    int msock, ssock;

    if((msock = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        perror("socket");
    }

    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(port);

    int reuse = 1;
    if(setsockopt(msock, SOL_SOCKET, SO_REUSEADDR, (char *) &reuse, sizeof(reuse)) < 0)
        perror("setsockopt(SO_REUSEADDR) failed");

    if(bind(msock, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0){
        perror("bind");
    }

    listen(msock, 30);

    create_shm();

    while(1){
        socklen_t clilen = sizeof(cli_addr);
        if((ssock = accept(msock, (struct sockaddr *) &cli_addr, &clilen)) < 0){
            if (errno == EINTR) {
                continue;
            }
            perror("accept");
            exit(0);
        }

        pid_t pid;
        if((pid = fork()) < 0)
            perror("fork");
        else if(pid == 0){ // child
            close(msock);
            dup2(ssock, STDIN_FILENO);
            dup2(ssock, STDOUT_FILENO);
            dup2(ssock, STDERR_FILENO);
            close(ssock);

            handle_user(pid, cli_addr);

            exit(0);
        }
        close(ssock);
    }
    close(msock);
}

void Nothing(int signo){
    // cout << "SIGUSR1\n" << flush;
    return;
}

int main(int argc, char* argv[]){
    if(argc < 2){
        printf("Missing port.\n");
        return 0;
    }
    
    signal(SIGCHLD, sig_chld);
    signal(SIGINT, remove_shm);
    signal(SIGUSR1, Nothing);


    np_socket_multi(argv[1]);

    return 0;
}