#pragma once

#include "NetworkManager1.h"
#include "DigitalSignatureManager.h"
#include "Node.h"


//
// Base class for the node factories
// Realizes work with the simple nodes (class Node)
//
class NodeFactory
{
public:
	NodeFactory(DigitalSignatureManager::Ptr pSignatureManager);
	~NodeFactory();

	//
	// Create, register and run new node
	//
	virtual Node::Ptr RunNewNode();

protected:
	//
	// Associated electronic signature manager
	//
	DigitalSignatureManager::Ptr	m_pSignatureManager;

	//
	// The current network manager
	//
	NetworkManager1::Ptr			m_pNetwork;

	//
	// Public keys of this network participants
	//
	std::vector<ByteVector>			m_vPublicKeys;

private: 
	std::vector<Node::Ptr>			m_vpNodes;
};
