#include "npshell.hpp"

int main(){
    init();

    signal(SIGCHLD, sig_chld);
    string s;
    vector<vector<string> > cmds;

    vector<Num_Pipe_Info> num_pipe_table;
    int num_pipe_index;

    while(1){
        printf("%% ");
        getline(cin, s);
        if(cin.eof()) break;

        cmds = split_cmd(s);
        int num_cmds = cmds.size();

        if(num_cmds == 0)
            continue;

        // for(int i=0; i<num_cmds; i++){
        //     for(size_t j=0; j<cmds[i].size(); j++){
        //         cout << cmds[i][j] << " ";
        //     }
        //     cout << "\n";
        // }
        
        Pipe_Type pt;
        int cnt = check_pipe_type(cmds, s, pt);
        sub_num_pipe(num_pipe_table);
        // cout << "pipe_type: " << pt << " cnt: " << cnt << endl;

        pid_t pid;
        int pipefd[2];
        vector<Pipe_Info> pipe_table;
        string file = "";
        for(int i=0; i<num_cmds; i++){
            if(build_in(cmds[i])) break;
            
            if(i != (num_cmds - 1) && i != pre_cmd){ // 如果是最後一個cmd，就不用建pipe
                creat_pipe(pipefd, pipe_table);
            }

            preprocessing_redir(cmds[i], file);

            if(i == (num_cmds - 1) && pt != ordinary){ // numbered pipe
                num_pipe_index = creat_num_pipe(pt, cnt, num_pipe_table);
            }
            if((pid = fork()) < 0){ // need to handel
                MaxFork = ForkCount;
                // perror("fork error");
                while(ForkCount >= MaxFork){}
                pre_cmd = i;
                i--;
                continue;
            }
            else ForkCount++;
            if(pid == 0){ // child
                child_process(i, num_cmds, pipe_table);

                handle_num_pipe(num_pipe_table, pt, i, cmds[i], num_cmds, num_pipe_index);
                close_num_pipe(num_pipe_table, 1);

                if(file != ""){ // redirection
                    handle_redir(file);
                }
                parse_cmd(cmds[i]);
                exit(0);
            }
            else{ // parent
                if(i != 0)
                    close_pipe_table(pipe_table, i-1);
                
                close_num_pipe(num_pipe_table, 0);
            }
        }
        if(pt == 0)
            waitpid(pid, NULL, 0); // wait for the last command
        ForkCount = 0;
        pre_cmd = -1;
    }
    printf("\n");
    
    return 0;
}
