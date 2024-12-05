#include <boost/json/src.hpp>
#include <fstream>
#include <sstream>
#include <iostream>

typedef unsigned char byte;
const std::string YES = "YES";
struct config 
{
    private:
std::string 	devicelink;
std::string 	fileName;	
std::string 	logFilePath;
std::string     _asDaemon;
std::vector<byte> _begin;
std::vector<byte> _end;
std::vector<byte> _pilot;
std::vector<byte> _finaliser;

    std::vector<byte> splitter(std::string str,char delimiter)
    {
        std::istringstream split(str);
        std::vector<std::string> tokens;
        std::vector<byte> v_out;
        for (std::string each; std::getline(split, each, delimiter); tokens.push_back(each));
        std::transform(tokens.begin(), tokens.end(), std::back_inserter(v_out), [](std::string hex_) 
        {
            int num;
            sscanf(hex_.c_str(), "%x", &num);
            return (num);
        });
        return (v_out);
    }

    public:
    void load(std::string configfile)
    {
        std::ifstream readedfile(configfile);
        if (!readedfile.is_open())
        throw(std::runtime_error("can't open config file "+configfile));
        std::string json;
        char tmp[100];
        memset(tmp, 0, 100);
        while (readedfile.getline(tmp, 100))
        {
            auto count_ = strlen(tmp);
            std::copy(&tmp[0], &tmp[count_], std::back_inserter(json));
        } 

        boost::json::value tokens = boost::json::parse(json).at("tokens");
        devicelink 	= boost::json::value_to<std::string>(tokens.at("device_link"));
        fileName 	= boost::json::value_to<std::string>(tokens.at("outFileName"));
        logFilePath = boost::json::value_to<std::string>( tokens.at("logFilePath") ); 
        std::string _strbeg = boost::json::value_to<std::string>(tokens.at("begin"));
        std::string _strend = boost::json::value_to<std::string>(tokens.at("end"));
        std::string _strplt = boost::json::value_to<std::string>(tokens.at("PILOT"));
        std::string _strfin = boost::json::value_to<std::string>(tokens.at("FINALISATOR"));
        _asDaemon = boost::json::value_to<std::string>(tokens.at("AsDaemon"));
        _begin = splitter(_strbeg,',');
        _end   = splitter(_strend,','); 
        _pilot = splitter(_strplt,',');
        _finaliser = splitter(_strfin,',');
    }

    std::string getLogPath(void) const
    {
        std::string ret = logFilePath;
        std::cout << "logFilePath = " << ret << std::endl;
        return (ret);
    }

    std::string outFileName(void) const
    {
        return (fileName);
    }

    std::string getDevice(void) const
    {
        return (devicelink);
    }

    std::vector<byte> getBegin(void) const
    {
        return (_begin);
    }

    std::vector<byte> getEnd(void) const
    {
        return (_end);
    }

    std::vector<byte> getPilot(void) const
    {
        return (_pilot);
    }

    std::vector<byte> getFinaliser(void) const
    {
        return (_finaliser);
    }

    bool asDaemon(void)
    {
        return (!_asDaemon.compare(YES));
    }
};