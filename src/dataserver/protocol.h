#ifndef eMU_DATASERVER_PROTOCOL_H
#define eMU_DATASERVER_PROTOCOL_H

#include "core.h"
#include "shared.h"
#include "user.h"

class protocolExecutorInterface_t {
public:
	virtual void onAccountCheckRequest(dataServerUser_t &user,
										unsigned int connectionStamp,
										const std::string &accountId,
										const std::string &password,
										const std::string &ipAddress) = 0;

	virtual void onCharacterListRequest(dataServerUser_t &user,
											unsigned int connectionStamp,
											const std::string &accountId) = 0;

	virtual void onLogoutRequest(dataServerUser_t &user,
									const std::string &accountId) = 0;

	virtual void onCharacterCreateRequest(dataServerUser_t &user,
											unsigned int connectionStamp,
											const std::string &accountId,
											const std::string &name,
											unsigned char race) = 0;

	virtual void onCharacterDeleteRequest(dataServerUser_t &user,
											unsigned int connectionStamp,
											const std::string &accountId,
											const std::string &name,
											const std::string &pin) = 0;

	virtual void onCharacterSelectRequest(dataServerUser_t &user,
											unsigned int connectionStamp,
											const std::string &accountId,
											const std::string &name) = 0;

	virtual void onCharacterSaveRequest(dataServerUser_t &user,
										const std::string &accountId,
										const eMUShared::characterAttributes_t &attr) = 0;
};

class protocol_t {
public:
	protocol_t(protocolExecutorInterface_t &iface):
	  m_executorInterface(iface) {}

	void core(dataServerUser_t &user, 
				const eMUCore::packet_t &packet) const;

	void parseAccountCheckRequest(dataServerUser_t &user,
								const eMUCore::packet_t &packet) const;

	void constructAccountCheckAnswer(eMUCore::packet_t &buff,
									unsigned int connectionStamp,
									const std::string &accountId,
									unsigned char result) const;

	void parseCharacterListRequest(dataServerUser_t &user,
									const eMUCore::packet_t &packet) const;

	void constructCharacterListAnswer(eMUCore::packet_t &buff,
										unsigned int connectionStamp,
										const eMUShared::characterList_t &characterList) const;

	void parseLogoutRequest(dataServerUser_t &user,
							const eMUCore::packet_t &packet) const;

	void parseCharacterCreateRequest(dataServerUser_t &user,
										const eMUCore::packet_t &packet) const;

	void constructCharacterCreateAnswer(eMUCore::packet_t &buff,
										unsigned int connectionStamp,
										const std::string &name,
										unsigned char race,
										unsigned char result) const;

	void parseCharacterDeleteRequest(dataServerUser_t &user,
										const eMUCore::packet_t &packet) const;

	void constructCharacterDeleteAnswer(eMUCore::packet_t &buff,
										unsigned int connectionStamp,
										const std::string &name,
										unsigned char result) const;

	void parseCharacterSelectRequest(dataServerUser_t &user,
										const eMUCore::packet_t &packet) const;

	void constructCharacterSelectAnswer(eMUCore::packet_t &buff,
										unsigned int connectionStamp,
										const eMUShared::characterAttributes_t &attr) const;

	void parseCharacterSaveRequest(dataServerUser_t &user,
									const eMUCore::packet_t &packet) const;

	void constructQueryExceptionNotice(eMUCore::packet_t &buff,
										unsigned int connectionStamp,
										const std::string &what) const;

private:
	protocol_t();
	protocol_t(const protocol_t&);
	protocol_t& operator=(const protocol_t&);

	protocolExecutorInterface_t &m_executorInterface;
};

#endif // eMU_DATASERVER_PROTOCOL_H