#ifndef LOGRECORDER_H
#define LOGRECORDER_H

#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include "nubotdataconfig.h"
#include "targetconfig.h"
#include "Infrastructure/NUBlackboard.h"
#include <boost/filesystem.hpp>

enum LogFileStatus
{
    lf_CLOSED,
    lf_OPEN,
    lf_UNAVAILABLE,
    lf_UNKNOWN
};

// Wrapper for log file writer class. One of these is required for each data type.
class LogFileWriter
{
public:
    LogFileWriter(std::string data_type);
    ~LogFileWriter();

    bool Open(std::string log_path);
    bool Close();

    std::string GetDataType()
    {
        return m_data_name;
    };

    std::string GetFileName()
    {
        return m_file_name;
    };

    LogFileStatus GetFileStatus()
    {
        return m_status;
    }

    std::fstream& GetFile()
    {
        return m_file_stream;
    }

    std::string toString();

private:
    std::string m_data_name;
    std::string m_file_name;
    LogFileStatus m_status;
    std::fstream m_file_stream;

};

class LogRecorder
{

public:
    LogRecorder(int playerNumber);
    ~LogRecorder();
    bool SetLogging(std::string dataType, bool enabled);
    bool WriteData(NUBlackboard* theBlackboard);
    static std::string GetLogPath(int robot_number, std::string data_name)
    {
        const std::string extension = "strm";
        const std::string seperator = ".";
#ifdef TARGET_IS_NAO
        const std::string data_dir = "/var/volatile/";
#else
        const std::string data_dir = std::string(DATA_DIR);
#endif
        if(data_name == "image") data_name = "_" + data_name; // Hack to stop image files getting opened automatically.
        std::stringstream filepath;
        filepath << data_dir << robot_number;
        boost::filesystem::path dir(filepath.str());
        boost::filesystem::create_directories(dir);
        if(boost::filesystem::is_directory(dir))
        {
            filepath << '/';
            std::stringstream filename;
            filename << filepath.str() << data_name << seperator << extension;
            return filename.str();
        }
        else
        {
            std::stringstream filename;
            filename << data_dir << robot_number << "_" << data_name << seperator << extension;
            return filename.str();
        }

    };

private:
    int m_player_number;
    bool HasDataType(std::string dataType);
    LogFileWriter* GetDataWriter(std::string dataType);
    std::vector<LogFileWriter*> m_log_writers;

};

#endif // LOGRECORDER_H
