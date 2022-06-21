#include "StdAfx.h"

#include "NetworkManager.h"


NetworkManager::NetworkManager()
{
	m_dataPromice = std::promise<DataPackage*>();
	m_dataFuture = std::shared_future<DataPackage*>(m_dataPromice.get_future());
}


NetworkManager::~NetworkManager()
{

}


void NetworkManager::PostData(DataPackage* data)
{
	std::lock_guard<std::mutex> queueGuard(m_queueMutex);

	m_dataPromice.set_value(data);

	m_dataPromice = std::promise<DataPackage*>();
	m_futureMutex.lock();
	m_dataFuture = std::shared_future<DataPackage*>(m_dataPromice.get_future());
	m_futureMutex.unlock();

	std::this_thread::sleep_for(std::chrono::milliseconds(10));
}


std::shared_future<DataPackage*> NetworkManager::GetData()
{
	std::lock_guard<std::mutex> getGuard(m_futureMutex);
	return m_dataFuture;
}