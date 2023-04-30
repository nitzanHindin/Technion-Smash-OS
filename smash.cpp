#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include "Commands.h"
#include "signals.h"

int main(int argc, char* argv[]) {

    struct sigaction sa;
    sa.sa_sigaction = *alarmHandler;
    sa.sa_flags = SA_SIGINFO | SA_RESTART;
    if(sigemptyset(&sa.sa_mask) == ERROR)
    {
        perror("smash error: sigemptyset failed");
    }
    if(sigaddset(&sa.sa_mask, SIGALRM) == ERROR)
    {
        perror("smash error: sigaddset failed");
    }

    if(sigaction(SIGALRM, &sa, NULL)==ERROR) {
        perror("smash error: failed to set alarm handler");
    }


    if(signal(SIGTSTP , ctrlZHandler)==SIG_ERR) {
        perror("smash error: failed to set ctrl-Z handler");
    }
    if(signal(SIGINT , ctrlCHandler)==SIG_ERR) {
        perror("smash error: failed to set ctrl-C handler");
    }
    
    //TODO: setup sig alarm handler

    SmallShell& smash = SmallShell::getInstance();
    while(true) {
        SmallShell::getInstance().fg_job = std::make_shared<JobsList::JobEntry>(-1, false, "", -1, "");
        std::cout << smash.smash_name << "> ";
        std::string cmd_line;
        std::getline(std::cin, cmd_line);
        if(cmd_line == "")
            continue;
        smash.executeCommand(cmd_line);
    }
    return 0;
}