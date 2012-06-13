#ifndef OTSERVPP_SQL_HPP_
#define OTSERVPP_SQL_HPP_

#include <memory>
#include "sqldcl.hpp"
#include "connectionpool.h"
#include "query.h"

/*!\file
 * Useful file for inclusion in clients. It contains some typedefs in order to abstract the use
 * of the underlying SQL service. We can later change the service without modifying clients.
 */
namespace otservpp{ namespace sql{

typedef Connection::ResultSet ResultSet;
typedef Connection::Id ConnectionId;
typedef Connection::PreparedHandle PreparedHandle;

typedef Connection::Service::TinyInt  TinyInt;
typedef Connection::Service::SmallInt SmallInt;
typedef Connection::Service::Int      Int;
typedef Connection::Service::BigInt   BigInt;
typedef Connection::Service::Float    Float;
typedef Connection::Service::Double   Double;
typedef Connection::Service::String   String;
typedef Connection::Service::Blob     Blob;

template <class... T>
using Row = Connection::Row<T...>;

template <class... T>
using RowSet = Connection::RowSet<T...>;

} /* namespace sql */
} /* namespace otservpp */

#endif // OTSERVPP_SQL_HPP_
