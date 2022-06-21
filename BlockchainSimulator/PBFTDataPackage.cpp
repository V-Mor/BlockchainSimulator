#include "StdAfx.h"

#include "PBFTDataPackage.h"


PBFTDataPackage::PBFTDataPackage(unsigned int sender,
								unsigned int destination,
								MessageType type) :
	DataPackage(sender, destination),
	type(type),
	pBlock(nullptr),
	pTimestamp(nullptr),
	puiNodeId(nullptr),
	puiViewNumber(nullptr),
	puiSequenceNumber(nullptr),
	pClientRequest(nullptr)    
{
}

//
// All data in message are converted to raw bytes by using shifts 
// and bit mask and are insluded to total vector
//
ByteVector PBFTDataPackage::GetRawData()
{
	ByteVector vbTotalData;

	for (size_t i = 0; i < sizeof(unsigned int); ++i)
	{
		// The mask for every byte is obtained by shifting 0xff value to the left
		// Then the resulting value is shifted to the rigth to make it correct
		int shift = (sizeof(unsigned int) - 1 - i) * 8;
		vbTotalData.push_back((uiSender & (0xff << shift)) >> shift);
	}

	for (size_t i = 0; i < sizeof(unsigned int); ++i)
	{
		int shift = (sizeof(unsigned int) - 1 - i) * 8;
		vbTotalData.push_back((uiDestination & (0xff << shift)) >> shift);
	}

	for (size_t i = 0; i < sizeof(MessageType); ++i)
	{
		int shift = (sizeof(unsigned int) - 1 - i) * 8;
		vbTotalData.push_back((static_cast<unsigned int>(type) & (0xff << shift)) >> shift);
	}

	if (pBlock)
	{
		ByteVector vbBlockHash = pBlock->GetHash();
		vbTotalData.insert(vbTotalData.end(), vbBlockHash.begin(), vbBlockHash.end());
	}

	if (pTimestamp)
	{
		long long llTimeStamp = pTimestamp->time_since_epoch().count();
		for (size_t i = 0; i < sizeof(long long); ++i)
		{
			int shift = (sizeof(long long) - 1 - i) * 8;
			vbTotalData.push_back((llTimeStamp & (0xffll << shift)) >> shift);
		}
	}

	if (puiNodeId)
	{
		for (size_t i = 0; i < sizeof(unsigned int); ++i)
		{
			int shift = (sizeof(unsigned int) - 1 - i) * 8;
			vbTotalData.push_back((*puiNodeId & (0xff << shift)) >> shift);
		}
	}

	if (puiViewNumber)
	{
		for (size_t i = 0; i < sizeof(unsigned int); ++i)
		{
			int shift = (sizeof(unsigned int) - 1 - i) * 8;
			vbTotalData.push_back((*puiViewNumber & (0xff << shift)) >> shift);
		}
	}

	if (puiSequenceNumber)
	{
		for (size_t i = 0; i < sizeof(unsigned int); ++i)
		{
			int shift = (sizeof(unsigned int) - 1 - i) * 8;
			vbTotalData.push_back((*puiSequenceNumber & (0xff << shift)) >> shift);
		}
	}

	// Client's request is not included to total data because it has its ows signature
	// and is concatenated to pre-prepare message separately, i.e. is not signed along with it. 
	/*if (pClientRequest)
	{
		ByteVector vbRequestHash = pClientRequest->GetDataHash();
		vbTotalData.insert(vbTotalData.end(), vbRequestHash.begin(), vbRequestHash.end());
	}*/

	if (!message.empty())
	{
		ByteVector vbMessageData;
		vbMessageData.assign(message.c_str(), message.c_str() + message.size());

		vbTotalData.insert(vbTotalData.end(), vbMessageData.begin(), vbMessageData.end());
	}

	return vbTotalData;
}
