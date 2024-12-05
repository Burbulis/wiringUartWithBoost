#include "locker.h"

    
	pidBlocking::pidBlocking():pid_fd(-1),pid_file_name(""){};
    pidBlocking::~pidBlocking(){destroyPidFile();}
	
	void pidBlocking::
	createPidFile(std::string pid_file_name)
	{
		this->pid_file_name = pid_file_name;
		char str[256];
		pid_fd = open(pid_file_name.c_str(), O_RDWR|O_CREAT, 0640);
		
		if (pid_fd  < 0)
			throw std::runtime_error(std::string("can't open file : " + pid_file_name));
	}

	bool 
    pidBlocking::lock(void)
	{
		if ((pid_file_name.empty()) || (pid_fd < 0))
			throw std::runtime_error("can't lock undefined or unopened file!");	

		return (lockf(pid_fd, F_TLOCK, 0) >= 0) ;		
	}

	void 
    pidBlocking::setPid(void)
	{
		if ((pid_file_name.empty()) || (pid_fd < 0))
			throw std::runtime_error("can't setPid to undefined or unopened file!");	
	    char str[260];
		sprintf(str, "%d\n", getpid());
		write(pid_fd, str, strlen(str));
	}

	void
    pidBlocking::destroyPidFile(void)
	{
		if ((pid_file_name.empty()) || (pid_fd < 0))
			throw std::runtime_error(std::string("can't destroy undefined or unopened file!") + std::to_string(pid_fd));	

		lockf(pid_fd, F_ULOCK, 0);
		close(pid_fd);
		pid_fd = 0;
		if (unlink(pid_file_name.c_str())<0)
		{
		 	int err = errno;
			throw std::runtime_error(std::string("can't unlink file! filename:")+ pid_file_name );
		}	
		return;
	}