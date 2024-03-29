#ifndef OTSERVPP_SQL_ERROR_HPP_
#define OTSERVPP_SQL_ERROR_HPP_

#include <mysql/errmsg.h>

namespace otservpp{ namespace sql{

// TODO camel-case this one :|
enum class Error{
	/// Unknown MySQL error
	UnknownError = CR_UNKNOWN_ERROR,

	/// Can't create UNIX socket (%d)
	socket_create_error = CR_SOCKET_CREATE_ERROR,

	/// Can't connect to local MySQL server through socket '%-.100s' (%d)
	connection_error = CR_CONNECTION_ERROR,

	/// Can't connect to MySQL server on '%-.100s' (%d)
	conn_host_error = CR_CONN_HOST_ERROR,

	/// Can't create TCP/IP socket (%d)
	ipsock_error = CR_IPSOCK_ERROR,

	/// Unknown MySQL server host '%-.100s' (%d)
	unknown_host = CR_UNKNOWN_HOST,

	/// MySQL server has gone away
	server_gone_error = CR_SERVER_GONE_ERROR,

	/// Protocol mismatch; server version = %d client version = %d
	version_error = CR_VERSION_ERROR,

	/// MySQL client ran out of memory
	OutOfMemory = CR_OUT_OF_MEMORY,

	/// Wrong host info
	wrong_host_info = CR_WRONG_HOST_INFO,

	/// Localhost via UNIX socket
	localhost_connection = CR_LOCALHOST_CONNECTION,

	/// %-.100s via TCP/IP
	tcp_connection = CR_TCP_CONNECTION,

	/// Error in server handshake
	server_handshake_err = CR_SERVER_HANDSHAKE_ERR,

	/// Lost connection to MySQL server during query
	server_lost = CR_SERVER_LOST,

	/// Commands out of sync; you can't run this command now
	commands_out_of_sync = CR_COMMANDS_OUT_OF_SYNC,

	/// Named pipe: %-.32s
	named_pipe_connection = CR_NAMEDPIPE_CONNECTION,

	/// Can't wait for named pipe to host: %-.64s  pipe: %-.32s (%lu)
	named_pipe_wait_error = CR_NAMEDPIPEWAIT_ERROR,

	/// Can't open named pipe to host: %-.64s  pipe: %-.32s (%lu)
	named_pipe_open_error = CR_NAMEDPIPEOPEN_ERROR,

	/// Can't set state of named pipe to host: %-.64s  pipe: %-.32s (%lu)
	named_pipe_setstate_error = CR_NAMEDPIPESETSTATE_ERROR,

	/// Can't initialize character set %-.32s (path: %-.100s)
	cant_read_charset = CR_CANT_READ_CHARSET,

	/// Got packet bigger than 'max_allowed_packet' bytes
	net_packet_too_large = CR_NET_PACKET_TOO_LARGE,

	/// Embedded server
	embedded_connection = CR_EMBEDDED_CONNECTION,

	/// Error on SHOW SLAVE STATUS
	probe_slave_status = CR_PROBE_SLAVE_STATUS,

	/// Error on SHOW SLAVE HOSTS
	probe_slave_hosts = CR_PROBE_SLAVE_HOSTS,

	/// Error connecting to slave
	probe_slave_connect = CR_PROBE_SLAVE_CONNECT,

	/// Error connecting to master
	probe_master_connect = CR_PROBE_MASTER_CONNECT,

	/// SSL connection error
	ssl_connection_error = CR_SSL_CONNECTION_ERROR,

	/// Malformed packet
	malformed_packet = CR_MALFORMED_PACKET,

	/// This client library is licensed only for use with MySQL servers having
	/// '%s' license
	wrong_license = CR_WRONG_LICENSE,

	/// Invalid use of null pointer
	null_pointer = CR_NULL_POINTER,

	/// Statement not prepared
	no_prepare_stmt = CR_NO_PREPARE_STMT,

	/// No data supplied for parameters in prepared statement
	params_not_bound = CR_PARAMS_NOT_BOUND,

