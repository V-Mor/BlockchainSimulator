#pragma once

#include "DataPackage.h"
#include <semaphore>


//
// Second version of network manager
// More stable, but more resource-consuming
//
class NetworkManager1
{
public:
	typedef std::shared_ptr<NetworkManager1> Ptr;

	NetworkManager1();
	~NetworkManager1();

	//
	// Returns data for the node with particular ID
	//
	DataPackage*	GetData(unsigned int uiClientId);

	//
	// Publishes data package for all nodes
	// (it decides who is a reciever)
	//
	void			PostData(DataPackage* data);

	//
	// Registers a new client Ц initialize a new message queue for it
	//
	void			RegisterClient(unsigned int uiClientId);

	size_t			GetClientQueueLength(unsigned int uiClientId);

private:
	//
	// Structure representing the queue with waiting
	// If queue is empty, current thread is blocked until any data appears
	//
	struct WaitingQueue
	{
		// Semaphore counts the number of elements in queue and realize the waiting process
		std::counting_semaphore<INT_MAX>	dataReadySemaphore;

		// Protect queue from simultaneous reading and writing
		std::mutex							dataRWMutex;

		std::queue<DataPackage*>			dataQueue;

		WaitingQueue(): dataReadySemaphore(0) {}
	};

	//
	// Represents "tree queue" Ц the message published from the one side is retranslated to many other sides
	// можно прочитать с множества концов (по одному на клиента)
	//
	//                                               /---> Client 2 (reads)
	//                                              /
	// Client 1 (publishes) --------->[q][u][e][u][e]-----> Client 1 (reads)
	//                                              \
	//                                               \---> Client 3 (reads)
	//
	std::map<unsigned int, WaitingQueue>	m_mainTreeQueue;
	std::mutex								m_mainQueueMutex;
};

