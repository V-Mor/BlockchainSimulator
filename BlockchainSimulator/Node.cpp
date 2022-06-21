#include "StdAfx.h"

#include "Node.h"
#include <random>


Node::Node(	unsigned int id,
			NetworkManager1::Ptr pNetwork,
			DigitalSignatureManager::Ptr pSignatureManager,
			ByteVector vbPrivateKey,
			std::vector<ByteVector>& const vbPublicKeys):
	m_id(id), 
	m_pNetwork(pNetwork),
	m_vbPrivateKey(vbPrivateKey),
	m_pSignatureManager(pSignatureManager),
	m_vvbPublicKeys(vbPublicKeys),
	m_chain(new Blockchain())
{
	m_bStopped = std::make_shared<std::atomic<bool>>(new std::atomic<bool>());
	m_bStopped->store(false);
	m_poolMutex = std::shared_ptr<std::mutex>(new std::mutex);
	m_transactionPool = std::shared_ptr<std::vector<SimpleDataPackage*>>(new std::vector<SimpleDataPackage*>);

}


Node::~Node()
{

}


void Node::operator()()
{
	// The listening thread starts instantly
	std::thread listenThread(&Node::ListenMessages, this);

	// RNG created (numbers from 1 to 5)
	std::mt19937::result_type seed = time(0);
	auto getRandomInt = std::bind(std::uniform_int_distribution<int>(1, 5), std::mt19937(seed));

	// While the node is not stopped
	while (!m_bStopped->load())
	{
		// Random delay is simulated
		std::this_thread::sleep_for(std::chrono::seconds(getRandomInt()));

		// New data package is generated
		// Address INT_MAX means "send to all"
		SimpleDataPackage* dataToSend{ new SimpleDataPackage(m_id, INT_MAX, "Hello from node with id " + std::to_string(m_id)) };
		// Package is signed
		dataToSend->vbSign = m_pSignatureManager->Sign(dataToSend->GetRawData(), m_vbPrivateKey);

		// Package is sent to the network 
		m_pNetwork->PostData(dataToSend);
	}

	// When the node is stopped, waiting for the listener thread stop
	listenThread.join();
}


void Node::ListenMessages()
{
	// While node is not stopped
	while (!m_bStopped->load())
	{
		// Obtaining data from the network. Data are waited, but still is not ready.
		SimpleDataPackage* data = dynamic_cast<SimpleDataPackage*>(m_pNetwork->GetData(m_id));

		// Signature checking
		if (!m_pSignatureManager->Verify(data->GetRawData(), data->vbSign, m_vvbPublicKeys[data->uiSender]))
		{
			std::cout << "Invalid signature of message from " << data->uiSender << std::endl;
			continue;
		}

		// Adding message to uncommited pool
		std::lock_guard<std::mutex> chainLock(*m_poolMutex);
		m_transactionPool->push_back(data);
	}
}


void Node::PrintSavedBlocks() const
{
	std::lock_guard<std::mutex> chainLock(*m_poolMutex);

	std::cout << "Chain from node with id " << m_id << ":\n";
	for (SimpleDataPackage* record : *m_transactionPool)
		std::cout << record->message << std::endl;
	std::cout << std::endl;
}


size_t Node::GetPoolSize() const
{
	return m_transactionPool->size();
}


size_t Node::GetStoredBlocksNumber() const
{
	return m_chain->GetChain().size();
}


void Node::Stop()
{
	m_bStopped->store(true);
}


void Node::AddNewBlock()
{
	
}