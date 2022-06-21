#include "StdAfx.h"

#include "Block.h"
#include "sha256.h"


Block::Block(uint32_t nIndexIn, std::vector<std::string> dataIn) : m_nIndex(nIndexIn), m_data(dataIn)
{
	m_nNonce = -1;
	m_tTime = time(nullptr);
	m_vbHash = CalculateHash();
}


ByteVector Block::CalculateHash() const
{
	ByteVector hashes;
	for (const std::string& dataChunk : m_data)
	{
		ByteVector data;
		data.assign(dataChunk.c_str(), dataChunk.c_str() + dataChunk.size());
		ByteVector dataChunkHash = sha256(data);
		hashes.insert(hashes.end(), dataChunkHash.begin(), dataChunkHash.end());
	}

	return sha256(hashes);
}


void Block::MineBlock(uint32_t nDifficulty) 
{
	m_vbHash = CalculateHash();
}


ByteVector Block::GetHash()
{
	return m_vbHash;
}


uint32_t Block::GetIndex()
{
	return m_nIndex;
}

