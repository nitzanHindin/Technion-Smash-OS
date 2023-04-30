//#include <unistd.h>
//#include <string.h>
#include <iostream>
//#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
#include "Commands.h"

using namespace std;

#if 0
#define FUNC_ENTRY()  \
  cout << _PRETTY_FUNCTION_ << " --> " << endl;

#define FUNC_EXIT()  \
  cout << _PRETTY_FUNCTION_ << " <-- " << endl;
#else
#define FUNC_ENTRY()
#define FUNC_EXIT()
#endif

const string WHITESPACE = " \n\r\t\f\v";
const string NUMBERS = "0123456789";

string _ltrim(const std::string& s)
{
  size_t start = s.find_first_not_of(WHITESPACE);
  return (start == std::string::npos) ? "" : s.substr(start);
}

string _rtrim(const std::string& s)
{
  size_t end = s.find_last_not_of(WHITESPACE);
  return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

string _trim(const std::string& s)
{
  return _rtrim(_ltrim(s));
}

int _parseCommandLine(const std::string& cmd_line, char** args) {
  FUNC_ENTRY()
  int i = 0;
  std::istringstream iss(_trim(string(cmd_line)).c_str());
  for(std::string s; iss >> s; ) {
    args[i] = (char*)malloc(s.length()+1);
    memset(args[i], 0, s.length()+1);
    strcpy(args[i], s.c_str());
    args[++i] = NULL;
  }
  return i;

  FUNC_EXIT()
}

bool _isBackgroundComamnd(const char* cmd_line) {
  const string str(cmd_line);
  return str[str.find_last_not_of(WHITESPACE)] == '&';
}

int _isPipeComamnd(const char* cmd_line) {
  const string str(cmd_line);
  if(str.find_first_of('|') != std::string::npos)
    return str.find_first_of('|');
  return ERROR;
}

int _isRedirectionComamnd(const char* cmd_line) {
  const string str(cmd_line);
  if(str.find_first_of('>') != std::string::npos)
    return str.find_first_of('>');
  return ERROR;
}

void _removeBackgroundSign(char* cmd_line) {
  const string str(cmd_line);
  // find last character other than spaces
  unsigned int idx = str.find_last_not_of(WHITESPACE);
  // if all characters are spaces then return
  if (idx == string::npos) {
    return;
  }
  // if the command line does not end with & then return
  if (cmd_line[idx] != '&') {
    return;
  }
  // replace the & (background sign) with space and then remove all tailing spaces.
  cmd_line[idx] = ' ';
  // truncate the command line string up to the last non-space character
  cmd_line[str.find_last_not_of(WHITESPACE, idx) + 1] = 0;
}

// TODO: Add your implementation for classes in Commands.h

SmallShell::SmallShell() {
  smash_name = "smash";
  pid = getpid();
  char* got_path = get_current_dir_name();
  path = std::string(got_path);
  free((void*)got_path);
  got_path = nullptr;
  last_path = "";
  last_path_saver = "";
  //new_path = "";
  //new_last_path = "";
  fg_job = std::make_shared<JobsList::JobEntry>(ERROR, false, "", ERROR, "");
  /*list = nullptr;
  timeout_list = nullptr;*/
}

SmallShell::~SmallShell() {
// TODO: add your implementation
  list.jobs_list.clear();
  fg_job = nullptr;
}

/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
std::shared_ptr<Command> SmallShell::CreateCommand(std::string cmd_line) {
  string cmd_s = _trim(string(cmd_line));
  bool found_bg = false;
  if(_isBackgroundComamnd(cmd_s.c_str()))
  {
    found_bg = true;
    _removeBackgroundSign((char*)cmd_s.c_str());
    cmd_s = _trim((char*)cmd_s.c_str());
  }
  
  if(_isPipeComamnd(cmd_s.c_str()) != ERROR) {
    return std::shared_ptr<Command>(new PipeCommand(cmd_s, false, cmd_line));
  }
  else if(_isRedirectionComamnd(cmd_s.c_str()) != ERROR) {
    return std::shared_ptr<Command>(new RedirectionCommand(cmd_s, false, cmd_line));
  }

  string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));

  if (firstWord.compare("chprompt") == 0) {
    return std::shared_ptr<Command>(new chpromptCommand(cmd_s, false, cmd_line));
  }
  else if (firstWord.compare("showpid") == 0) {
    return std::shared_ptr<Command>(new ShowPidCommand(cmd_s, false, cmd_line));
  }
  else if (firstWord.compare("pwd") == 0) {
    return std::shared_ptr<Command>(new GetCurrDirCommand(cmd_s, false, cmd_line));
  }
  else if (firstWord.compare("cd") == 0) {
    return std::shared_ptr<Command>(new ChangeDirCommand(cmd_s, false, cmd_line));
  }
  else if (firstWord.compare("jobs") == 0) {
    return std::shared_ptr<Command>(new JobsCommand(cmd_s, false, cmd_line));
  }
  else if (firstWord.compare("fg") == 0) {
    return std::shared_ptr<Command>(new ForegroundCommand(cmd_s, false, cmd_line));
  }
  else if(firstWord.compare("bg") == 0) {
    return std::shared_ptr<Command>(new BackgroundCommand(cmd_s, false, cmd_line));
  }
  else if(firstWord.compare("quit") == 0) {
    return std::shared_ptr<Command>(new QuitCommand(cmd_s, false, cmd_line));
  }
  else if(firstWord.compare("kill") == 0) {
    return std::shared_ptr<Command>(new KillCommand(cmd_s, false, cmd_line));
  }
  else if(firstWord.compare("timeout") == 0)
    return std::shared_ptr<Command>(new TimeoutCommand(cmd_s, found_bg, cmd_line));

  return std::shared_ptr<Command>(new ExternalCommand(cmd_s, found_bg, cmd_line, -1));
}

