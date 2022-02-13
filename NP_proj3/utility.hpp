#ifndef __UTILITY_HPP__
#define __UTILITY_HPP__

#include <iostream>
#include <string.h>
#include <string>
#include <vector>
#include <sstream>
#include <boost/asio.hpp>
using namespace std;

struct ClientInfo {
    string hostname;
    int port;
    string file;
};

vector<ClientInfo> clients;

string HTML_head = R"(
<!DOCTYPE html>
<html lang=\"en\">
    <head>
        <meta charset="UTF-8" />
        <title>NP Project 3 Console</title>
        <link
            rel="stylesheet"
            href="https://cdn.jsdelivr.net/npm/bootstrap@4.5.3/dist/css/bootstrap.min.css"
            integrity="sha384-TX8t27EcRE3e/ihU7zmQxVncDAy5uIKz4rEkgIXeMed4M0jlfIDPvg6uqKI2xXr2"
            crossorigin="anonymous"
        />
        <link
            href="https://fonts.googleapis.com/css?family=Source+Code+Pro"
            rel="stylesheet"
        />
        <link
            rel="icon"
            type="image/png"
            href="https://cdn0.iconfinder.com/data/icons/small-n-flat/24/678068-terminal-512.png"
        />
        <style>
            * {
                font-family: 'Source Code Pro', monospace;
                font-size: 1rem !important;
            }
            body {
                background-color: #212529;
            }
            pre {
                color: #cccccc;
            }
            b {
                color: #01b468;
            }
        </style>
    </head>
    <body>
        <table class="table table-dark table-bordered">
            <thead>
                <tr>
)";

string HTML_body = R"(
                </tr>
            </thead>
            <tbody>
                <tr>
)";

string HTML_foot = R"(
                </tr>
            </tbody>
        </table>
    </body>
</html>)";

vector<string> split_string(string s, char delim) {
    vector<string> ret;
    stringstream ss(s);
    string tmp;
    while(getline(ss, tmp, delim)) {
        ret.push_back(tmp);
	}
    return ret;
}

void InitHTML(){
    cout << HTML_head << flush;
    for(int i=0; i<(int)clients.size(); i++) {
        cout << "<th scope=\"col\">" << clients[i].hostname << ":" <<
                to_string(clients[i].port) << "</th>\n" << flush;
    }
    cout << HTML_body << flush;
    for(int i=0; i<(int)clients.size(); i++) {
        cout << "<td><pre id=\"s" << to_string(i) << "\" class=\"mb-0\"></pre></td>\n" << flush;
    }
    cout << HTML_foot << flush;
}

void html_replace(string &msg){
    vector<char> ori_msg = {'&', '\n', '<', '>', '\"', '\'', '\r'};
    vector<string> html_msg = {"&amps;", "&NewLine;", "&lt;", "&gt;", "&quot;", "&apos;", ""};

    for(int i=0; i<(int)ori_msg.size(); i++) {
        size_t pos = 0;
        while((pos = msg.find(ori_msg[i], pos)) != string::npos) {
            msg.replace(pos, 1, html_msg[i]);
            pos += html_msg[i].length();
        }
    }
}

void delete_carriage_return(string &s){
    char carriage_return = '\r';
    size_t pos = s.find(carriage_return);
    if(pos != string::npos) {
        s = s.substr(0, pos);
    }
}

#endif