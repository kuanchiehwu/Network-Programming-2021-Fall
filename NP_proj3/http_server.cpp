#include <iostream>
#include <cstdlib>
#include <memory>
#include <utility>
#include <boost/asio.hpp>
#include <string>
#include <stdlib.h>
#include <unistd.h>
using namespace std;
using boost::asio::ip::tcp;

boost::asio::io_context global_io_context;

class session : public enable_shared_from_this<session> {
    public:
        session(tcp::socket socket)
          : socket_(move(socket)) {}

        void start() {
            do_read();
        }
    private:
        void do_read() {
            auto self(shared_from_this());
            socket_.async_read_some(boost::asio::buffer(data_, max_length),
                                    [this, self](boost::system::error_code ec, 
                                    size_t length) {
                /*
                http://nplinux4.cs.nctu.edu.tw:52101/panel.cgi?a=b&c=d in data_:
                GET /panel.cgi?a=b&c=d HTTP/1.1
                Host: nplinux4.cs.nctu.edu.tw:52101
                */
                // data_ : HTTP 協定的標頭檔
                sscanf(data_, "%s %s %s %s %s", REQUEST_METHOD, REQUEST_URI, 
                      SERVER_PROTOCOL, c, HTTP_HOST);
                if(!ec) {
                    do_write(length);
                }
            });
        }
        void do_write(size_t length) {
            auto self(shared_from_this());
            boost::asio::async_write(socket_, 
                                     boost::asio::buffer(status_str, strlen(status_str)),
                                     [this, self](boost::system::error_code ec, 
                                     size_t /*length*/) {
                    if(!ec) {
                        strcpy(SERVER_ADDR,
                               socket_.local_endpoint().address().to_string().c_str());
                        sprintf(SERVER_PORT, "%u", socket_.local_endpoint().port());
                        strcpy(REMOTE_ADDR,
                               socket_.remote_endpoint().address().to_string().c_str());
                        sprintf(REMOTE_PORT, "%u", socket_.remote_endpoint().port());

                        /* REQUEST_URI = /panel.cgi?a=b&c=d */
                        bool has_query_string = false;
                        int j = 0;
                        for(int i=0; i<max_length; i++) {
                            if(REQUEST_URI[i] == '\0')
                                break;
                            else if(REQUEST_URI[i] == '?') {
                                has_query_string = true;
                                continue;
                            }
                            
                            if(has_query_string) {
                                QUERY_STRING[j] = REQUEST_URI[i];
                                j++;
                            }
                        }
                        for(int i=0; i<max_length; i++) {
                            if(REQUEST_URI[i] == '\0' || REQUEST_URI[i] == '?')
                                break;
                            exec_file[i+1] = REQUEST_URI[i];
                        }
                        handle_cgi();

                        do_read();
                }
            });
        }
        void set_cgi_env(){
            setenv("REQUEST_METHOD", REQUEST_METHOD, 1);
            setenv("REQUEST_URI", REQUEST_URI, 1);
            setenv("QUERY_STRING", QUERY_STRING, 1);
            setenv("SERVER_PROTOCOL", SERVER_PROTOCOL, 1);
            setenv("HTTP_HOST", HTTP_HOST, 1);
            setenv("SERVER_ADDR", SERVER_ADDR, 1);
            setenv("SERVER_PORT", SERVER_PORT, 1);
            setenv("REMOTE_ADDR", REMOTE_ADDR, 1);
            setenv("REMOTE_PORT", REMOTE_PORT, 1);
        }
        void handle_cgi(){
            global_io_context.notify_fork(boost::asio::io_context::fork_prepare);
            if(fork() == 0) { // child
                global_io_context.notify_fork(boost::asio::io_context::fork_child);
                set_cgi_env();
                int sock = socket_.native_handle();
                cout << "socket fd: " << sock << "\n" << flush;
                dup2(sock, STDIN_FILENO);
                dup2(sock, STDOUT_FILENO);
                // dup2(sock, STDERR_FILENO);
                socket_.close();

                execlp(exec_file, exec_file, NULL);
            } else { // parent
                global_io_context.notify_fork(boost::asio::io_context::fork_parent);
                socket_.close();
            }
        }
    tcp::socket socket_;
    enum { max_length = 1024 };
    char data_[max_length];
    char status_str[max_length] = "HTTP/1.1 200 OK\n";
    char exec_file[max_length] = "./";
    char c[max_length];
    char REQUEST_METHOD[max_length];
    char REQUEST_URI[max_length];
    char QUERY_STRING[max_length];
    char SERVER_PROTOCOL[max_length];
    char HTTP_HOST[max_length];
    char SERVER_ADDR[max_length];
    char SERVER_PORT[max_length];
    char REMOTE_ADDR[max_length];
    char REMOTE_PORT[max_length];
};

class server {
    public:
        server(short port)
          : acceptor_(global_io_context, tcp::endpoint(tcp::v4(), port)),
                      socket_(global_io_context) {
            do_accept();
        }
    private:
        void do_accept(){
            acceptor_.async_accept(socket_, [this](boost::system::error_code ec) {
                if (!ec) {
                    make_shared<session>(move(socket_))->start();
                }
            do_accept();
            });
        }
    tcp::acceptor acceptor_;
    tcp::socket socket_;
};

int main(int argc, char* argv[]){
    try {
        if(argc != 2) {
            cerr << "Usage: " << argv[0] << " <port>\n";
            return 1;
        }

        server s(atoi(argv[1]));

        global_io_context.run();
    } catch(exception &e) {
        cerr << "Exception: " << e.what() << "\n";
    }
    return 0;
}