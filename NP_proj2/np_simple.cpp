#include "npshell.hpp"

void np_socket(const char* argv){
    int port = atoi(argv);
    struct sockaddr_in serv_addr, cli_addr;
    int sockfd, newsockfd;

    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        printf("socket error\n");

    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(port);

    int reuse = 1;
    if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char *) &reuse, sizeof(reuse)) < 0)
        perror("setsockopt(SO_REUSEADDR) failed");

    if(bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
        printf("bind error\n");

    listen(sockfd, 10);

    while(1){
        socklen_t clilen = sizeof(cli_addr);
        if((newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen)) < 0)
            printf("accept error\n");
        
        pid_t pid;
        if((pid = fork()) < 0)
            printf("fork error\n");
        else if(pid == 0){
            close(sockfd);
            dup2(newsockfd, STDIN_FILENO);
            dup2(newsockfd, STDOUT_FILENO);
            dup2(newsockfd, STDERR_FILENO);
            close(newsockfd);
            npshell();
            close(newsockfd);
            exit(0);
        }
        close(newsockfd);
    }
    close(sockfd);
}

int main(int argc, char* argv[]){
    // return 0;
    if(argc < 2){
        printf("Missing port.\n");
        return 0;
    }
    
    signal(SIGCHLD, sig_chld);

    // npshell();
    np_socket(argv[1]);
    return 0;
}
