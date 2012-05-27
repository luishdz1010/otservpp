#ifndef OTSERVPP_NETWORKDCL_HPP_
#define OTSERVPP_NETWORKDCL_HPP_

#include <boost/asio.hpp>

namespace otservpp{

typedef boost::asio::ip::tcp::socket TcpSocket;
typedef boost::system::error_code SystemError;

}

#endif // OTSERVPP_NETWORKDCL_HPP_
