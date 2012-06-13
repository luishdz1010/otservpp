#include "crypto.h"
#include <openssl/rsa.h>
#include <openssl/bn.h>
#include <openssl/err.h>
#include "lambdautil.hpp"

namespace otservpp { namespace crypto{

boost::asio::io_service::id CryptoService::id;

ErrorCategory::ErrorCategory()
{
	static struct Init{
		Init(){ ERR_load_crypto_strings(); }
	} init;
}

const char* ErrorCategory::name() const
{
	return "crypto";
}

std::string ErrorCategory::message(int e) const
{
	return ERR_error_string(e, nullptr);
}

const boost::system::error_category& errorCategory()
{
	static ErrorCategory c;
	return c;
}

uint32_t adler32(uint8_t* data, int32_t len)
{
	assert(len != 0);

	uint_fast32_t a = 1, b = 0;
	int k;
	while(len > 0)
	{
		k = len > 5552? 5552 : len;
		len -= k;

		while(k >= 16){
			a += *data++; b += a;
			a += *data++; b += a;
			a += *data++; b += a;
			a += *data++; b += a;
			a += *data++; b += a;
			a += *data++; b += a;
			a += *data++; b += a;
			a += *data++; b += a;
			a += *data++; b += a;
			a += *data++; b += a;
			a += *data++; b += a;
			a += *data++; b += a;
			a += *data++; b += a;
			a += *data++; b += a;
			a += *data++; b += a;
			a += *data++; b += a;
			k -= 16;
		}

		if(k != 0){
			do{
				a += *data++; b += a;
			} while (--k);
		}

		a %= 65521;
		b %= 65521;
	}

	return (uint32_t)(b << 16) | a;
}

Xtea::Xtea(uint32_t k0, uint32_t k1, uint32_t k2, uint32_t k3) :
	key{k0, k1, k2, k3}
{}

void Xtea::decrypt(uint32_t* buffer, std::size_t lenght) const
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

void Xtea::encrypt(uint32_t* buffer, std::size_t lenght) const
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


Rsa::Rsa(boost::asio::io_service& ioService,
		 const char* n, const char* e, const char* d, const char* p, const char* q) :
	rsa(RSA_new()),
	svc(boost::asio::use_service<CryptoService>(ioService))
{
	int ok = BN_dec2bn(&rsa->n, n)?
			(BN_dec2bn(&rsa->e, e)?
			(BN_dec2bn(&rsa->d, d)?
			(BN_dec2bn(&rsa->p, p)?
			 BN_dec2bn(&rsa->q, q):0):0):0):0;

	if(!ok || RSA_check_key(rsa) < 1){
		RSA_free(rsa);
		throw boost::system::system_error(static_cast<int>(ERR_get_error()), errorCategory());
	}
}

Rsa::~Rsa()
{
	RSA_free(rsa);
}

int Rsa::decrypt(uint8_t* buffer, std::size_t length, boost::system::error_condition& e)
{
	int newLen = RSA_private_decrypt((int)length, buffer, buffer, rsa, RSA_NO_PADDING);

	if(newLen == -1)
		e.assign(static_cast<int>(ERR_get_error()), errorCategory());

	return newLen;
}

} /* namespace crypto */
} /* namespace otservpp */
