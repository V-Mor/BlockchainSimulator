#pragma once

#include "NetworkManager.h"
#include "DigitalSignatureManager.h"
#include "NodeFactory.h"
#include "PBFTNode.h"


//
// Works like as parent class, but runs PBFTNodes instead of common Nodes
//
class PBFTNodeFactory : NodeFactory
{
public:
	PBFTNodeFactory(DigitalSignatureManager::Ptr pSignatureManager);
	~PBFTNodeFactory();

	Node::Ptr RunNewNode();

private:
	std::vector<PBFTNode::Ptr>	m_vpNodes;
};

