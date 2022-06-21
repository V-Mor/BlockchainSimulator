#pragma once

#include "NetworkManager.h"
#include "DigitalSignatureManager.h"
#include "NodeFactory.h"
#include "SBFTNode.h"


//
// �������� ���������� ������������� ������, �� ��� �����������, ��� 
// ������ Node ��������� � ����������� ������� SBFTNode
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

