#include "crypto.h"
#include <openssl/rsa.h>
#include <openssl/bn.h>
#include <openssl/err.h>

namespace otservpp { namespace crypto{

Xtea::Xtea(uint32_t k0, uint32_t k1, uint32_t k2, uint32_t k3) :
	key{k0, k1, k2, k3}
{}

void Xtea::decrypt(uint32_t* buffer, std::size_t lenght)
{
	uint32_t k[4] {key[0], key[1], key[2], key[3]};

	uint pos = 0, round;
	uint32_t v0, v1, sum, delta;

	while(pos < lenght/4){
		v0 = buffer[pos];
		v1 = buffer[pos+1];
		delta = 0x9E3779B9;
		sum = delta << 5;

		round = 32;
		while(round-- > 0){
			v1 -= ((v0 << 4 ^ v0 >> 5) + v0) ^ (sum + k[sum >> 11 & 3]);
			sum -= delta;
			v0 -= ((v1 << 4 ^ v1 >> 5) + v1) ^ (sum + k[sum & 3]);
		}

		buffer[pos] = v0;
		buffer[pos+1] = v1;
		pos += 2;
	}
}

void Xtea::encrypt(uint32_t* buffer, std::size_t lenght)
{
	uint32_t k[4] {key[0], key[1], key[2], key[3]};

	uint pos = 0, round;
	uint32_t v0, v1, sum;

	while(pos < lenght/4){
		v0 = buffer[pos];
		v1 = buffer[pos+1];
		sum = 0;

		round = 32;
		while(round-- > 0){
			v0 += ((v1 << 4 ^ v1 >> 5) + v1) ^ (sum + k[sum & 3]);
			sum += 0x9E3779B9;
			v1 += ((v0 << 4 ^ v0 >> 5) + v0) ^ (sum + k[sum >> 11 & 3]);
		}

		buffer[pos] = v0;
		buffer[pos+1] = v1;
		pos += 2;
	}
}


Rsa::Rsa(const char* n, const char* e, const char* d, const char* p, const char* q) :
	rsa(RSA_new())
{
	int ok = BN_dec2bn(&rsa->n, n)?
			(BN_dec2bn(&rsa->e, e)?
			(BN_dec2bn(&rsa->d, d)?
			(BN_dec2bn(&rsa->p, p)?
			 BN_dec2bn(&rsa->p, p):0):0):0):0;

	if(!ok || RSA_check_key(rsa) != 0){
		ERR_load_crypto_strings();
		throw std::runtime_error(ERR_error_string(ERR_get_error(), nullptr));
	}
}

Rsa::~Rsa()
{
	RSA_free(rsa);
}

std::size_t Rsa::decrypt(uint8_t* buffer, std::size_t lenght)
{
	int newLen = RSA_private_decrypt(lenght, buffer, buffer, rsa, RSA_NO_PADDING);

	if(newLen == -1)
		throw std::runtime_error(ERR_error_string(ERR_get_error(), nullptr));

	return newLen;
}

} /* namespace crypto */
} /* namespace otservpp */
