#pragma once

#include <fstream>

class NodeLogger
{
public:
	NodeLogger(unsigned int uiNumber);
	~NodeLogger();

	void LogMessage(std::string message);

private:
	std::mutex		m_fileMutex;
	unsigned int	m_uiFileNumber;
	std::string		m_logFileName;
};

