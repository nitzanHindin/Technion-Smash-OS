#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_

#include <vector>
#include <string>
#include <unistd.h>
#include <list>
#include <time.h>
#include <cstring> // ?????
#include <signal.h>
#include <typeinfo>
#include <memory>
#include <fcntl.h>
#include <limits.h>

#define ERROR -1
#define NEW_JOB -2

#define COMMAND_ARGS_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (20)

using namespace std;

class Command {
// TODO: Add your data members
 public:
  Command(string cmd_line, bool bg, string cmd_org);
  virtual ~Command();
  virtual void execute() = 0;
  //virtual void prepare();
  //virtual void cleanup();
  // TODO: Add your extra methods if needed
  std::string cmd;
  char* args[21];
  bool bg;
  std::string cmd_org;
  bool timeout;
};

class BuiltInCommand : public Command {
 public:
  BuiltInCommand(string cmd_line, bool bg, string cmd_org);
  virtual ~BuiltInCommand() {}
};

class chpromptCommand : public BuiltInCommand {
 public:
 std::string saved_name;

  chpromptCommand(string cmd_line, bool bg, string cmd_org);
  //chpromptCommand();
  ~chpromptCommand() {}
  void execute() override;
};

class ExternalCommand : public Command {
 public:
  ExternalCommand(string cmd_line, bool bg, string cmd_org, int duration);
  virtual ~ExternalCommand() {}
  void execute() override;
  int duration;
  bool complex;
  string cmd_txt;
};

class PipeCommand : public Command {
  // TODO: Add your data members
 public:
  PipeCommand(string cmd_line, bool bg, string cmd_org);
  virtual ~PipeCommand() {}
  void execute() override;

  long unsigned int pipe_index;
  bool not_a_pipe_command;
  bool stderr_flag;
  long unsigned int background_index;
};

class RedirectionCommand : public Command {
 // TODO: Add your data members
 public:
  explicit RedirectionCommand(string cmd_line, bool bg, string cmd_org);
  virtual ~RedirectionCommand() {}
  void execute() override;
  //void prepare() override;
  //void cleanup() override;

  long unsigned int redirect_index;
  bool not_a_redirection;
  long unsigned int second_redirect_index;
};

class ChangeDirCommand : public BuiltInCommand {
// TODO: Add your data members
public:
  ChangeDirCommand(string cmd_line, bool bg, string cmd_org);
  virtual ~ChangeDirCommand() {}
  void execute() override;

  bool found_third;
  bool found_doubledot;
};

class GetCurrDirCommand : public BuiltInCommand {
 public:
  GetCurrDirCommand(string cmd_line, bool bg, string cmd_org);
  virtual ~GetCurrDirCommand() {}
  void execute() override;
};

class ShowPidCommand : public BuiltInCommand {
 public:
  ShowPidCommand(string cmd_line, bool bg, string cmd_org);
  virtual ~ShowPidCommand() {}
  void execute() override;
};

class JobsList;
class QuitCommand : public BuiltInCommand {
// TODO: Add your data members
public:
  QuitCommand(string cmd_line, bool bg, string cmd_org);
  virtual ~QuitCommand() {}
  void execute() override;

  bool found_kill;
};