void SmallShell::executeCommand(string cmd_line) {
  // TODO: Add your implementation here
  // for example:
  // Command* cmd = CreateCommand(cmd_line);
  // cmd->execute();
  // Please note that you must fork smash process for some commands (e.g., external commands....)

  SmallShell::getInstance().list.removeFinishedJobs();

  std::shared_ptr<Command> cmd = CreateCommand(cmd_line);
  cmd->execute();
}

Command::Command(string cmd_line, bool bg, string cmd_org) : bg(bg), cmd_org(cmd_org), timeout(false){
  string cmd_s = _trim(cmd_line);
  cmd = cmd_s.substr(0, cmd_s.find_first_of(" \n"));
  *args = {0};
  _parseCommandLine(cmd_line, args);
}

Command::~Command() {
  for(int i=0; i<21; i++)
  {
    if(args[i]) {
      free(args[i]);
      args[i] = nullptr;
    }
  }
}

BuiltInCommand::BuiltInCommand(string cmd_line, bool bg, string cmd_org) : Command(cmd_line, bg, cmd_org) {}

chpromptCommand::chpromptCommand(string cmd_line, bool bg, string cmd_org) : BuiltInCommand(cmd_line, bg, cmd_org)
{
    string cmd_s = _trim(cmd_line);
    //string second_word = cmd_s.substr(cmd_s.find_first_of(" ") + 1, cmd_s.find_first_of(" \n"));
    if (!args[1]){
      saved_name = "smash";
    }
    else{
      saved_name = std::string(args[1]);
    }
}

  void chpromptCommand::execute()
  {
    SmallShell::getInstance().smash_name = this->saved_name;
  }

  ShowPidCommand::ShowPidCommand(string cmd_line, bool bg, string cmd_org) : BuiltInCommand(cmd_line, bg, cmd_org){}

  void ShowPidCommand::execute()
  {
    std::cout << "smash pid is " << SmallShell::getInstance().pid << std::endl;
  }

  GetCurrDirCommand::GetCurrDirCommand(string cmd_line, bool bg, string cmd_org) : BuiltInCommand(cmd_line, bg, cmd_org){}

  void GetCurrDirCommand::execute()
  {
    string path = SmallShell::getInstance().path.c_str();
    std::cout << path;
    std::cout << std::endl;
  }

  ChangeDirCommand::ChangeDirCommand(string cmd_line, bool bg, string cmd_org) : BuiltInCommand(cmd_line, bg, cmd_org)
  {
    /*found_third = false;

    string cmd_s = _trim(string(cmd_line));
    string second_word = cmd_s.substr(cmd_s.find_first_of(" ") + 1, cmd_s.find_first_of("\n"));
    second_word = _trim(second_word);
    if(second_word.find_first_of(" ") != std::string::npos)
    {
      found_third = true;
      return;
    }

    if(second_word.find_last_of("/") == second_word.length() - 1)
    {
      second_word = second_word.substr(0, second_word.size() - 1);
    }

    if(second_word.compare("-") == 0)
    {
      if(string(SmallShell::getInstance().last_path).compare("") == 0)
      {
        std::cerr << "smash error: cd: OLDPWD not set" << std::endl;
      }
      else
      {
        SmallShell::getInstance().new_path = SmallShell::getInstance().last_path;
        SmallShell::getInstance().new_last_path = SmallShell::getInstance().path;
      }
    }
    else if(second_word.compare("..") == 0)
    {
      if(SmallShell::getInstance().path.find_last_of("/") == std::string::npos)
      {
        SmallShell::getInstance().new_last_path = SmallShell::getInstance().last_path;
        SmallShell::getInstance().new_path = SmallShell::getInstance().path;
        return;
      }
      if(SmallShell::getInstance().path.find_last_of("/") == 0)
      {
        SmallShell::getInstance().new_last_path = SmallShell::getInstance().path;
        SmallShell::getInstance().new_path = "/";
        return;
      }
      string new_path = SmallShell::getInstance().path.substr(0, SmallShell::getInstance().path.find_last_of("/"));
      SmallShell::getInstance().new_last_path = SmallShell::getInstance().path;
      SmallShell::getInstance().new_path = new_path;
    }
    else
    {
      SmallShell::getInstance().new_last_path = SmallShell::getInstance().path;
      //SmallShell::getInstance().new_path = second_word;
      if(second_word.find_first_of("/") == 0)
      {
        SmallShell::getInstance().new_path = second_word;
      }
      else
      {
        SmallShell::getInstance().new_path = (SmallShell::getInstance().path + "/" + second_word);
      }
    }
    */

    found_third = false;
    found_doubledot = false;

    if(args[2])
    {
      found_third = true;
      return;
    }


    if(strcmp(args[1], "-") == 0)
    {
      if(string(SmallShell::getInstance().last_path).compare("") == 0)
      {
        std::cerr << "smash error: cd: OLDPWD not set" << std::endl;
      }
      else
      {
        string new_path = SmallShell::getInstance().last_path;
        SmallShell::getInstance().last_path = SmallShell::getInstance().path;
        SmallShell::getInstance().path = new_path;
      }
    }
    else if(strcmp(args[1], "..") == 0)
    {
      found_doubledot = true;
      SmallShell::getInstance().last_path = SmallShell::getInstance().path;
      SmallShell::getInstance().path = args[1];
    }
    else
    { // got a new path
      SmallShell::getInstance().last_path_saver = SmallShell::getInstance().last_path;
      SmallShell::getInstance().last_path = SmallShell::getInstance().path;
      SmallShell::getInstance().path = args[1];
    }
  }

  void ChangeDirCommand::execute()
  {
    if(!args[1])
    {
      std::cerr << "smash error:> \"" << cmd_org << "\"" << std::endl;
      return;
    }

    string second_word = string(args[1]);
    if((second_word.compare("-") == 0) && (string(SmallShell::getInstance().last_path).compare("") == 0))
      return;

    if(found_third)
    {
      std::cerr << "smash error: cd: too many arguments" << std::endl;
      return;
    }

    if(chdir(SmallShell::getInstance().path.c_str()) == ERROR)
    {
      perror("smash error: chdir failed");
      SmallShell::getInstance().path = SmallShell::getInstance().last_path;
      SmallShell::getInstance().last_path = SmallShell::getInstance().last_path_saver;
      SmallShell::getInstance().last_path_saver = "";
      return;
    }

    char* new_path = get_current_dir_name();
    /*if(new_path == 0)
    {
      perror("smash error: getcwd failed");
      return;
    }*/
    SmallShell::getInstance().path = new_path;

  }

  JobsList::JobEntry::JobEntry(int jobID, bool stopped, string cmd_line, pid_t pid, string cmd_org) : jobID(jobID), stopped(stopped), finished(false), cmd_txt(cmd_line), pid(pid), cmd_org(cmd_org)
  {
    if(time(&created) == ERROR)
    {
      perror("smash error: time failed");
    }
    duration = 0;
  }

  bool JobsList::JobEntry::operator< (const JobEntry& job){
    return this->jobID < job.jobID;
  }
  /*bool JobsList::JobEntry::operator< (const JobEntry& job1, const JobEntry& job2)
  {
    return job1<job2;
  }*/

  JobsList::JobsList()
  {
    jobs_list = std::list<std::shared_ptr<JobsList::JobEntry>>();
  }

  JobsList::~JobsList()
  {
    jobs_list.clear();
  }

  static bool compareID(std::shared_ptr<JobsList::JobEntry>& job1, std::shared_ptr<JobsList::JobEntry>& job2)
  {
    return job1->jobID < job2->jobID;
  }

  static bool compareTimes(std::shared_ptr<JobsList::JobEntry>& job1, std::shared_ptr<JobsList::JobEntry>& job2)
  {
    return (job1->created + job1->duration) < (job2->created + job2->duration);
  }

  void JobsList::insert_in_order(std::shared_ptr<JobsList::JobEntry> element , bool (*compare)(std::shared_ptr<JobsList::JobEntry>& job1, std::shared_ptr<JobsList::JobEntry>& job2)) {
    std::list<std::shared_ptr<JobsList::JobEntry>>::iterator begin = jobs_list.begin();
    std::list<std::shared_ptr<JobsList::JobEntry>>::iterator end = jobs_list.end();
    while ((begin != end) && compare(*begin,element)) {
        ++begin;
    }
    jobs_list.insert(begin, element);
}

