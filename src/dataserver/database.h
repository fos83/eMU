#ifndef eMU_DATASERVER_DATABASE_H
#define eMU_DATASERVER_DATABASE_H

#include <string>
#include <sstream>
#include <winsock2.h> // What the fuck?????????????????
#include <mysql\mysql.h>
#include "..\core\core.h"

class database_t {
public:
	database_t(eMUCore::logger_t &logger):
	  m_logger(logger) {}

	~database_t() {
		mysql_close(&m_connectionHandle);
	}

	void startup(const std::string &hostname,
					unsigned short port,
					const std::string &dbName,
					const std::string &userName,
					const std::string &password);

	inline std::stringstream& query() { 
		m_queryBuffer.str("");
		return m_queryBuffer; 
	}
	void execute();
	void ping();
	MYSQL_RES* getQueryResult();

	class iterator_t {
	public:
		iterator_t(MYSQL_RES *queryResult):
		  m_queryResult(queryResult),
		  m_resultRow(NULL),
		  m_isFirstIteration(true) {
			this->mapFields();
			m_resultRow = mysql_fetch_row(m_queryResult);
		}

		~iterator_t() {
			mysql_free_result(m_queryResult);
			m_queryResult = NULL;
			m_resultRow = NULL;
			m_fieldMap.clear();
		}

		bool nextRow() {
			if(!m_isFirstIteration) {
				m_resultRow = mysql_fetch_row(m_queryResult);

				if(m_resultRow != NULL) {
					return true;
				} else {
					return false;
				}
			} else {
				m_isFirstIteration = false;

				if(this->getNumRows() > 0) {
					return true;
				} else {
					return false;
				}
			}
		}

		size_t getNumRows() {
			if(m_queryResult != NULL) {
				return static_cast<size_t>(mysql_num_rows(m_queryResult));
			} else {
				return 0;
			}
		}

		size_t getNumFields() {
			if(m_queryResult != NULL) {
				return mysql_num_fields(m_queryResult); 
			} else {
				return 0;
			}
		}

		inline bool empty() { return this->getNumRows() == 0; }

		template<typename T>
		T getValue(const std::string &fieldName) {
			fieldMap_t::iterator iter = m_fieldMap.find(fieldName);

			if(iter != m_fieldMap.end()) {
				return getValue<T>(iter->second);
			} else {
				eMUCore::exception_t e;
				e.in() << "Field '" << fieldName << "' is not defined.";
				throw e;
			}
		}

		template<typename T>
		T getValue(size_t index) {
			if(this->getNumFields() > index) {
				if(m_resultRow[index] == NULL) {
					return boost::lexical_cast<T>(0);
				} else {
					return boost::lexical_cast<T>(m_resultRow[index]);
				}
			} else {
				eMUCore::exception_t e;
				e.in() << "Field index " << index << " out of range.";
				throw e;
			}
		}

	private:
		typedef std::map<std::string, size_t> fieldMap_t;

		iterator_t();
		iterator_t(const iterator_t&);
		iterator_t& operator=(const iterator_t&);

		void mapFields() {
			MYSQL_FIELD *field;
			size_t i = 0;

			while(field = mysql_fetch_field(m_queryResult)) {
				m_fieldMap[field->name] = i;
				++i;
			}
		}

		fieldMap_t	m_fieldMap;
		MYSQL_RES	*m_queryResult;
		MYSQL_ROW	m_resultRow;
		bool		m_isFirstIteration;
	};

private:
	database_t();
	database_t(const database_t &database);
	database_t& operator=(const database_t &database);

	std::stringstream m_queryBuffer;
	MYSQL m_connectionHandle;
	eMUCore::logger_t &m_logger;
};

#endif // eMU_DATASERVER_DATABASE_H