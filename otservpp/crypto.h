#ifndef OTSERVP_CRYPTO_H_
#define OTSERVP_CRYPTO_H_

#include <boost/asio/io_service.hpp>
#include <boost/asio/strand.hpp>

typedef struct rsa_st RSA; // openssl rsa struct

namespace otservpp { namespace crypto{

/// Translates crypto errors into human redable strings
class ErrorCategory : public boost::system::error_category{
public:
	ErrorCategory();

	const char* name() const override;
	std::string message(int e) const override;
};

const boost::system::error_category& errorCategory();

/*! Asynchronous support for openssl
 * Openssl (the lib we use for RSA) requires us to define a set of lock callback functions in
 * order to be used in multi-threaded applications. Since I don't want to read the documentation,
 * we will do it async. (it's probably a waste of time since the only place RSA is used is on
 * during fist contact with a client, i.e. the login protocol).
 *
 * Here we just serialize the access to anything that needs openssl functionality, using an
 * implicit lock (asio strand).
 */
class CryptoService : public boost::asio::io_service::service{
public:
	static boost::asio::io_service::id id;

	explicit CryptoService(boost::asio::io_service& ioService) :
		boost::asio::io_service::service(ioService),
		strand(ioService)
	{}

	template <class Func>
	void runSync(Func&& alg)
	{
		strand.dispatch([=]{ alg(); });
	}

	void shutdown_service() override {}

private:
	boost::asio::strand strand;
};

/// Sems like a good place for this
uint32_t adler32(uint8_t* data, int32_t len);

/// XTEA cypher
class Xtea{
public:
	Xtea(){}

	Xtea(uint32_t v0, uint32_t v1, uint32_t v2, uint32_t v3);

	void decrypt(uint32_t* buffer, std::size_t lenght) const;

	void encrypt(uint32_t* buffer, std::size_t lenght) const;

private:
	uint32_t key[4];
};

/// RSA cypher
class Rsa{
public:
	/// Constructs a new RSA structure using the given io_service and keys, throws
	/// boost::system::system_error on failure
	Rsa(boost::asio::io_service& ioService,
		const char* n, const char* e, const char* d, const char* p, const char* q);

	~Rsa();

	template <class Handler>
	void decrypt(uint8_t* buffer, std::size_t length, Handler&& handler)
	{
		svc.runSync([=](){
			boost::system::error_condition e;
			int newLen = decrypt(buffer, length, e);
			handler(e, newLen);
		});
	}

private:
	int decrypt(uint8_t* buffer, std::size_t lenght, boost::system::error_condition& e);

	RSA* rsa;
	CryptoService& svc;
};

} /* namespace crypto */
} /* namespace otservpp */

#endif // OTSERVP_CRYPTO_H_
