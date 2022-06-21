#include "StdAfx.h"

#include "SBFTNodeFactory.h"


SBFTNodeFactory::SBFTNodeFactory(DigitalSignatureManager::Ptr pSignatureManager) : NodeFactory(pSignatureManager)
{
}


SBFTNodeFactory::~SBFTNodeFactory()
{
}


Node::Ptr SBFTNodeFactory::RunNewNode()
{
	ByteVector vbNewPrivateKey = m_pSignatureManager->GeneratePrivateKey();
	m_vPublicKeys.push_back(m_pSignatureManager->ComputePublicKey(vbNewPrivateKey));
	m_vpNodes.emplace_back(new SBFTNode(m_vpNodes.size(), m_pNetwork, m_pSignatureManager, vbNewPrivateKey, m_vPublicKeys));

	m_pNetwork->RegisterClient(m_vpNodes.size() - 1);

	std::thread newNodeThread(*m_vpNodes.back());
	newNodeThread.detach();

	return m_vpNodes.back();
}