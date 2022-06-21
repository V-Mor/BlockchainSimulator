#include "StdAfx.h"

#include "Blockchain.h"


Blockchain::Blockchain()
{
    std::vector<std::string> genesisBlockData{ "Genesis Block" };
    m_vChain.emplace_back(Block(0, genesisBlockData));
    m_nDifficulty = 2;
}


Block Blockchain::GetLastBlock() const 
{
    return m_vChain.back();
}


std::vector<Block>  Blockchain::GetChain() const
{
    return m_vChain;
}


void Blockchain::AddBlock(Block bNew) 
{
    bNew.sPrevHash = GetLastBlock().GetHash();
    bNew.MineBlock(m_nDifficulty);
    m_vChain.push_back(bNew);
}
