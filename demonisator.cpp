#include "includeFiles.h"
#include "demonisator.h"
void demonize()
{
    pid_t pid = 0;
    pid = fork();
    _LOG::out_s << "pid = " << pid <<  std::endl;
    LOGTOFILE(LOGHTML::messageType::STRONG_WARNING,_LOG::out_s);
    if (pid < 0)
    {
        LOGTOFILE(LOGHTML::messageType::STRONG_WARNING,"EXIT_FAILURE");
	exit(EXIT_FAILURE);
    }
    if (pid > 0)
    {
        LOGTOFILE(LOGHTML::messageType::STRONG_WARNING,"EXIT_SUCCESS");
	exit(EXIT_SUCCESS);
    }
    if (setsid() < 0)
    {
        LOGTOFILE(LOGHTML::messageType::STRONG_WARNING,"EXIT_FAILURE");
	exit(EXIT_FAILURE);
    }
	pid = fork();
	if (pid < 0) {
		 LOGTOFILE(LOGHTML::messageType::STRONG_WARNING,"EXIT_FAILURE");
		exit(EXIT_FAILURE);
	}
	if (pid > 0) {
        LOGTOFILE(LOGHTML::messageType::STRONG_WARNING,"EXIT_SUCCESS");
		exit(EXIT_SUCCESS);
	}
    signal(SIGCHLD, SIG_IGN);
    umask(0);
    int fd = 0;
    for (fd = sysconf(_SC_OPEN_MAX);fd > 0;fd--)
    close(fd);
	stdin = fopen("/dev/null", "r");
	stdout = fopen("/dev/null", "w+");
	stderr = fopen("/dev/null", "w+");
}