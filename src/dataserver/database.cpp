#include <boost\algorithm\string.hpp>
#include "database.h"

void database_t::startup(const std::string &hostname,
							unsigned short port,
							const std::string &dbName,
							const std::string &userName,
							const std::string &password) {
	if(mysql_init(&m_connectionHandle) != NULL) {
		if(mysql_real_connect(&m_connectionHandle,
								hostname.c_str(),
								userName.c_str(),
								password.c_str(),
								dbName.c_str(),
								port,
								0,
								CLIENT_MULTI_RESULTS | CLIENT_MULTI_STATEMENTS) != NULL) {
			bool arg = true;
			mysql_options(&m_connectionHandle, MYSQL_OPT_RECONNECT, &arg);
		} else {
			eMUCore::exception_t e;
			e.in() << __FILE__ << ":" << __LINE__ << "[database_t::database_t()] MySQL connect error: " 
					<< mysql_error(&m_connectionHandle);
			throw e;
		}
	} else {
		eMUCore::exception_t e;
		e.in() << __FILE__ << ":" << __LINE__ << "[database_t::database_t()] MySQL init error.";
		throw e;
	}
}

void database_t::execute() {
	#ifdef _DEBUG
	m_logger.in(eMUCore::logger_t::_MESSAGE_DEBUG) << "Executing query: " << m_queryBuffer.str() << ".";
	m_logger.out();
	#endif

	// SQL Injection protect.
	boost::algorithm::replace_all(m_queryBuffer.str(), "'", "\\'");
	boost::algorithm::replace_all(m_queryBuffer.str(), "\\", "\\\\");

    if(mysql_real_query(&m_connectionHandle,
						m_queryBuffer.str().c_str(),
						m_queryBuffer.str().size()) != 0) {
		//m_logger.in(eMUCore::logger_t::_MESSAGE_ERROR) << "Could not execute query: " << m_queryBuffer.str()
		//												<< ". Reason: " << mysql_error(&m_connectionHandle);
		//m_logger.out();

		eMUCore::exception_t e;
		e.in() << "Could not execute query: " << m_queryBuffer.str() << ". Reason: " << mysql_error(&m_connectionHandle);
		throw e;
	}
}

void database_t::ping() {
	if(mysql_ping(&m_connectionHandle) != 0) {
		m_logger.in(eMUCore::logger_t::_MESSAGE_ERROR) << "Could not ping mysql server. Reason: " << mysql_error(&m_connectionHandle) << ".";
		m_logger.out();
	}
}

MYSQL_RES* database_t::getQueryResult() {
	MYSQL_RES *queryResult = mysql_store_result(&m_connectionHandle);

	if(queryResult == NULL) {
		//m_logger.in(eMUCore::logger_t::_MESSAGE_WARNING) << "No result for query: " << m_queryBuffer.str();
		//m_logger.out();

		eMUCore::exception_t e;
		e.in() << "Empty result for query: " << m_queryBuffer.str();
		throw e;
	} else {
		return queryResult;
	}
}