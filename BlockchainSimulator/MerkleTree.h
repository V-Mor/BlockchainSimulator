#pragma once

#include "sha256.h"
#include "Blockchain.h"


// Merkle tree is not used


class MerkleTree
{
public:
	struct MerkleTreeNode
	{
		bool							bIsRoot{ false };
		ByteVector						vbLeftHash;
		ByteVector						vbRightHash;
		ByteVector						vbSelfHash;

		std::shared_ptr<MerkleTreeNode>	parent{ nullptr };
	};

	MerkleTree(std::shared_ptr<Blockchain> pBlockchain);

	void															RebuildTree();
	ByteVector														GetRootHash();
	std::pair<Block, std::vector<std::shared_ptr<MerkleTreeNode>>>	GetMerkleProof(size_t uiElementNumber);

private:
	std::shared_ptr<Blockchain>						m_pBlockchain;
	std::shared_ptr<MerkleTreeNode>					m_root;
	std::vector<std::shared_ptr<MerkleTreeNode>>	m_leaves;
};