void JobsList::addToTimeoutList(string cmd_line, pid_t pid, string cmd_org, bool isStopped, int duration)
{
  std::shared_ptr<JobsList::JobEntry> job = std::make_shared<JobsList::JobEntry>(-1, -1, cmd_line, pid, cmd_org);
  job.get()->duration = duration;
  insert_in_order(job, &compareTimes);
  SmallShell& smash = SmallShell::getInstance();
  std::list<std::shared_ptr<JobsList::JobEntry>>::iterator it = smash.timeout_list.jobs_list.begin();
  time_t current_time;
  if(time(&current_time) == ERROR)
  {
    perror("smash error: time failed");
    return;
  }
  alarm(it->get()->created + it->get()->duration - current_time);
}

  void JobsList::addJob(string cmd_line, pid_t pid, string cmd_org, bool isStopped)
  {
    SmallShell& smash = SmallShell::getInstance();
    if(smash.fg_job.get()->jobID == NEW_JOB || smash.fg_job.get()->jobID == ERROR)
    {
      if(smash.pid != getpid())
        return;
      int new_job_id = 0;
      if(jobs_list.size() != 0)
      {
        new_job_id = jobs_list.back()->jobID+1;
        if(smash.fg_job.get()->jobID == new_job_id)
          new_job_id++;
      }
      else
      {
        new_job_id = 1;
        if(smash.fg_job.get()->jobID == new_job_id)
          new_job_id++;
      }
      std::shared_ptr<JobsList::JobEntry> job = std::make_shared<JobsList::JobEntry>(new_job_id, isStopped, cmd_line, pid, cmd_org);
      insert_in_order(job, &compareID);
    }
    else
    {
      std::shared_ptr<JobsList::JobEntry> job = std::make_shared<JobsList::JobEntry>(smash.fg_job.get()->jobID, isStopped, cmd_line, pid, cmd_org);
      insert_in_order(job, &compareID);
    }
  }

  void JobsList::printJobsList()
  {
    for(std::list<std::shared_ptr<JobsList::JobEntry>>::iterator it = jobs_list.begin(); it != jobs_list.end(); ++it)
    {
      time_t current_time;
      time(&current_time);
      int diff_time = (int)difftime(current_time, it->get()->created);

      if(it->get()->stopped)
      {
        std::cout << "[" << it->get()->jobID << "] " << it->get()->cmd_org << " : " << it->get()->pid << " " << diff_time << " secs (stopped)" << std::endl;
      }
      else
      {
        std::cout << "[" << it->get()->jobID << "] " << it->get()->cmd_org << " : " << it->get()->pid << " " << diff_time << " secs" << std::endl;
      }
    }
  }

  void JobsList::killAllJobs() // TODO
  {

  }

  bool JobsList::checkIfFinished(pid_t pid)
  {
    int status = 0;
    pid_t tmp = waitpid(pid, &status, WNOHANG);
    if(tmp == ERROR)
    {
      perror("smash error: waitpid failed");
      return false;
    }

    while(tmp==0)
    {
       return false;
    }

    for(std::list<std::shared_ptr<JobsList::JobEntry>>::iterator it = SmallShell::getInstance().timeout_list.jobs_list.begin(); it != SmallShell::getInstance().timeout_list.jobs_list.end(); ++it)
    {    
      if(it->get()->pid == pid)
        it->get()->finished = true;
    }
      return true;
  }

  void JobsList::removeFinishedJobs()
  {
    pid_t pid = SmallShell::getInstance().pid;
    if(pid != getpid())
      return;

    for(std::list<std::shared_ptr<JobsList::JobEntry>>::iterator it = jobs_list.begin(); it != jobs_list.end(); )
    {
      if(JobsList::checkIfFinished((*it)->pid))
      {
        it = jobs_list.erase(it);
      }
      else
        it++;
    }
  }

  std::shared_ptr<JobsList::JobEntry> JobsList::getJobById(int jobId)
  {
    if(findInList(jobId) != jobs_list.end())
      return (*findInList(jobId));
    return nullptr;
  }

  void JobsList::removeJobById(int jobId)
  {
    if(findInList(jobId) != jobs_list.end())
      jobs_list.erase(findInList(jobId));
  }

  std::list<std::shared_ptr<JobsList::JobEntry>>::iterator JobsList::findInList(int jobID)
  {
    for(std::list<std::shared_ptr<JobsList::JobEntry>>::iterator it = jobs_list.begin(); it != jobs_list.end(); ++it)
    {
      if((*it)->jobID == jobID)
      {
        return it;
      }
    }

    return jobs_list.end();
  }

  int JobsList::getSize()
  {
    return jobs_list.size();
  }

  std::shared_ptr<JobsList::JobEntry> JobsList::getLastJob()
  {
     return getJobById(jobs_list.back()->jobID);
  }

  std::shared_ptr<JobsList::JobEntry> JobsList::getLastStoppedJob(int *jobId)
  {
    for(std::list<std::shared_ptr<JobsList::JobEntry>>::reverse_iterator it = jobs_list.rbegin(); it != jobs_list.rend(); ++it)
    {
      if((*it)->stopped)
      {
        *jobId = (*it)->jobID;
        return (*it);
      }
    }

    return nullptr;
  }

  JobsCommand::JobsCommand(string cmd_line, bool bg, string cmd_org) : BuiltInCommand(cmd_line, bg, cmd_org) {}

  void JobsCommand::execute()
  {
    SmallShell::getInstance().list.printJobsList();
  }

  ForegroundCommand::ForegroundCommand(string cmd_line, bool bg, string cmd_org) : BuiltInCommand(cmd_line, bg, cmd_org), found_third(false)
  {
    if(args[2] != 0)
    {
      found_third = true;
      return;
    }
    
    if(args[1] != 0)
    {
      try
      {
        jobid = stoi(std::string(args[1]));
      }
      catch(const std::invalid_argument& ia)
      {
        found_third = true;
      }
    }
  }

  void ForegroundCommand::execute()
  {
    if(found_third)
    {
      std::cerr << "smash error: fg: invalid arguments" << std::endl;
      return;
    }

    if(SmallShell::getInstance().pid != getpid())
      return;

    std::shared_ptr<JobsList::JobEntry> job;

    if(args[1] == 0)
    {
      if(SmallShell::getInstance().list.getSize() == 0)
      {
        std::cerr << "smash error: fg: jobs list is empty" << std::endl;
        return;
      }
      job = SmallShell::getInstance().list.getLastJob();
      jobid = job.get()->jobID;
    }
    else
    {
      job = SmallShell::getInstance().list.getJobById(jobid);
      if(!job)
      {
        std::cerr << "smash error: fg: job-id " << jobid << " does not exist" << std::endl;
        return;
      }
    }

    std::cout << job->cmd_org << " : " << job->pid << std::endl;

    if(job->stopped)
    {
      if(kill(job->pid, SIGCONT) == ERROR)
      {
        perror("smash error: kill failed");
        return;
      }
      job->stopped = false;
    }

    JobsList::JobEntry * fg_job = SmallShell::getInstance().fg_job.get();
    fg_job->jobID = jobid;
    fg_job->stopped = false;
    fg_job->cmd_txt = job->cmd_txt;
    fg_job->pid = job->pid;
    fg_job->cmd_org = job->cmd_org;


    SmallShell::getInstance().list.removeJobById(jobid);

    int status = 0;
    if(waitpid(job->pid, &status, WUNTRACED) == ERROR)
    {
      perror("smash error: waitpid failed");
      return;
    }
    job->finished = true;
  }

  BackgroundCommand::BackgroundCommand(string cmd_line, bool bg, string cmd_org) : BuiltInCommand(cmd_line, bg, cmd_org), found_third(false)
  {
    if(args[2] != 0)
    {
      found_third = true;
      return;
    }
    
    if(args[1] != 0)
    {
      try
      {
        jobid = stoi(std::string(args[1]));
      }
      catch(const std::invalid_argument& ia)
      {
        found_third = true;
      }
    }
  }
  void BackgroundCommand::execute() {
    if(SmallShell::getInstance().pid != getpid())
      return;

    if(found_third)
    {
      std::cerr << "smash error: bg: invalid arguments" << std::endl;
      return;
    }

    std::shared_ptr<JobsList::JobEntry> job;

    if(args[1] == 0)
    {
      job = SmallShell::getInstance().list.getLastStoppedJob(&jobid);
      if(SmallShell::getInstance().list.getSize() == 0 || !job)
      {
        std::cerr << "smash error: bg: there is no stopped jobs to resume" << std::endl;
        return;
      }
    }
    else
    {
      job = SmallShell::getInstance().list.getJobById(jobid);
      if(!job)
      {
        std::cerr << "smash error: bg: job-id " << jobid << " does not exist" << std::endl;
        return;
      }

      if(!job->stopped)
      {
        std::cerr << "smash error: bg: job-id " << jobid << " is already running in the background" << std::endl;
        return;
      }
    }

    std::cout << job->cmd_org << " : " << job->pid << std::endl;

    if(kill(job->pid, SIGCONT) == ERROR)
    {
      perror("smash error: kill failed");
      return;
    }
    job->stopped = false;
  }

  QuitCommand::QuitCommand(string cmd_line, bool bg, string cmd_org) : BuiltInCommand(cmd_line, bg, cmd_org)
  {
    string cmd_s = _trim(string(cmd_line));
    string second_word = cmd_s.substr(cmd_s.find_first_of(" ") + 1, cmd_s.find_first_of(" \n"));
    second_word = _trim(second_word);

    if(second_word == "kill")
      found_kill = true;
    else
      found_kill = false;
  }

  void QuitCommand::execute()
  {
    if(found_kill)
    {
      std::cout << "smash: sending SIGKILL signal to " << SmallShell::getInstance().list.getSize() << " jobs:" << std::endl;
      for(auto const &job : SmallShell::getInstance().list.jobs_list)
      {
        std::cout << job->pid << ": " << job->cmd_org << std::endl;
        if(kill(job->pid, SIGKILL) == ERROR)
        {
         perror("smash error: kill failed");
         return;
        }
      }
    }

    exit(0);
  }

  KillCommand::KillCommand(string cmd_line, bool bg, string cmd_org) : BuiltInCommand(cmd_line, bg, cmd_org), found_not_numbers(false)
  {
    int size =0;
    for(int i = 0; i<21; i++)
    {
      if(args[i])
        size++;
      else
        break;
    }
    if(size > 4 || size < 3){
      found_not_numbers = true;
      return;      
    }
    if(strcmp(args[1], "-")==0) // size = 4
    { // check if 3rd and 4th words are numbers
      if((std::string(args[2]).find_first_not_of(NUMBERS) != std::string::npos) || (std::string(args[3]).find_first_not_of(NUMBERS) != std::string::npos)){
        found_not_numbers = true;
        return;
      }
      
      sig_num = stoi(std::string(args[2]));
      jobid = stoi(std::string(args[3]));
    }
    else
    { // check if 2nd word is -num and 3rd is a number
      if(args[3] || (std::string(args[1]).find_last_not_of(NUMBERS) != 0) || (std::string(args[2]).find_last_not_of(NUMBERS) != 0 && std::string(args[2]).find_last_not_of(NUMBERS) != std::string::npos)){
        found_not_numbers = true;
        return;
      }
      
      try
      {
        sig_num = stoi(std::string(args[1]));
        if(sig_num < 0)
          sig_num *= -1;
        jobid = stoi(std::string(args[2]));
      }
      catch(const std::invalid_argument& ia)
      {
        found_not_numbers = true;
      }
    }
  }

  void KillCommand::execute()
  {
    if(found_not_numbers || sig_num < 1 || sig_num > 31)
    {
      std::cerr << "smash error: kill: invalid arguments" << std::endl;
      return;
    }
  
    if(SmallShell::getInstance().list.findInList(jobid) == SmallShell::getInstance().list.jobs_list.end())
    {
      std::cerr << "smash error: kill: job-id " << jobid << " does not exist" << std::endl;
      return;
    }

    pid_t job_pid = (*SmallShell::getInstance().list.findInList(jobid))->pid;

    if(kill(job_pid, sig_num) == ERROR)
    {
      perror("smash error: kill failed");
      return;
    }

    std::cout << "signal number " << sig_num << " was sent to pid " << job_pid << std::endl;
  }

  ExternalCommand::ExternalCommand(string cmd_line, bool bg, string cmd_org, int duration) : Command(cmd_line, bg, cmd_org), duration(duration)
  {
    string cmd_s = _trim(string(cmd_line));
    if(cmd_s.find_first_of("*") != std::string::npos || cmd_s.find_first_of("?") != std::string::npos)
      complex = true;
    else
      complex = false;

    cmd_txt = cmd_line;
    timeout = false;
  }

  void ExternalCommand::execute()
  {
    pid_t pid = fork();
    if(pid == ERROR)
    {
      perror("smash error: fork failed");
      return;
    }

    if(pid == 0)
    { // son
      // kill(SmallShell::getInstance().pid, 14); FOR DEBUGGING PURPOSES
      if(setpgrp() == ERROR)
      {
        perror("smash error: setpgrp failed");
        exit(0);
        //return;
      }
      if(complex)
      {
        char* outside_args[3] = {0};
        outside_args[0] = (char*)"/bin/bash";
        outside_args[1] = (char*)"-c";
        outside_args[2] = (char*)cmd_txt.c_str();

        if(execv(outside_args[0], outside_args) == ERROR)
        {
          perror("smash error: execv failed");
          //exit(0);
          //return;
        }
      }
      else
      {
        if(execvp(this->args[0], this->args) == ERROR)
        {
          perror("smash error: execvp failed");
          //exit(0);
          //return;
        }

      }
      exit(0);
    }
    else
    { // father
      SmallShell& smash = SmallShell::getInstance(); 
      if(timeout)
      {
        if(bg)
        {
          smash.list.addJob(cmd_txt, pid, cmd_org, false);
          smash.timeout_list.addToTimeoutList(cmd_txt, pid, cmd_org, false, duration);
        }
        else
        {
          //SmallShell::getInstance().fg_job = std::make_shared<JobsList::JobEntry>(NEW_JOB, false, cmd_txt, pid, cmd_org);
          JobsList::JobEntry* job = smash.fg_job.get();
          job->finished = false;
          job->pid = pid;
          job->jobID = NEW_JOB;
          job->stopped = false;
          job->cmd_txt = cmd_txt;
          job->cmd_org = cmd_org;
          
          smash.timeout_list.addToTimeoutList(cmd_txt, pid, cmd_org, false, duration);

          if(waitpid(pid, nullptr, WUNTRACED) == ERROR)
          {
            perror("smash error: waitpid failed");
            return;
          }

          for(std::list<std::shared_ptr<JobsList::JobEntry>>::iterator it2 = smash.timeout_list.jobs_list.begin(); it2 != smash.timeout_list.jobs_list.end(); ++it2)
          {
            if(it2->get()->pid == job->pid)
              it2->get()->finished = true;
          }
        }
      }

      else
      {
        if(bg)
        {
          smash.list.addJob(cmd_txt, pid, cmd_org, false);
        }
        else{
          //SmallShell::getInstance().fg_job = std::make_shared<JobsList::JobEntry>(NEW_JOB, false, cmd_txt, pid, cmd_org);
          JobsList::JobEntry* job = smash.fg_job.get();
          job->finished = false;
          job->pid = pid;
          job->jobID = NEW_JOB;
          job->stopped = false;
          job->cmd_txt = cmd_txt;
          job->cmd_org = cmd_org;
          
          if(waitpid(pid, nullptr, WUNTRACED) == ERROR)
          {
            perror("smash error: waitpid failed");
            return;
          }
        }
      }
    }
  }


