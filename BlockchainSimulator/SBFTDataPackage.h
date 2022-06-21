#pragma once

#include "DataPackage.h"
#include "Block.h"


//
// Представляет пакет с данными, отправляемый при работе по протоколу SBFT
//
// Содержит ВСЕ атрибуты, которые могут быть у любого пакета в данном протоколе. 
// Нужные атрибуты заполняются и вычитываются в зависимости от MessageType. 
// Атрибуты, не актуальные для данного типа сообщений, устанавливаются равными nullptr.
//
struct SBFTDataPackage : public DataPackage
{
	enum class MessageType : unsigned int {COMMON_MESSAGE, REQUEST, PREPREPARE, SIGN_SHARE, FULL_COMMIT_PROOF, SIGN_STATE, FULL_EXECUTE_PROOF, EXECUTE_ACK};

	SBFTDataPackage(unsigned int sender,
					unsigned int destination,
					MessageType type);
	ByteVector GetRawData();

	MessageType								type;
	std::string								message;
	Block *									pBlock;				// o или r
	std::chrono::time_point<
		std::chrono::high_resolution_clock>*pTimestamp;			// t
	unsigned int *							puiNodeId;			// k
	unsigned int *							puiViewNumber;		// v
	ByteVector								vbMessageDigest;	// proof
	unsigned int *							puiSequenceNumber;	// s
	ByteVector								vbHSign;			// õ_i(h) or п_i(d)
	std::map<unsigned int, ByteVector> *	pSigns;				// õ(h) or п(d)

};

