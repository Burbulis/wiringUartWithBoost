#include <string>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "includeFiles.h"

class pidBlocking
{
	int pid_fd;
	std::string pid_file_name;
public:
	pidBlocking();
	~pidBlocking();
	void createPidFile(std::string pid_file_name);
	bool lock(void);
	void setPid(void);
	void destroyPidFile(void);
};