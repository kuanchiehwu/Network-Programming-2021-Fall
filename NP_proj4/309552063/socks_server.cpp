#include <iostream>
#include "socks_server.hpp"
using namespace std;

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