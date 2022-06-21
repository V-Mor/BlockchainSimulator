#pragma once

#include "StdAfx.h"

#include <cryptopp/config.h>

//
// Base class representing an interface for work with electronic signatures
//
class DigitalSignatureManager
{
public:
	typedef std::shared_ptr<DigitalSignatureManager> Ptr;

	virtual ByteVector	Sign(ByteVector vbMessage, ByteVector vbPrivateKey) = 0;
	virtual bool		Verify(ByteVector vbMessage, ByteVector vbSignature, ByteVector vbPublicKey) = 0;
	virtual ByteVector	GeneratePrivateKey() = 0;
	virtual ByteVector	ComputePublicKey(ByteVector vbPrivateKey) = 0;
};

