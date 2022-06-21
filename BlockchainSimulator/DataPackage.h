#pragma once

#include "DigitalSignatureManager.h"
#include "sha256.h"


//
// Represents the base class for the sent data package
// Has only three main attributes for each packet:
// 1. Sender
// 2. Reciever
// 3. Signature
//
struct DataPackage
{
	typedef std::shared_ptr<DataPackage> Ptr;

	DataPackage(unsigned int sender,
				unsigned int destination) :
		uiSender(sender),
		uiDestination(destination)
	{
	}

	unsigned int uiSender;
	unsigned int uiDestination;
	ByteVector vbSign;

	//
	// Gets all the packet data serialized into bytes
	//
	virtual ByteVector GetRawData() = 0;

	//
	// Gets the hash from the packet data
	//
	ByteVector GetDataHash()
	{
		return sha256(GetRawData());
	}
};