TimeoutCommand::TimeoutCommand(string cmd_line, bool bg, string cmd_org) : BuiltInCommand(cmd_line, bg, cmd_org), found_not_number(false)
{
  if(!args[1] || !args[2] || std::string(args[1]).find_first_not_of(NUMBERS) != std::string::npos)
  {
    found_not_number = true;
    return;
  }
  duration = stoi(std::string(args[1]));
}

void TimeoutCommand::execute()
{
  if(found_not_number){
    //std::cerr << "smash error: timeout: invalid arguments" << std::endl;
    std::cerr << "smash error:> \"" << cmd_org << "\"" << std::endl;
    return;
  }
  if(duration == 0)
  {
    std::cout << "smash: got an alarm" << std::endl;
    std::cout << "smash: " << cmd_org << " timed out!" << std::endl;
    return;
  }
  //alarm(duration);
  std::string time_cmd = _trim(cmd_org);
  time_cmd.erase(0, time_cmd.find_first_of(" "));
  time_cmd = _trim(time_cmd);
  time_cmd.erase(0, time_cmd.find_first_of(" "));
  time_cmd = _trim(time_cmd);
  if(bg)
  {
    _removeBackgroundSign((char*)time_cmd.c_str());
    time_cmd = _trim((char*)time_cmd.c_str());
  }
  std::shared_ptr<Command> external_cmd  = std::shared_ptr<Command>(new ExternalCommand(time_cmd, bg, cmd_org, duration));
  external_cmd.get()->timeout = true;
  external_cmd.get()->bg = bg;
  external_cmd->execute();
}

