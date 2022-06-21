#include "StdAfx.h"

#include "PBFTNodeFactory.h"


PBFTNodeFactory::PBFTNodeFactory(DigitalSignatureManager::Ptr pSignatureManager) : NodeFactory(pSignatureManager)
{
}


PBFTNodeFactory::~PBFTNodeFactory()
{
}


Node::Ptr PBFTNodeFactory::RunNewNode()
{
	ByteVector vbNewPrivateKey = m_pSignatureManager->GeneratePrivateKey();
	m_vPublicKeys.push_back(m_pSignatureManager->ComputePublicKey(vbNewPrivateKey));
	m_vpNodes.emplace_back(new PBFTNode(m_vpNodes.size(), m_pNetwork, m_pSignatureManager, vbNewPrivateKey, m_vPublicKeys));

	m_pNetwork->RegisterClient(m_vpNodes.size() - 1);

	std::thread newNodeThread(*m_vpNodes.back());
	newNodeThread.detach();

	return m_vpNodes.back();
}