#include "StdAfx.h"

#include "ECDSASignatureManager.h"


ByteVector ECDSASignatureManager::Sign(ByteVector vbMessage, ByteVector vbPrivateKey)
{
	ECDSAPrivateKey privateKey = PrivateKeyFromBytes(vbPrivateKey);

	CryptoPP::ECDSA<CryptoPP::ECP, CryptoPP::SHA1>::Signer signer(privateKey);
	size_t uiSigLen = signer.MaxSignatureLength();
	ByteVector vbSign(uiSigLen);
	uiSigLen = signer.SignMessage(m_prng, vbMessage.data(), vbMessage.size(), vbSign.data());
	vbSign.resize(uiSigLen);

	return vbSign;
}


bool ECDSASignatureManager::Verify(ByteVector vbMessage, ByteVector vbSignature, ByteVector vbPublicKey)
{
	ECDSAPublicKey publicKey = PublicKeyFromBytes(vbPublicKey);

	CryptoPP::ECDSA<CryptoPP::ECP, CryptoPP::SHA1>::Verifier verifier(publicKey);
	return verifier.VerifyMessage(vbMessage.data(), vbMessage.size(), vbSignature.data(), vbSignature.size());
}


ByteVector ECDSASignatureManager::GeneratePrivateKey()
{
	ECDSAPrivateKey newPrivKey;
	newPrivKey.Initialize(m_prng, CryptoPP::ASN1::secp256r1());

	return PrivateKeyToBytes(newPrivKey);
}


ByteVector ECDSASignatureManager::ComputePublicKey(ByteVector vbPrivateKey)
{
	ECDSAPrivateKey privateKey = PrivateKeyFromBytes(vbPrivateKey);

	ECDSAPublicKey publicKey;
	privateKey.MakePublicKey(publicKey);

	return PublicKeyToBytes(publicKey);
}


ECDSASignatureManager::ECDSAPrivateKey ECDSASignatureManager::PrivateKeyFromBytes(ByteVector& const vbPrivateKeyBytes) const
{
	ECDSAPrivateKey privateKey;
	CryptoPP::ArraySource arraySource(&vbPrivateKeyBytes[0], vbPrivateKeyBytes.size(), true);
	privateKey.BERDecode(arraySource);

	return privateKey;
}


ByteVector ECDSASignatureManager::PrivateKeyToBytes(ECDSAPrivateKey& const privateKey) const
{
	// Private key is always 67 bytes long
	ByteVector vbPrivateKeyBytes(67);
	CryptoPP::ArraySink arraySink(&vbPrivateKeyBytes[0], vbPrivateKeyBytes.size());
	privateKey.DEREncode(arraySink);

	return vbPrivateKeyBytes;
}


ECDSASignatureManager::ECDSAPublicKey ECDSASignatureManager::PublicKeyFromBytes(ByteVector& const vbPublicKeyBytes) const
{
	ECDSAPublicKey publicKey;
	CryptoPP::ArraySource arraySource(&vbPublicKeyBytes[0], vbPublicKeyBytes.size(), true);
	publicKey.BERDecode(arraySource);

	return publicKey;
}


ByteVector ECDSASignatureManager::PublicKeyToBytes(ECDSAPublicKey& const publicKey) const
{
	// Public key is always 91 bytes long
	ByteVector vbPublicKeyBytes(91);
	CryptoPP::ArraySink arraySink(&vbPublicKeyBytes[0], vbPublicKeyBytes.size());
	publicKey.DEREncode(arraySink);

	return vbPublicKeyBytes;
}
