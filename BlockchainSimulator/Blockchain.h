#pragma once

#include "Block.h"

class Blockchain 
{
public:
    Blockchain();

    void AddBlock(Block bNew);
    [[nodiscard]] Block GetLastBlock() const;
    std::vector<Block>  GetChain() const;

private:

    uint32_t            m_nDifficulty;  // It is a complexity
    std::vector<Block>  m_vChain;       // this vector stores the chain of the blocks
};