	/// Data truncated
	data_truncated = CR_DATA_TRUNCATED,

	/// No parameters exist in the statement
	no_parameters_exists = CR_NO_PARAMETERS_EXISTS,

	/// Invalid parameter number
	invalid_parameter_no = CR_INVALID_PARAMETER_NO,

	/// Can't send long data for non-string/non-binary data types
	/// (parameter: %d)
	invalid_buffer_use = CR_INVALID_BUFFER_USE,

	/// Using unsupported buffer type: %d  (parameter: %d)
	unsupported_param_type = CR_UNSUPPORTED_PARAM_TYPE,

	/// Shared memory: %-.100s
	shared_memory_connection = CR_SHARED_MEMORY_CONNECTION,

	/// Can't open shared memory; client could not create request event (%lu)
	shared_memory_connect_request_error =
		CR_SHARED_MEMORY_CONNECT_REQUEST_ERROR,

	/// Can't open shared memory; no answer event received from server (%lu)
	shared_memory_connect_answer_error = CR_SHARED_MEMORY_CONNECT_ANSWER_ERROR,

	/// Can't open shared memory; server could not allocate file mapping (%lu)
	shared_memory_connect_file_map_error =
		CR_SHARED_MEMORY_CONNECT_FILE_MAP_ERROR,

	/// Can't open shared memory; server could not get pointer to file mapping
	/// (%lu)
	shared_memory_connect_map_error = CR_SHARED_MEMORY_CONNECT_MAP_ERROR,

	/// Can't open shared memory; client could not allocate file mapping (%lu)
	shared_memory_file_map_error = CR_SHARED_MEMORY_FILE_MAP_ERROR,

	/// Can't open shared memory; client could not get pointer to file mapping
	/// (%lu)
	shared_memory_map_error = CR_SHARED_MEMORY_MAP_ERROR,

	/// Can't open shared memory; client could not create %s event (%lu)
	shared_memory_event_error = CR_SHARED_MEMORY_EVENT_ERROR,

	/// Can't open shared memory; no answer from server (%lu)
	shared_memory_connect_abandoned_error =
		CR_SHARED_MEMORY_CONNECT_ABANDONED_ERROR,

	/// Can't open shared memory; cannot send request event to server (%lu)
	shared_memory_connect_set_error = CR_SHARED_MEMORY_CONNECT_SET_ERROR,

	/// Wrong or unknown protocol
	conn_unknow_protocol = CR_CONN_UNKNOW_PROTOCOL,

	/// Invalid connection handle
	invalid_conn_handle = CR_INVALID_CONN_HANDLE,

	/// Connection using old (pre-4.1.1) authentication protocol refused
	/// (client option 'secure_auth' enabled)
	secure_auth = CR_SECURE_AUTH,

	/// Row retrieval was canceled by mysql_stmt_close() call
	fetch_canceled = CR_FETCH_CANCELED,

	/// Attempt to read column without prior row fetch
	no_data = CR_NO_DATA,

	/// Prepared statement contains no metadata
	no_stmt_metadata = CR_NO_STMT_METADATA,

	/// Attempt to read a row while there is no result set associated with the
	/// statement
	no_result_set = CR_NO_RESULT_SET,

	/// This feature is not implemented yet
	not_implemented = CR_NOT_IMPLEMENTED,


	// Custom errors follow
	/// The number of fields that the query will return is different from the expected
	BadFieldCount = CR_ERROR_LAST+1,

	/// The returned result set is to big to hold in memory
	ResultIsTooBig
};

class SqlErrorCategory : public boost::system::error_category{
public:
	SqlErrorCategory(){}

	const char* name() const override
	{
		return "mysql";
	}

	// TODO implement these
	std::string message(int e) const override
	{
		return "error messages not yet implemented";
	}
};

inline const boost::system::error_category& getErrorCategory()
{
	static SqlErrorCategory e;
	return e;
}

// TODO add templates overloads for boost::system::is_error_code_enum to enable comparison

} /* namespace sql */
} /* namespace otservpp */

#endif // ERROR_HPP_ 
