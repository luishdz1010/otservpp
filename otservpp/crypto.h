#ifndef OTSERVP_CRYPTO_H_
#define OTSERVP_CRYPTO_H_

#include <cstdint>
#include <stdexcept>

typedef struct rsa_st RSA;

namespace otservpp { namespace crypto{

class Xtea{
public:
	Xtea(){}

	Xtea(uint32_t v0, uint32_t v1, uint32_t v2, uint32_t v3);

	void decrypt(uint32_t* buffer, std::size_t lenght);

	void encrypt(uint32_t* buffer, std::size_t lenght);

private:
	uint32_t key[4];
};

class Rsa{
public:
	Rsa(const char* n, const char* e, const char* d, const char* p, const char* q);

	~Rsa();

	/// Returns the size of the recovered buffered
	std::size_t decrypt(uint8_t* buffer, std::size_t lenght);

private:
	RSA* rsa;
};

} /* namespace crypto */
} /* namespace otservpp */

#endif // OTSERVP_CRYPTO_H_
