#pragma once

#include "SimpleDataPackage.h"

class Block 
{
public:
	Block(uint32_t nIndexIn, std::vector<std::string> dataIn);

	ByteVector	GetHash();
	uint32_t	GetIndex();
	void		MineBlock(uint32_t nDifficulty);

	std::vector<std::string> GetData()
	{
		return m_data;
	}

	ByteVector sPrevHash;

private:
	[[nodiscard]] ByteVector   CalculateHash() const;

	uint32_t					m_nIndex;
	int64_t						m_nNonce;
	std::vector<std::string>	m_data;
	ByteVector					m_vbHash;
	time_t						m_tTime;
};

