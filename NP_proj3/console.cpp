#include <iostream>
#include <string.h>
#include <stdlib.h>
#include <vector>
#include "console.hpp"
using namespace std;

boost::asio::io_context global_io_context;

void get_client(string QUERY_STRING) {
    vector<string> parameters = split_string(QUERY_STRING, '&');
    for(int i=0; i<(int)parameters.size(); i+=3) {
        string hostname = parameters[i].substr(parameters[i].find("=") + 1);
        if(hostname != "") {
            ClientInfo c;
            c.hostname = hostname;
            c.port = stoi(parameters[i + 1].substr(parameters[i + 1].find("=") + 1));
            c.file = parameters[i + 2].substr(parameters[i + 2].find("=") + 1);
            // cout << c.hostname << "\n" << flush;
            // cout << c.port << "\n" << flush;
            // cout << c.file << "\n" << flush;
            clients.push_back(c);
        }
    }
}

int main(int argc, const char * argv[]) {
    cout << "Content-Type:text/html\n\n" << flush;
    string QUERY_STRING = getenv("QUERY_STRING");
    get_client(QUERY_STRING);
    InitHTML();
    for(int i=0; i<(int)clients.size(); i++) {
        make_shared<ShellSession>(global_io_context, i)->start();
    }
    global_io_context.run();
    return 0;
}