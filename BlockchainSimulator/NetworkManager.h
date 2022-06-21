#pragma once

#include "DataPackage.h"


// Old network manager. Is not used now. 


class NetworkManager
{
public:
	typedef std::shared_ptr<NetworkManager> Ptr;

	NetworkManager();
	~NetworkManager();

	std::shared_future<DataPackage*>GetData();
	void							PostData(DataPackage* data);

private:
	std::mutex							m_queueMutex;
	std::mutex							m_futureMutex;
	std::promise<DataPackage *>			m_dataPromice;
	std::shared_future<DataPackage *>	m_dataFuture;
};