class JobsList {
 public:
  class JobEntry {
   // TODO: Add your data members
  public:
   int jobID;
   bool stopped;
   bool finished;
   std::string cmd_txt;
   pid_t pid;
   time_t created;
   std::string cmd_org;
   int duration;
   JobEntry(int jobID, bool stopped, string cmd_line, pid_t pid, string cmd_org);
   ~JobEntry() = default;
   bool operator< (const JobEntry& job);
   //bool JobsList::JobEntry::operator<(const JobEntry& job1, const JobEntry& job2);
  };
 // TODO: Add your data members
 public:
  JobsList();
  ~JobsList();
  void addJob(string cmd_line, pid_t pid, string cmd_org, bool isStopped);
  void addToTimeoutList(string cmd_line, pid_t pid, string cmd_org, bool isStopped, int duration);
  void printJobsList();
  void killAllJobs();
  void removeFinishedJobs();
  std::shared_ptr<JobEntry> getJobById(int jobId);
  void removeJobById(int jobId);
  std::shared_ptr<JobEntry> getLastJob();
  std::shared_ptr<JobEntry> getLastStoppedJob(int *jobId);
  bool checkIfFinished(pid_t pid);
  // TODO: Add extra methods or modify exisitng ones as needed
  void insert_in_order(std::shared_ptr<JobsList::JobEntry> element , bool (*compare)(std::shared_ptr<JobsList::JobEntry>& job1, std::shared_ptr<JobsList::JobEntry>& job2));
  //void insert_in_order(std::shared_ptr<JobsList::JobEntry> element);
  std::list<std::shared_ptr<JobEntry>> jobs_list;
  std::list<std::shared_ptr<JobEntry>>::iterator findInList(int jobID);
  int getSize();
  //bool compareID(std::shared_ptr<JobsList::JobEntry>& job1, std::shared_ptr<JobsList::JobEntry>& job2);
  //bool compareTimes(std::shared_ptr<JobsList::JobEntry>& job1, std::shared_ptr<JobsList::JobEntry>& job2);

};

class JobsCommand : public BuiltInCommand {
 // TODO: Add your data members
 public:
  JobsCommand(string cmd_line, bool bg, string cmd_org);
  virtual ~JobsCommand() {}
  void execute() override;
};

class ForegroundCommand : public BuiltInCommand {
 // TODO: Add your data members
 public:
  ForegroundCommand(string cmd_line, bool bg, string cmd_org);
  virtual ~ForegroundCommand() {}
  void execute() override;

  bool found_third;
  int jobid;
};

class BackgroundCommand : public BuiltInCommand {
 // TODO: Add your data members
 public:
  BackgroundCommand(string cmd_line, bool bg, string cmd_org);
  virtual ~BackgroundCommand() {}
  void execute() override;

  bool found_third;
  int jobid;
};

class TimeoutCommand : public BuiltInCommand {
/* Optional */
// TODO: Add your data members
 public:
  explicit TimeoutCommand(string cmd_line, bool bg, string cmd_org);
  virtual ~TimeoutCommand() {}
  void execute() override;
  int duration;
  bool found_not_number;
};

class FareCommand : public BuiltInCommand {
  /* Optional */
  // TODO: Add your data members
 public:
  FareCommand(string cmd_line, bool bg, string cmd_org);
  virtual ~FareCommand() {}
  void execute() override;
};

class SetcoreCommand : public BuiltInCommand {
  /* Optional */
  // TODO: Add your data members
 public:
  SetcoreCommand(string cmd_line, bool bg, string cmd_org);
  virtual ~SetcoreCommand() {}
  void execute() override;
};

class KillCommand : public BuiltInCommand {
  /* Bonus */
 // TODO: Add your data members
 public:
  KillCommand(string cmd_line, bool bg, string cmd_org);
  virtual ~KillCommand() {}
  void execute() override;

  int sig_num;
  int jobid;
  bool found_not_numbers;
};

class SmallShell {
 private:
  // TODO: Add your data members
  SmallShell();
 public:
  std::shared_ptr<Command> CreateCommand(string cmd_line);
  SmallShell(SmallShell const&)      = delete; // disable copy ctor
  void operator=(SmallShell const&)  = delete; // disable = operator
  static SmallShell& getInstance() // make SmallShell singleton
  {
    static SmallShell instance; // Guaranteed to be destroyed.
    // Instantiated on first use.
    return instance;
  }
  ~SmallShell();
  void executeCommand(string cmd_line);
  // TODO: add extra methods as needed
  std::string smash_name;
  pid_t pid;
  std::string path;
  std::string last_path;
  std::string last_path_saver;
  //std::string new_path;
  //std::string new_last_path;

  JobsList list;
  JobsList timeout_list;

  std::shared_ptr<JobsList::JobEntry> fg_job;
};

#endif //SMASH_COMMAND_H_