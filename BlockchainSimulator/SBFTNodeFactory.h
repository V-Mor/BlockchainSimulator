#pragma once

#include "NetworkManager.h"
#include "DigitalSignatureManager.h"
#include "NodeFactory.h"
#include "SBFTNode.h"


//
// Работает аналогично родительскому классу, за тем исключением, что 
// вместо Node создаются и запускаются объекты SBFTNode
//
class SBFTNodeFactory : NodeFactory
{
public:
	SBFTNodeFactory(DigitalSignatureManager::Ptr pSignatureManager);
	~SBFTNodeFactory();

	Node::Ptr RunNewNode();

private:
	std::vector<SBFTNode::Ptr>	m_vpNodes;
};

