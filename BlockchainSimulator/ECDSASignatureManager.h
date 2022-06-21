#pragma once

#include "DigitalSignatureManager.h"

#include <cryptopp/eccrypto.h>
#include <cryptopp/osrng.h>
#include <cryptopp/oids.h>

//
// Represents an object for working with ECDCA electronic signature
//
class ECDSASignatureManager : public DigitalSignatureManager
{
public:
	typedef CryptoPP::ECDSA<CryptoPP::ECP, CryptoPP::SHA1>::PrivateKey	ECDSAPrivateKey;
	typedef CryptoPP::ECDSA<CryptoPP::ECP, CryptoPP::SHA1>::PublicKey	ECDSAPublicKey;

	ByteVector	Sign(ByteVector vbMessage, ByteVector vbPrivateKey);
	bool		Verify(ByteVector vbMessage, ByteVector vbSignature, ByteVector vbPublicKey);
	ByteVector	GeneratePrivateKey();
	ByteVector	ComputePublicKey(ByteVector vbPrivateKey);

private:
	//
	// Keys serializing and deserizlising methods
	//
	ECDSAPrivateKey	PrivateKeyFromBytes(ByteVector& const vbPrivateKeyBytes) const;
	ByteVector		PrivateKeyToBytes(ECDSAPrivateKey& const privateKey) const;
	ECDSAPublicKey	PublicKeyFromBytes(ByteVector& const vbPublicKeyBytes) const;
	ByteVector		PublicKeyToBytes(ECDSAPublicKey& const publicKey) const;

	CryptoPP::AutoSeededRandomPool m_prng;
};

