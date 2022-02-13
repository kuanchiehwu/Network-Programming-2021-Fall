#ifndef __FIREWALL_HPP__
#define __FIREWALL_HPP__

#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <regex>
using namespace std;

bool firewall(unsigned char CD, string D_IP) {
    ifstream f;
    f.open("socks.conf");
    if(!f) {
        // cout << "open socks.conf fail\n" << flush;
        return true;
    }

    string s;
    while(getline(f, s)) {
        // cout << s << flush;
        stringstream ss(s);
        string tmp, operation, ip_addr;
        ss >> tmp >> operation >> ip_addr;
        if((operation == "c" && CD == 0x01) || (operation == "b" && CD == 0x02)) {
            // check D_IP
            string regex_ip = "";
            for(int i=0; i<(int)ip_addr.size(); i++) {
                if(ip_addr[i] == '*') {
                    regex_ip += "[0-9]+";
                } else {
                    regex_ip += ip_addr[i];
                }
            }
            // cout << regex_ip << "\n" << flush;
            regex reg(regex_ip);
            if(regex_match(D_IP, reg)) {
                f.close();
                return false;
            }
        }
    }

    f.close();
    return true;
}

#endif