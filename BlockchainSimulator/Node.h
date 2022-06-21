#pragma once

#include "NetworkManager1.h"
#include "Blockchain.h"
#include "DigitalSignatureManager.h"
#include "SimpleDataPackage.h"

extern double INTERVAL_AVERAGE;


//
// Base class for all nodes in network
//
class Node
{
public:
	typedef std::shared_ptr<Node> Ptr;

	Node(unsigned int id, 
		NetworkManager1::Ptr pNetwork, 
		DigitalSignatureManager::Ptr pSignatureManager, 
		ByteVector vbPrivateKey,
		std::vector<ByteVector>& const vbPublicKeys);
	~Node();

	//
	// Entry point
	//
	void virtual operator()();

	//
	// Prints hashes of all commited blocks in all inheritants and all uncommited in this class
	// (used for testing)
	//
	void virtual PrintSavedBlocks() const;

	size_t virtual GetPoolSize() const;

	//
	// Returnes the number of blocks in this node's chain
	//
	size_t virtual GetStoredBlocksNumber() const;

	//
	// Signals tha work of this node should be stopped
	//
	void virtual Stop();

protected:
	//
	// Listens and handles all new messages in the network
	// Here is the starting point for all protocols' logic (realized by inheritants)
	//
	virtual void						ListenMessages();

	//
	// Adds a new block to the chain (realized by inheritants)
	//
	virtual void						AddNewBlock();


	unsigned int						m_id;
	NetworkManager1::Ptr				m_pNetwork;
	std::vector<ByteVector>&			m_vvbPublicKeys;
	std::shared_ptr<std::mutex>			m_poolMutex;	// Controls the access to the message pool
	std::shared_ptr<Blockchain>			m_chain;
	std::shared_ptr<std::atomic<bool>>	m_bStopped;
	DigitalSignatureManager::Ptr		m_pSignatureManager;
	ByteVector							m_vbPrivateKey;

private:
	std::shared_ptr<std::vector<SimpleDataPackage*>>	m_transactionPool;
};

