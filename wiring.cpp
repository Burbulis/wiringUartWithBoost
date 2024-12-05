#include "demonisator.h"
#include "includeFiles.h"
#include "_log.hpp"
#include "config.h"
#include "locker.h"
#include <boost/version.hpp>
#include <boost/asio.hpp>
#include <boost/filesystem.hpp>
#include <sstream>
#include <algorithm>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

//#define _DEBUG_
//#define _PRE_DEBUG_
//#define _FILE_BLOCKS_DEBUG_
typedef unsigned char byte;

template
<
	typename T
>
class Factory
{
public:
	static void Create(void)
	{
		if (instance == NULL)
			instance = new T;
	}

	static T* getInstance(void)
	{
		return instance;
	}

	static void Destroy(void)
	{
		if (instance != NULL)
			delete instance;
		instance = NULL;
	}

private:

	static T* instance;
};

template <typename T>
	T* Factory<T>::instance = NULL;


static bool running = false;





enum{
	BASE_BUFFER_RESERVATION = 5000,
	TOTAL_BUFFER_RESERVATION= 500000
};

union u_chk{
	unsigned int chk;
	unsigned char out[sizeof(chk)];
};

//static pidBlocking *blocker = NULL;

void reconfig(config *conf);

std::optional<size_t>  
specialSearch(const size_t stertpos,const std::vector<byte>& buffer,const std::vector<byte>& token)
{
	if (buffer.size() < token.size())
	    return std::nullopt;        
     for (size_t i = 0 ; i < buffer.size() ;i++)
        if (0==memcmp(&buffer[i],&token[0],token.size()))
            return ( i );   
    return std::nullopt;        
}

struct bufferUnit
{
    std::vector<byte> data;
  	u_chk _chk;
	bool _dataReady;
	std::vector<byte> tbegin;
	std::vector<byte> tend;

	bufferUnit(std::vector<byte>& finalBuffer,const config& conf):_dataReady(false),tbegin(conf.getBegin()),tend(conf.getEnd()){
		getFullBlock(finalBuffer);

	}

	const std::vector<byte>& getData(void) const
	{
		return (data);
	}

	size_t getCheck(const std::vector<byte>& arrtosave)
	{
		size_t chkSum = 0; 
		for (size_t i = 0; i < arrtosave.size(); ++i)
		{
			chkSum += (arrtosave[i] ^ i); 
		}
		return (chkSum);
	}

	bool Ready(void)
	{
		return (_dataReady);
	};

    bool clear(std::vector<byte>& finalBuffer , std::optional<size_t>& beg,std::optional<size_t>& end )
    {
        	const size_t total = *end + tend.size();
			std::vector<byte>::iterator Itbeg = finalBuffer.begin();
			std::advance(Itbeg, *beg );
			std::vector<byte>::iterator Itend = finalBuffer.begin();
			std::advance(Itend, total );
			finalBuffer.erase( Itbeg , Itend );
        return (finalBuffer.size() == total);
    }

	void getFullBlock(std::vector<byte>& finalBuffer)
	{
		if ((finalBuffer.size() < tend.size()) && (finalBuffer.size() < tbegin.size()))
			return ;
		std::optional<size_t> beg = specialSearch(0,finalBuffer,tbegin/*std::vector<byte>(tbegin.begin(),tbegin.end())*/);
		if (!beg)
			return;
		std::optional<size_t> end = specialSearch((beg)?((*beg) + tbegin.size()):0,finalBuffer, tend/*std::vector<byte>(tend.begin(),tend.end())*/);
		if (!end)
			return ;

		_dataReady = false;
		const size_t SIZE_OF_BUFFER = finalBuffer.size();
		const size_t total = *end + tend.size();
		const size_t diff = finalBuffer.size() - total;
		if (SIZE_OF_BUFFER < total)
				return ; 

		#ifdef _PRE_DEBUG_
		_LOG::out_s << "finalBuffer.size() == " << finalBuffer.size() << " (0)beg = " << ((beg)?std::to_string(*beg):"N/A") << " end = " << ((end)?std::to_string(*end):"N/A") << " total = " <<  total
		<< " diff = " << diff;
		LOGTOFILE(LOGHTML::messageType::STRONG_WARNING,_LOG::out_s);	
		#endif 		
		
	
		if (*beg <= *end)
		{
			const size_t CHK_OFFSET = *end - sizeof(_chk.chk);
			const size_t BUFFER_SIZE = CHK_OFFSET - tend.size();
			const size_t BUFFER_OFFSET = *beg + tbegin.size();
			memcpy(_chk.out,&finalBuffer.data()[CHK_OFFSET],sizeof(_chk.chk));	
			data.resize(BUFFER_SIZE);
			memcpy(data.data(),&finalBuffer.data()[BUFFER_OFFSET],BUFFER_SIZE);	
			size_t chk_ = getCheck(data);
			_dataReady= ( _chk.chk == chk_ ) && (SIZE_OF_BUFFER >= total);

		#ifdef _PRE_DEBUG_
		_LOG::out_s << "finalBuffer.size() = " << finalBuffer.size() << "BUFFER_OFFSET = " << BUFFER_OFFSET <<" CHK_OFFSET = " << CHK_OFFSET << " BUFFER_SIZE = " << BUFFER_SIZE << " _chk.chk = " << _chk.chk << " chk_ = " << chk_ << std::endl 
				<< " >>*>> beg = " << ((beg)?std::to_string(*beg):"N/A") << " end = " << ((end)?std::to_string(*end):"N/A") << " total= " << total << std::endl;
		LOGTOFILE(LOGHTML::messageType::STRONG_WARNING,_LOG::out_s);		
		std::optional<size_t> nextBufferPart =  (tbegin.length() <= finalBuffer.size())?specialSearch(0,finalBuffer,std::vector<byte>(tbegin.begin(),tbegin.end())):
			specialSearch(0,std::vector<byte>(tbegin.begin(),tbegin.end()),finalBuffer);
		if ((!nextBufferPart) || ((diff != 0) && (diff != finalBuffer.size())))
		{
			_LOG::out_s << "AFTER_CLEAR::finalBuffer.size() = " << finalBuffer.size() << " ";
			for (size_t i = 0 ; i < finalBuffer.size() ; ++i)
				_LOG::out_s << "fb[" << i << "](" << finalBuffer[i]  << "),";	 
			_LOG::out_s << std::endl;
		}
   		#endif 		 
		}
        if (SIZE_OF_BUFFER == total)
			finalBuffer.clear();
        else
            clear(finalBuffer,beg,end);    
 		return ;
	}
};


