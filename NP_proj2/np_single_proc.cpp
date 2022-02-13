#include "client.hpp"
// #include "new_npshell.hpp"
// #include "function.hpp"

void np_socket_single(const char* argv){
    int port = atoi(argv);
    struct sockaddr_in serv_addr, cli_addr;
    int msock, ssock;

    if((msock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        printf("socket error\n");

    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(port);

    int reuse = 1;
    if(setsockopt(msock, SOL_SOCKET, SO_REUSEADDR, (char *) &reuse, sizeof(reuse)) < 0)
        perror("setsockopt(SO_REUSEADDR) failed");

    if(bind(msock, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
        printf("bind error\n");

    listen(msock, 30);

    int nfds = getdtablesize();
    FD_ZERO(&afds);
    FD_SET(msock, &afds);

    save_ori_env();

    while(1){
        if(FD_ISSET(msock, &rfds)){
            new_client_handle(msock, ssock, cli_addr);
        }

        memcpy(&rfds, &afds, sizeof(rfds));
        // cout << "select: " << ssock << endl;
        restart:
        if(select(nfds, &rfds, (fd_set *)0, (fd_set *)0, (struct timeval *)0) < 0){
            if (errno == EINTR) {
                goto restart;
            }
            perror("select");
            exit(0);
        }

        for(int i=0; i<nfds; i++){
            if(i != msock && FD_ISSET(i, &rfds)){
                // cout << "handle " << i << '\n' << flush;
                client_handle(i);
            }
        }
    }
    close(msock);
}

int main(int argc, char* argv[]){
    if(argc < 2){
        printf("Missing port.\n");
        return 0;
    }
    
    signal(SIGCHLD, sig_chld);

    np_socket_single(argv[1]);
    return 0;
}