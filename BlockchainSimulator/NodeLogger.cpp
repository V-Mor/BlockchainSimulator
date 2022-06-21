#include "StdAfx.h"

#include "NodeLogger.h"


NodeLogger::NodeLogger(unsigned int uiNumber)
{
	m_logFileName = "logfile" + std::to_string(uiNumber) + ".log";
}


NodeLogger::~NodeLogger()
{
	/*std::lock_guard<std::mutex> fileGuard(m_fileMutex);

	std::ofstream logFile(m_logFileName, std::ios::out | std::ios::trunc);

	logFile.close();*/
}


void NodeLogger::LogMessage(std::string message)
{
	// Logger is disabled to save resources

	/*std::lock_guard<std::mutex> fileGuard(m_fileMutex);

	std::ofstream logFile(m_logFileName, std::ios::out | std::ios::app);
	logFile.write((message + "\n").c_str(), message.size() + 1);

	logFile.close();*/
}
