#pragma once

#include "DataPackage.h"
#include "Block.h"


//
// Represents a data package sent to network while perfirming PBFT protocol
//
// Includes ALL the attributes which can package has in this protocol
// Needed attributes is used according to MessageType value
// Not needed attributes are always set to nullptr value
//
struct PBFTDataPackage : public DataPackage
{
	enum class MessageType : unsigned int {COMMON_MESSAGE, REQUEST, PREPREPARE, PREPARE, COMMIT, REPLY};

	PBFTDataPackage(unsigned int sender,
					unsigned int destination,
					MessageType type);
	ByteVector GetRawData();

	// Variables names from PBFT article (M. Castro and B. Liskov, “Practical Byzantine fault tolerance,” Operating systems review, pp. 173–186, Jan. 1998.) are presented in comments

	MessageType								type;
	std::string								message;
	std::shared_ptr<Block>					pBlock;				// o or r
	std::chrono::time_point<
		std::chrono::high_resolution_clock>*pTimestamp;			// t
	std::shared_ptr<unsigned int>			puiNodeId;			// i
	std::shared_ptr<unsigned int>			puiViewNumber;		// v
	ByteVector								vbMessageDigest;	// d
	std::shared_ptr<PBFTDataPackage>		pClientRequest;		// m
	std::shared_ptr<unsigned int>			puiSequenceNumber;	// n
};

