#ifndef __CONSOLE_HPP__
#define __CONSOLE_HPP__

#include <boost/asio.hpp>
#include <fstream>
#include "utility.hpp"
using boost::asio::ip::tcp;

class ShellSession : public enable_shared_from_this<ShellSession> {
    public:
        ShellSession(boost::asio::io_context& io_context, int session_no)
          : socket_(io_context), resolver_(io_context) {
            session = "s" + to_string(session_no);
            client = clients[session_no];
            string filename = "./test_case/" + client.file;
            file.open(filename, ios::in);
        }
        void start() {
            connect2server();
        }
    private:
        void connect2server() {
            auto self = shared_from_this();
            tcp::resolver::query query(client.hostname, to_string(client.port));
            resolver_.async_resolve(query,
                                    [this, self](const boost::system::error_code& ec,
                                                 tcp::resolver::results_type results){
                if(!ec) {
                    boost::asio::async_connect(socket_, results, 
                                               [this, self](const boost::system::error_code& ec,
                                                            const tcp::endpoint& endpoint){
                        do_read();
                    });
                }
            });
        }
        void do_read() {
            auto self = shared_from_this();
            // cerr << "in_do_read\n" << flush;
            memset(data_, 0, max_length);
            socket_.async_read_some(boost::asio::buffer(data_, max_length),
                                    [this, self](boost::system::error_code ec, 
                                    size_t length) {
                // cerr << "in_async_read_some\n" << flush;
                if(!ec) {
                    // cerr << "in_if\n" << flush;
                    string msg = data_;
                    cerr << msg << flush;
                    html_replace(msg);
                    cout << "<script>document.getElementById('" << session << 
                            "').innerHTML+=\"" << msg << "\";</script>" << endl;
                    if(msg.find("% ") != string::npos){
                        do_write();
                    }
                    do_read();
                }
            });
        }
        void do_write() {
            auto self = shared_from_this();

            string cmd, html_cmd;
            getline(file, cmd);
            delete_carriage_return(cmd);
            cmd += "\n";
            html_cmd = cmd;
            cerr << cmd << flush;
            html_replace(html_cmd);
            cout << "<script>document.getElementById('" << session << 
                    "').innerHTML+= '<b>" << html_cmd << "</b>';</script>" << endl;
            socket_.async_send(boost::asio::buffer(cmd, cmd.length()),
                               [this, self](const boost::system::error_code& error,
                                            size_t length){
            });
            if(cmd.find("exit") != string::npos) {
                socket_.close();
                return;
            }
        }
    tcp::socket socket_;
    tcp::resolver resolver_;
    enum { max_length = 1024 };
    char data_[max_length];
    string session;
    fstream file;
    ClientInfo client;
};

#endif