void handle_signal(int sig)
{

	if (sig == SIGINT) {

		LOGTOFILE(LOGHTML::messageType::STRONG_WARNING,"SIGINT deteced try to stop execution.");
		pidBlocking *blocker  = Factory<pidBlocking>::getInstance();
		LOGTOFILE(LOGHTML::messageType::STRONG_WARNING,"try to destroy pid file...");
		running = false;
        try
		{
			blocker->destroyPidFile();
		}
		catch(const std::exception& e)
		{
			_LOG::out_s << "SIGINT HANDLING EXCEPTION:" 	<< e.what() << std::endl;
			LOGTOFILE(LOGHTML::messageType::STRONG_WARNING,_LOG::out_s);
		}		
		signal(SIGINT, SIG_DFL);
		}
	else 
		if (sig == SIGHUP) {
			LOGTOFILE(LOGHTML::messageType::STRONG_WARNING,"try to reload configuration file...");
		config *conf = Factory<config>::getInstance();
		reconfig(conf);

	} else if (sig == SIGCHLD) {
		//fprintf(log_stream, "Debug: received SIGCHLD signal\n");
		LOGTOFILE(LOGHTML::messageType::STRONG_WARNING,"Debug: received SIGCHLD signal...");

	}
}

int serialDataAvail (const int fd)
{
    int result = 0;
    if (ioctl (fd, FIONREAD, &result) == -1)
        return -1 ;
    return result ;
}


