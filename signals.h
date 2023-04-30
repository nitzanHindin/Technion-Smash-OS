#ifndef SMASH__SIGNALS_H_
#define SMASH__SIGNALS_H_

#include "sys/wait.h"

void ctrlZHandler(int sig_num);
void ctrlCHandler(int sig_num);
//struct sigaction alarmHandler(int sig_num);
void alarmHandler(int sig_num, siginfo_t *siginfo, void *context);

#endif //SMASH__SIGNALS_H_
