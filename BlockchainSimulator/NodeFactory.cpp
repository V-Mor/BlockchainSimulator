#include "StdAfx.h"

#include "NodeFactory.h"


NodeFactory::NodeFactory(DigitalSignatureManager::Ptr pSignatureManager) : m_pSignatureManager(pSignatureManager)
{
	m_pNetwork.reset(new NetworkManager1());
}


NodeFactory::~NodeFactory()
{
}


Node::Ptr NodeFactory::RunNewNode()
{
	// Key pair for the node is generated
	ByteVector vbNewPrivateKey = m_pSignatureManager->GeneratePrivateKey();
	// Public key is added to the common pool. Key's index in vector is equal to the ID of the node which owns it
	m_vPublicKeys.push_back(m_pSignatureManager->ComputePublicKey(vbNewPrivateKey));
	// The new node is created
	m_vpNodes.emplace_back(new Node(m_vpNodes.size(), m_pNetwork, m_pSignatureManager, vbNewPrivateKey, m_vPublicKeys));
	// Node is registered in network
	m_pNetwork->RegisterClient(m_vpNodes.size() - 1);

	// Created node is runned in separated thread
	std::thread newNodeThread(*m_vpNodes.back());
	newNodeThread.detach();

	return m_vpNodes.back();
}