size_t getUartDataLoop(const config& conf)
{
	size_t total = 0;
	int szData = 0;
	u_chk  _chk;
	std::vector<byte> finalBuffer;
	std::vector<byte> buffer;
	const std::vector<byte> PILOT = conf.getPilot();
	const std::vector<byte> total_end = conf.getFinaliser();
	bool starterOn = false;
	bool finalise = false;
	size_t n_ = 0;
	buffer.resize(10000);
	finalBuffer.reserve(TOTAL_BUFFER_RESERVATION * 2);
	//int serial = serialOpen( conf.getDevice().c_str(),2000000);
    boost::asio::io_service io;
    boost::asio::serial_port stream(io,"/dev/ttyAMA0");
    stream.set_option(boost::asio::serial_port_base::baud_rate(2000000));
    stream.set_option(boost::asio::serial_port_base::character_size(8));
    stream.set_option(boost::asio::serial_port_base::stop_bits(boost::asio::serial_port_base::stop_bits::two));
    stream.set_option(boost::asio::serial_port_base::flow_control(boost::asio::serial_port_base::flow_control::none));
    stream.set_option(boost::asio::serial_port_base::parity(boost::asio::serial_port_base::parity::none));

	FILE *fstream = fopen(conf.outFileName().c_str(),"wb+");
	if (NULL == fstream)
	{
		_LOG::out_s << "can not opened output file :" << conf.outFileName() << std::endl;
		throw (std::runtime_error(_LOG::out_s.str()));
	}
	running = true;
	while(running)
	{
        boost::system::error_code ec;
	    szData = serialDataAvail(stream.native_handle());
	    if (szData == 0)
			continue;
	    if (buffer.size()< szData)
			buffer.resize(szData * 2);
	    size_t readed =  boost::asio::read(stream,boost::asio::buffer(buffer),
            boost::asio::transfer_at_least(szData),ec);// read(serial,&buffer[0],szData);
	    finalBuffer.insert(finalBuffer.end(),&buffer[0],&buffer[readed]);
		if (!starterOn)
		{
			std::vector<byte>::iterator It = std::search(finalBuffer.begin(),finalBuffer.end(),PILOT.begin(),PILOT.end());
			if (finalBuffer.end() != It)
			{
				starterOn = true;
				finalBuffer.erase(It,It+PILOT.size());
				_LOG::out_s << "PILOT detected ... finalBuffer.size() = " << finalBuffer.size() << std::endl;
				LOGTOFILE(LOGHTML::messageType::STRONG_WARNING,_LOG::out_s);	
			}
		}
		if (!starterOn)
			continue;

        bufferUnit bu(finalBuffer,conf);
        if (bu.Ready())
		{
			const std::vector<byte> _data = bu.getData();
			if (_data.empty() )
				continue;
			total += fwrite(_data.data(),1,_data.size(),fstream);		
        }
		std::optional<size_t> it_is_end = specialSearch(0,finalBuffer,std::vector<byte>(total_end.begin(),total_end.end()));
		if (it_is_end)
		{
			_LOG::out_s << "it_is_end = " <<  *it_is_end  << std::endl;
			LOGTOFILE(LOGHTML::messageType::STRONG_WARNING,_LOG::out_s);
			break;	
		}	
	}
	fclose(fstream);
    LOGTOFILE(LOGHTML::messageType::STRONG_WARNING,"FINAL READING LOOP");
	return (total);
}
void reconfig(config *conf)
{
	conf->load("config.json");
	std::string logPath = conf->getLogPath();
	boost::filesystem::path _logPath = boost::filesystem::path(logPath);
	if (!boost::filesystem::is_directory(_logPath))
	{
		if (mkdir(logPath.c_str(),0777) == -1)
				_LOG::out_s << "cant't create logPath:" << logPath << std::endl;
			else
				_LOG::out_s << "Ok create logPath:" 	<< logPath << std::endl;
	}
	LOGTOFILE(LOGHTML::messageType::STRONG_WARNING,_LOG::out_s);
	_LOG::logFileName = logPath + std::string ("/console.log");
	_LOG::out_s << "_LOG::logFileName = " << _LOG::logFileName << std::endl;
	LOGTOFILE(LOGHTML::messageType::STRONG_WARNING,_LOG::out_s);
}

int main(void)
{

	//todo:Singleton для config-а
	std::string pidPath = "/run/wiring/wiring.pid";
	Factory<pidBlocking>::Create();
	Factory<config>::Create();
	pidBlocking *blocker  = Factory<pidBlocking>::getInstance();
	config *conf = Factory<config>::getInstance();
	reconfig(conf);
	std::cout << "starting daemon..." << std::endl;

		

	_LOG::out_s << "boost_version = " << BOOST_VERSION /100000 << "." << BOOST_VERSION / 100 % 1000 << "." 
	<< BOOST_VERSION % 100 << std::endl;
	
	if (conf->asDaemon())
	{	
		demonize();
		signal(SIGINT, handle_signal);
		signal(SIGHUP, handle_signal);
	}
	try
	{
		blocker->createPidFile(pidPath);
	}
	catch(const std::exception& e)
	{
		_LOG::out_s << "Debug:" << e.what() << '\n';
		LOGTOFILE(LOGHTML::messageType::STRONG_WARNING,_LOG::out_s);
		exit(EXIT_FAILURE);
	}
	if (!blocker->lock())
	{
		LOGTOFILE(LOGHTML::messageType::STRONG_WARNING,std::string("detected locked pid file ")+pidPath+std::string(" ") );
		return  (EXIT_FAILURE);
	}
	blocker->setPid();
	LOGTOFILE(LOGHTML::messageType::STRONG_WARNING,std::string("Starting daemon"));
	size_t total = 0;
	
	//signal(SIGINT, handle_signal);
	//signal(SIGHUP, handle_signal);
	auto start = std::chrono::steady_clock::now();
	try
	{
		total = getUartDataLoop(*conf);
	}
	catch(const std::exception& e)
	{
	LOGTOFILE(LOGHTML::messageType::STRONG_WARNING,std::string(e.what()));
	}
	
	auto end = std::chrono::steady_clock::now();

	auto seconds = std::chrono::duration_cast<std::chrono::seconds>(end-start);
		_LOG::out_s  << std::endl << "size = " << total << std::endl 
		  <<  "Elapsed time = " << seconds.count() << std::endl
		  << "Average speed:"   << total / seconds.count() 
		  << std::endl;
	LOGTOFILE(LOGHTML::messageType::STRONG_WARNING,_LOG::out_s);
	blocker->destroyPidFile();
	Factory<pidBlocking>::Destroy();
	return EXIT_SUCCESS;
}
