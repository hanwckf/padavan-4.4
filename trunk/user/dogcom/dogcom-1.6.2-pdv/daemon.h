#ifndef DAEMON_H_
#define DAEMON_H_

void kill_daemon();
void signal_handler(int signal);
void daemonise();

extern int daemon_flag;
extern int pid_file_handle;

#endif // DAEMON_H_