PipeCommand::PipeCommand(string cmd_line, bool bg, string cmd_org) : Command(cmd_line, bg, cmd_org), not_a_pipe_command(false), stderr_flag(false), background_index(0)
{
  pipe_index = cmd_line.find_first_of('|');

  std::string rest_of_word = cmd_line.substr(pipe_index+1, cmd_line.size()-1);
  rest_of_word = _trim(rest_of_word);

  if(rest_of_word[0] == '&')
  {
    stderr_flag = true;
    background_index = cmd_line.size() - rest_of_word.size() + 1;
  }

  cmd_line = _trim((char*)cmd_line.c_str());
  if(pipe_index == cmd_line.size() -1 || background_index == cmd_line.size() -1)
    not_a_pipe_command = true;

}

void PipeCommand::execute() 
{
  if(not_a_pipe_command)
  {
    //std::cerr << "smash error: pipecommand: invalid arguments" << std::endl;   SHOULD WE?
    return;
  }

  std::string first_command = cmd_org.substr(0, pipe_index);
  /*if(_isBackgroundComamnd((char*)first_command.c_str()))
    _removeBackgroundSign((char*)first_command.c_str());*/

  std::string second_command;
  if(stderr_flag)
    second_command = cmd_org.substr(background_index+1, cmd_org.length());
  else
    second_command = cmd_org.substr(pipe_index+1, cmd_org.length());
  /*if(_isBackgroundComamnd((char*)second_command.c_str()))
    _removeBackgroundSign((char*)second_command.c_str());*/

  int close_fd = stderr_flag+1;

  int my_pipe[2];
  if(pipe(my_pipe) == ERROR)
  {
    perror("smash error: pipe failed");
    return;
  }

  pid_t first_pid = fork();
  if(first_pid == ERROR)
  {
    perror("smash error: fork failed");
    return;
  }

  if (first_pid == 0) { // first command
    if(setpgrp() == ERROR)
    {
      perror("smash error: setpgrp failed");
      exit(0);
      //return;
    }

    if(close(close_fd) == ERROR)
    {
      perror("smash error: close failed");
      exit(0);
      //return;
    }

    if(dup2(my_pipe[1], close_fd) == ERROR)
    {
      perror("smash error: dup2 failed");
      exit(0);
      //return;
    }

    if(close(my_pipe[1]) == ERROR)
    {
      perror("smash error: close failed");
      exit(0);
      //return;
    }

    std::shared_ptr<Command> first_process = SmallShell::getInstance().CreateCommand(first_command);
    first_process.get()->bg = false;
    first_process.get()->execute();

    if(close(close_fd) == ERROR)
    {
      perror("smash error: close failed");
      exit(0);
      //return;
    }

    exit(0);
  } 
  else
  { // smash
    if(close(my_pipe[1]) == ERROR)
    {
      perror("smash error: close failed");
      exit(0);
      //return;
    }

    pid_t second_pid = fork();
    if(second_pid == ERROR)
    {
      perror("smash error: fork failed");
      exit(0);
      //return;
    }

    if (second_pid == 0) { // second command

      if(setpgrp() == ERROR)
      {
        perror("smash error: setpgrp failed");
        exit(0);
        //return;
      }

      if(close(0) == ERROR)
      {
        perror("smash error: close failed");
        exit(0);
        //return;
      }

      if(dup2(my_pipe[0], 0) == ERROR)
      {
        perror("smash error: close failed");
        exit(0);
        //return;
      }

      if(close(my_pipe[0]) == ERROR)
      {
        perror("smash error: close failed");
        exit(0);
        //return;
      }

      std::shared_ptr<Command> second_process = SmallShell::getInstance().CreateCommand(second_command);
      second_process.get()->bg = false;
      second_process.get()->execute();

      if(close(0) == ERROR)
      {
        perror("smash error: close failed");
        exit(0);
        //return;
      }

      exit(0);
    } 
    else
    {
      int status = 0;
    
      if((waitpid(first_pid, &status, WUNTRACED) == ERROR) 
          || (waitpid(second_pid, &status, WUNTRACED) == ERROR))
      {
        perror("smash error: waitpid failed");
        return;
      }

      if(close(my_pipe[0]) == ERROR)
      {
        perror("smash error: close failed");
        return;
      }
    }
  }
}

