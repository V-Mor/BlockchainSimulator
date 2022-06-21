#include "StdAfx.h"

#include "SBFTDataPackage.h"


SBFTDataPackage::SBFTDataPackage(unsigned int sender,
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
// Все данные, имеющиеся в сообщении, превращаются в байты путём сдвигов и 
// наложения битовой маски и добавляются в общий вектор
//
ByteVector SBFTDataPackage::GetRawData()
{
	ByteVector vbTotalData;

	for (size_t i = 0; i < sizeof(unsigned int); ++i)
	{
		// Маска для каждого байта данных получается путём сдвига 0xff на нужное количество байт влево
		// После наложения маски итог сдвигается на то же кол-во байт вправо, чтобы получить нужное значение без лишних нулей
		// Пример:
		// Пусть есть int 0x11223344
		// Чтобы получить первый байт, 0xff двигается на три байта влево, получается 0xff000000
		// Накладывается маска, получается 0x11000000
		// Результат двигается обратно на столько же байт, получается искомый байт 0x11
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

	// Клиентский запрос не включается в расчёт общих данных, т.к. имеет собственную подпись и
	// прикрепляется к сообщению pre-prepare отдельно, т.е. не подписывается вместе с ним. 
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
