#ifndef __SOCKS_SERVER_HPP__
#define __SOCKS_SERVER_HPP__

#include <iostream>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <utility>
#include <boost/asio.hpp>
#include <string>
#include <string.h>
#include "firewall.hpp"
using namespace std;
using boost::asio::ip::tcp;

boost::asio::io_context global_io_context;

class session : public enable_shared_from_this<session> {
    public:
        session(tcp::socket socket)
          : csocket_(move(socket)), ssocket_(global_io_context), resolver_(global_io_context), 
            acceptor_(global_io_context, tcp::endpoint(tcp::v4(), 0)) {}

        void start() {
            do_read();
        }
    private:
        void do_read() {
            auto self(shared_from_this());

            memset(data_, 0, max_length);
            csocket_.async_read_some(boost::asio::buffer(data_, max_length),
                                    [this, self](boost::system::error_code ec, 
                                    size_t length) {
                if(!ec) {
                    unsigned char VN = data_[0];
                    unsigned char CD = data_[1];
                    char tmp[max_length];
                    sprintf(tmp, "%u.%u.%u.%u", data_[4], data_[5], data_[6], data_[7]);
                    D_IP = string(tmp);
                    D_PORT = to_string(data_[2] << 8 | data_[3]);

                    if(D_IP[0] == '0') {
                        resolves_domain_name(length);
                    }

                    if(VN != 0x04 || firewall(CD, D_IP)) {
                        Reply = "Reject";
                        do_reject();
                        if(CD == 0x01) {
                            Command = "CONNECT";
                        }
                        else if(CD == 0x02) {
                            Command = "BIND";
                        }
                    } else {
                        Reply = "Accept";
                        if(CD == 0x01) {
                            Command = "CONNECT";
                            do_connect();
                        }
                        else if(CD == 0x02) {
                            // cout << "bind\n" << flush;
                            Command = "BIND";
                            do_bind();
                        }
                    }
                    print_msg(Command, Reply, 
                              csocket_.remote_endpoint().address().to_string(),
                              to_string(csocket_.remote_endpoint().port()),
                              D_IP, D_PORT);
                }
            });
        }
        void resolves_domain_name(size_t length) {
            unsigned char DOMAIN_NAME_tmp[max_length];
            int index = -1;
            for(int i=8; i<(int)length; i++) {
                if(!data_[i] && index == -1) {
                    index = 0;
                }
                else if(index >= 0) {
                    DOMAIN_NAME_tmp[index] = data_[i];
                    index ++;
                }
            }
            DOMAIN_NAME_tmp[index] = '\0';

            string DOMAIN_NAME = string((char *) DOMAIN_NAME_tmp);

            tcp::resolver::query query(DOMAIN_NAME, D_PORT);
            tcp::resolver::iterator iter = resolver_.resolve(query);
            tcp::resolver::iterator end;
            while(iter != end) {
                tcp::endpoint endpoint = *iter++;
                if(endpoint.address().is_v4()){
                    D_IP = endpoint.address().to_string();
                    break;
                }
            }
        }
        void print_msg(string Command, string Reply, string S_IP, string S_PORT, string D_IP, string D_PORT){
            cout << "--------------------------------\n" << flush;
            cout << "<S_IP>: " << S_IP << "\n";
            cout << "<S_PORT>: " << S_PORT << "\n";
            cout << "<D_IP>: " << D_IP << "\n";
            cout << "<D_PORT>: " << D_PORT << "\n";
            cout << "<Command>: " << Command << "\n";
            cout << "<Reply>: " << Reply << "\n";
        }
        void do_reject() {
            auto self(shared_from_this());

            reply[0] = 0;
            reply[1] = 0x5B;

            boost::asio::async_write(csocket_, boost::asio::buffer(reply, 8),
              [this, self](boost::system::error_code ec, size_t /*length*/) {

            });
        }
        void do_connect() {
            auto self(shared_from_this());

            tcp::endpoint endpoint(boost::asio::ip::address::from_string(D_IP), stoi(D_PORT));
            boost::system::error_code ec;
            // cout << "socket: " << ssocket_.native_handle() << "\n" << flush;
            ssocket_.connect(endpoint, ec);
            // cout << "socket: " << ssocket_.native_handle() << "\n" << flush;
            if(ec) {
                Reply = "Reject";
                do_reject();
                return;
            }
            
            // DSTPORT and DSTIP fields are ignored in CONNECT reply
            reply[0] = 0;
            reply[1] = 0x5A;

            boost::asio::async_write(csocket_, boost::asio::buffer(reply, 8),
              [this, self](boost::system::error_code ec, size_t /*length*/) {
                if(!ec) {
                    // tcp::endpoint endpoint(boost::asio::ip::address::from_string(D_IP), stoi(D_PORT));
                    // cout << "socket: " << ssocket_.native_handle() << "\n" << flush;
                    // ssocket_.connect(endpoint);
                    // cout << "socket: " << ssocket_.native_handle() << "\n" << flush;

                    read_from_server();
                    read_from_client();
                }
            });
        }
        void do_bind() {
            auto self(shared_from_this());

            // DSTPORT and DSTIP fields are meaningful in BIND reply
            reply[0] = 0;
            reply[1] = 0x5A;
            unsigned short port = acceptor_.local_endpoint().port();
            reply[2] = port >> 8;
            reply[3] = port & 0xFF;
            for(int i=4; i<8; i++) {
                reply[i] = 0;
            }

            boost::asio::async_write(csocket_, boost::asio::buffer(reply, 8),
              [this, self](boost::system::error_code ec, size_t /*length*/) {
                if(!ec) {
                    acceptor_.accept(ssocket_);

                    boost::asio::write(csocket_, boost::asio::buffer(reply, 8));

                    read_from_server();
                    read_from_client();
                }
            });
        }
        void read_from_server() {
            auto self(shared_from_this());
            
            memset(sdata_, 0, max_length);
            ssocket_.async_read_some(boost::asio::buffer(sdata_, max_length),
                                    [this, self](boost::system::error_code ec, 
                                    size_t length) {
                if(!ec) {
                    write_to_client(length);
                }
            });
        }
        void write_to_client(size_t length) {
            auto self(shared_from_this());
            boost::asio::async_write(csocket_, boost::asio::buffer(sdata_, length),
              [this, self](boost::system::error_code ec, size_t /*length*/) {
                if(!ec) {
                    read_from_server();
                }
            });
        }
        void read_from_client() {
            auto self(shared_from_this());

            memset(cdata_, 0, max_length);
            csocket_.async_read_some(boost::asio::buffer(cdata_, max_length),
                                    [this, self](boost::system::error_code ec, 
                                    size_t length) {
                if(!ec) {
                    write_to_server(length);
                }
            });
        }
        void write_to_server(size_t length) {
            auto self(shared_from_this());
            boost::asio::async_write(ssocket_, boost::asio::buffer(cdata_, length),
              [this, self](boost::system::error_code ec, size_t /*length*/) {
                if(!ec) {
                    read_from_client();
                }
            });
        }
    tcp::socket csocket_;
    tcp::socket ssocket_;
    tcp::resolver resolver_;
    tcp::acceptor acceptor_;
    enum { max_length = 1024 };
    unsigned char data_[max_length];
    char cdata_[max_length];
    char sdata_[max_length];
    char reply[8];
    string Reply;
    string Command;
    string D_IP;
    string D_PORT;
    // string DOMAIN_NAME;
};

class server {
    public:
        server(short port)
          : acceptor_(global_io_context, tcp::endpoint(tcp::v4(), port)),
                      csocket_(global_io_context) {
            do_accept();
        }
    private:
        void do_accept(){
            acceptor_.async_accept(csocket_, [this](boost::system::error_code ec) {
                if (!ec) {
                    global_io_context.notify_fork(boost::asio::io_context::fork_prepare);
                    if(fork() == 0) { // child
                        global_io_context.notify_fork(boost::asio::io_context::fork_child);
                        make_shared<session>(move(csocket_))->start();
                    } else { // parent
                        global_io_context.notify_fork(boost::asio::io_context::fork_parent);
                        csocket_.close();
                    }
                }
            do_accept();
            });
        }
    tcp::acceptor acceptor_;
    tcp::socket csocket_;
};

#endif