#include <iostream>
#include <signal.h>
#include "signals.h"
#include "Commands.h"

using namespace std;


void ctrlZHandler(int sig_num) {
	// TODO: Add your implementation
  std::cout << "smash: got ctrl-Z" << std::endl;
  SmallShell& smash = SmallShell::getInstance();

  smash.list.removeFinishedJobs();

  JobsList::JobEntry* job = smash.fg_job.get();
  if(job->jobID == ERROR){
    return;
  }

  pid_t pid = job->pid;

  if(kill(pid, SIGSTOP) == ERROR)
  {
    perror("smash error: kill failed");
    return;
  }

  smash.list.addJob(job->cmd_txt, pid, job->cmd_org, true);

  std::cout << "smash: process " << pid << " was stopped" << std::endl;
}

void ctrlCHandler(int sig_num) {
  // TODO: Add your implementation
  std::cout << "smash: got ctrl-C" << std::endl;
  SmallShell& smash = SmallShell::getInstance();

  smash.list.removeFinishedJobs();

  if(smash.fg_job.get()->jobID == ERROR){
    return;
  }
  pid_t pid = smash.fg_job.get()->pid;
  if(kill(pid, SIGKILL) == ERROR)
  {
    perror("smash error: kill failed");
    return;
  }
  std::cout << "smash: process " << pid << " was killed" << std::endl;
}

/*struct sigaction alarmHandler(int sig_num) {
  // TODO: Add your implementation
  std::cout << "smash: got an alarm" << std::endl;
}*/

void alarmHandler(int sig_num, siginfo_t *siginfo, void *context) {
  pid_t sender_pid = siginfo->si_pid;
  std::cout << "smash: got an alarm" << std::endl;
  //std::cout << "PID: " << sender_pid << std::endl;

  string cmd_txt = "";

  SmallShell& smash = SmallShell::getInstance();

  if(sender_pid != 0){ ///// child sent signal
    if(smash.fg_job.get()->pid == sender_pid)
      cmd_txt = smash.fg_job.get()->cmd_org;
    else
    {
      for(std::list<std::shared_ptr<JobsList::JobEntry>>::iterator it = smash.list.jobs_list.begin(); it != smash.list.jobs_list.end(); ++it)
      {
        if(it->get()->pid == sender_pid)
        {
          cmd_txt = it->get()->cmd_org;
          break;
        }
      }
    }
    if(kill(sender_pid, SIGKILL) == ERROR)
    {
      perror("smash error: kill failed");
      return;
    }
    std::cout << "smash: " << cmd_txt <<" timed out!" << std::endl;

    //smash.list.removeFinishedJobs();
  }

  else{ //father sent signal

    SmallShell::getInstance().list.removeFinishedJobs();

    std::list<std::shared_ptr<JobsList::JobEntry>>::iterator it = smash.timeout_list.jobs_list.begin();
    pid_t pid_to_kill = it->get()->pid;

    if(!it->get()->finished)
    {
      if(kill(pid_to_kill, SIGKILL) == ERROR)
      {
        perror("smash error: kill failed");
        return;
      }
      cmd_txt = it->get()->cmd_org;
      std::cout << "smash: " << cmd_txt <<" timed out!" << std::endl;
    }

    SmallShell::getInstance().timeout_list.jobs_list.erase(it);
    it = smash.timeout_list.jobs_list.begin();

    time_t current_time;

    if(it != smash.timeout_list.jobs_list.end())
    {
      if(time(&current_time) == ERROR)
      {
        perror("smash error: time failed");
        return;
      }

      time_t time_comp = it->get()->created + it->get()->duration - current_time;
      alarm(time_comp);
    }

  }
}