#include "StdAfx.h"
#include "MerkleTree.h"


MerkleTree::MerkleTree(std::shared_ptr<Blockchain> pBlockchain) : m_pBlockchain(pBlockchain)
{
	RebuildTree();
}


void MerkleTree::RebuildTree()
{
	m_leaves.clear();

	std::vector<Block> blocks = m_pBlockchain->GetChain();
	for (Block& block : blocks)
	{
		std::shared_ptr<MerkleTreeNode> leaf{ new MerkleTreeNode };
		leaf->vbSelfHash = block.GetHash();
		m_leaves.push_back(leaf);
	}

	if ((m_leaves.size() % 2) != 0)
	{
		m_leaves.push_back(m_leaves.back());
	}

	std::vector<std::shared_ptr<MerkleTreeNode>> level0;
	std::vector<std::shared_ptr<MerkleTreeNode>> level1 = m_leaves;

	while (level1.size() != 1)
	{
		level0 = level1;
		level1.clear();
		if ((level0.size() % 2) != 0)
			level0.push_back(level0.back());

		for (size_t i = 0; i < level0.size(); i += 2)
		{
			std::shared_ptr<MerkleTreeNode> node{ new MerkleTreeNode };
			node->vbLeftHash = level0[i]->vbSelfHash;
			node->vbRightHash = level0[i + 1]->vbSelfHash;
			ByteVector hashConcat = node->vbLeftHash;
			hashConcat.insert(hashConcat.end(), node->vbRightHash.begin(), node->vbRightHash.end());
			node->vbSelfHash = sha256(hashConcat);

			level0[i]->parent = node;
			level0[i + 1]->parent = node;

			level1.push_back(node);
		}
	}

	m_root = level1.back();
	m_root->bIsRoot = true;
}


ByteVector MerkleTree::GetRootHash()
{
	return m_root->vbSelfHash;
}


std::pair<Block, std::vector<std::shared_ptr<MerkleTree::MerkleTreeNode>>> MerkleTree::GetMerkleProof(size_t uiElementNumber)
{
	std::vector<std::shared_ptr<MerkleTreeNode>> proofChain;
	std::pair<Block, std::vector<std::shared_ptr<MerkleTreeNode>>> proof{ m_pBlockchain->GetChain()[uiElementNumber], proofChain };

	std::shared_ptr<MerkleTreeNode> leaf = m_leaves[uiElementNumber];

	while (!leaf->bIsRoot)
	{
		proof.second.push_back(leaf);
		leaf = leaf->parent;
	}

	return proof;
}
