#pragma once

#include "DataPackage.h"
#include "sha256.h"


//
// Represents the simplest possible data packet containing a string value
//
struct SimpleDataPackage : public DataPackage
{
	typedef std::shared_ptr<SimpleDataPackage> Ptr;

	SimpleDataPackage(unsigned int sender,
					unsigned int destination,
					std::string message) : 
		message(message),
		DataPackage(sender, destination)
	{
	}

	ByteVector GetRawData()
	{
		ByteVector data;
		data.assign(message.c_str(), message.c_str() + message.size());

		return data;
	}

	std::string message;
};