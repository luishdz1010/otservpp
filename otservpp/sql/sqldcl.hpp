#ifndef OTSERVPP_SQL_SQLDCL_HPP_
#define OTSERVPP_SQL_SQLDCL_HPP_

#include <memory>
#include "connection.hpp"
#include "mysqlservice.h"

namespace otservpp{ namespace sql{

typedef BasicConnection<mysql::Service> Connection;
typedef std::shared_ptr<Connection> ConnectionPtr;

} /* namespace sql */
} /* namespace otservpp */

#endif // OTSERVPP_SQL_SQLDCL_HPP_
