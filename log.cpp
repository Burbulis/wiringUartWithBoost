#include "_log.hpp"

    std::ostringstream _LOG::out_s;
	std::string _LOG::logFileName;

    
	std::string 
    _LOG::timeStamp()
	{
		std::ostringstream strs;
		auto t = std::time(nullptr);
		//tm time;
		auto time = std::localtime(&t);//err = localtime(&time ,&t);
		strs << "|" << std::put_time(time, "%d-%m-%Y %H-%M-%S") << "|";
    	return (strs.str());
	}
	void _LOG::initLog(std::string _log)
	{
		logFileName = _log;
	}
	_LOG::_LOG(
		std::string message,
		std::string file,
    		int line
		)
	{
		std::ostringstream log;
		log << timeStamp() <<" file:" << file << " line:" <<  line <<" ["<< message <<"]" <<std::endl;
	//	printf("%s\n",log.str().c_str());
		if (!logFileName.empty())
			save(logFileName,log);
	} 

	_LOG::_LOG(
		std::ostringstream& message,
		std::string file,
		int line
		)
	{
		std::ostringstream log;
		log << timeStamp() << " file:" << file << " line:" << line <<" ["<< message.str() <<"]" <<std::endl;
	//	printf("%s\n",log.str().c_str());
		if (!logFileName.empty())
			save(logFileName,log);
		message.str("");
	}

	_LOG::_LOG(
		std::ostringstream& message,
		std::string file,
		int line,
		std::string toFile
		)
	{
		std::ostringstream log;
		log << timeStamp() << " file:" << file << " line:" << line <<" ["<< message.str() <<"]" <<std::endl;
		save(toFile,log);
		message.str("");
	}
	_LOG::_LOG(
		std::string message,
		std::string file,
        int line,
		std::string toFile
		)
	{
		std::ostringstream xlog;
		xlog << " file:" << file << " line:" <<  line <<" ["<< message <<"]" <<std::endl;
		std::ofstream write(toFile,std::ios::app);
		write << xlog.str();
	} 

	_LOG::~_LOG()
	{
		out_s.str("");
	}