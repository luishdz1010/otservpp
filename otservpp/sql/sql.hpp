#ifndef OTSERVPP_SQL_HPP_
#define OTSERVPP_SQL_HPP_

#include <memory>
#include "connection.hpp"

namespace otservpp{ namespace sql{

template <class SqlService>
class BasicConnection;

struct OnDeleteReturnToPool;
class ConnectionPool;
class Query;
class PreparedQuery;

namespace mysql{

class Service;
class ResultSet;

}

typedef BasicConnection<mysql::Service> Connection;
typedef Connection::ResultSet ResultSet;

typedef std::shared_ptr<Connection> ConnectionPtr;

} /* namespace sql */
} /* namespace otservpp */

#endif // OTSERVPP_SQL_HPP_