RedirectionCommand::RedirectionCommand(string cmd_line, bool bg, string cmd_org) : Command(cmd_line, bg, cmd_org), not_a_redirection(false), second_redirect_index(0)
{
  redirect_index = cmd_line.find_first_of('>');

  std::string rest_of_word = cmd_line.substr(redirect_index+1, cmd_line.size()-1);
  rest_of_word = _trim(rest_of_word);

  if(rest_of_word[0] == '>')
  {
    second_redirect_index = cmd_line.size() - rest_of_word.size() + 1;
  }

  cmd_line = _trim((char*)cmd_line.c_str());
  if(redirect_index == cmd_line.size() -1 || second_redirect_index == cmd_line.size() -1)
    not_a_redirection = true;

}

void RedirectionCommand::execute()
{
  if(not_a_redirection)
  {
    // std::cerr << "smash error: redirectioncommand: invalid arguments" << std::endl;   SHOULD WE?
    return;
  }

  std::string first_command = cmd_org.substr(0, redirect_index);
  /*if(_isBackgroundComamnd((char*)first_command.c_str()))
    _removeBackgroundSign((char*)first_command.c_str());*/

  int file_fd = -1;
  if(second_redirect_index)
  {
    std::string file_name = cmd_org.substr(second_redirect_index+1, cmd_org.length());
    file_name = _trim(file_name);
    mode_t mods = S_IRUSR | S_IWUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;
    file_fd = open(file_name.c_str(), O_RDWR | O_CREAT | O_APPEND, mods);
  }
  else
  {
    std::string file_name = cmd_org.substr(redirect_index+1, cmd_org.length());
    file_name = _trim(file_name);
    mode_t mods = S_IRUSR | S_IWUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;
    file_fd = open(file_name.c_str(), O_RDWR | O_CREAT | O_TRUNC, mods);
  }

  if(file_fd == ERROR)
  {
    perror("smash error: open failed");
    return;
  }

  pid_t command_pid = fork();
  if(command_pid == ERROR)
  {
    perror("smash error: fork failed");
    return;
  }

  if(command_pid == 0)
  {
    if(close(1) == ERROR)
    {
      perror("smash error: close failed");
      return;
    }

    if(dup2(file_fd, 1) == ERROR)
    {
      perror("smash error: dup2 failed");
      return;
    }

    std::shared_ptr<Command> cmd_process = SmallShell::getInstance().CreateCommand(first_command);
    cmd_process.get()->bg = false;
    cmd_process.get()->execute();

    if(close(file_fd) == ERROR)
    {
      perror("smash error: close failed");
      return;
    }

    if(close(1) == ERROR)
    {
      perror("smash error: close failed");
      return;
    }

    exit(0);
  }
  else
  {
    int status = 0;
    if(waitpid(command_pid, &status, WUNTRACED) == ERROR)
      {
        perror("smash error: waitpid failed");
        return;
      }


    if(close(file_fd) == ERROR)
    {
      perror("smash error: close failed");
      return;
    }
  }
}