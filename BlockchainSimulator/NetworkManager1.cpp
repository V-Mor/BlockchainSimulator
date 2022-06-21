#include "StdAfx.h"

#include "NetworkManager1.h"


unsigned int const NETWORK_BUFFER_SIZE = 100;


NetworkManager1::NetworkManager1()
{
	
}


NetworkManager1::~NetworkManager1()
{

}


void NetworkManager1::PostData(DataPackage* data)
{
	// Blocks all queue set not to register new clients during writing
	std::lock_guard<std::mutex> queueGuard(m_mainQueueMutex);

	// Looks at every client
	for (std::pair<const unsigned int, WaitingQueue>& clientQueue : m_mainTreeQueue)
	{
		// Blocks current client's queue
		clientQueue.second.dataRWMutex.lock();
		
		// If queue is not overloaded
		if (clientQueue.second.dataQueue.size() <= NETWORK_BUFFER_SIZE)
		{
			// Puts the data
			clientQueue.second.dataQueue.push(data);

			// Signals that available data amount is increased
			clientQueue.second.dataReadySemaphore.release();
		}
		// Releases the queue
		clientQueue.second.dataRWMutex.unlock();
	}	
}


DataPackage* NetworkManager1::GetData(unsigned int uiClientId)
{
	// Signals that one data element is read from the queue
	// If there is no data in queue now, blocks the thread until data appears
	m_mainTreeQueue.at(uiClientId).dataReadySemaphore.acquire();
	// If some data is presented, blocks reading client's queue not to write during reading
	m_mainTreeQueue.at(uiClientId).dataRWMutex.lock();

	// Saves the result and pop it from the queue
	DataPackage* result = m_mainTreeQueue.at(uiClientId).dataQueue.front();
	m_mainTreeQueue.at(uiClientId).dataQueue.pop();

	// Unlocks the client's queue
	m_mainTreeQueue.at(uiClientId).dataRWMutex.unlock();

	return result;
}


void NetworkManager1::RegisterClient(unsigned int uiClientId)
{
	// Blocks data adding to the whole queue set
	std::lock_guard<std::mutex> queueGuard(m_mainQueueMutex);
	// Creates an empty queue for the new client
	m_mainTreeQueue[uiClientId];
}


size_t NetworkManager1::GetClientQueueLength(unsigned int uiClientId)
{
	return m_mainTreeQueue.at(uiClientId).dataQueue.size();
}