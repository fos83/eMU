#ifndef eMU_GAMESERVER_DSPROTOCOL_H
#define eMU_GAMESERVER_DSPROTOCOL_H

#include "core.h"
#include "shared.h"

// what a big name xD.
class dataServerProtocolExecutorInterface_t {
public:
	virtual void onAccountCheckAnswer(unsigned int connectionStamp,
								const std::string &accountId,
								unsigned char result) = 0;

	virtual void onCharacterListAnswer(unsigned int connectionStamp,
										const eMUShared::characterList_t &characterList) = 0;

	virtual void onCharacterCreateAnswer(unsigned int connectionStamp,
											const std::string &name,
											unsigned char race,
											unsigned char result) = 0;

	virtual void onCharacterDeleteAnswer(unsigned int connectionStamp,
											const std::string &name,
											unsigned char result) = 0;

	virtual void onCharacterSelectAnswer(unsigned int connectionStamp,
											const eMUShared::characterAttributes_t &attr) = 0;

	virtual void onQueryExceptionNotice(unsigned int connectionStamp,
									const std::string &what) = 0;
};

class dataServerProtocol_t {
public:
	dataServerProtocol_t(dataServerProtocolExecutorInterface_t &iface):
	  m_executorInterface(iface) {}

	void core(const eMUCore::packet_t &packet) const;

	void constructAccountCheckRequest(eMUCore::packet_t &buff,
									unsigned int connectionStamp,
									const std::string &accountId,
									const std::string &password,
									const std::string &ipAddress) const;

	void parseAccountCheckAnswer(const eMUCore::packet_t &packet) const;

	void constructCharacterListRequest(eMUCore::packet_t &buff,
										unsigned int connectionStamp,
										const std::string &accountId) const;

	void parseCharacterListAnswer(const eMUCore::packet_t &packet) const;

	void constructLogoutRequest(eMUCore::packet_t &buff,
								const std::string &accountId) const;

	void constructCharacterCreateRequest(eMUCore::packet_t &buff,
											unsigned int connectionStamp,
											const std::string &accountId,
											const std::string &name,
											unsigned char race) const;

	void parseCharacterCreateAnswer(const eMUCore::packet_t &packet) const;

	void constructCharacterDeleteRequest(eMUCore::packet_t &buff,
											unsigned int connectionStamp,
											const std::string &accountId,
											const std::string &name,
											const std::string &pin) const;

	void parseCharacterDeleteAnswer(const eMUCore::packet_t &packet) const;

	void constructCharacterSelectRequest(eMUCore::packet_t &buff,
											unsigned int connectionStamp,
											const std::string &accountId,
											const std::string &name) const;

	void parseCharacterSelectAnswer(const eMUCore::packet_t &packet) const;

	void constructCharacterSaveRequest(eMUCore::packet_t &buff,
										const std::string &accountId,
										const eMUShared::characterAttributes_t &attr) const;

	void parseQueryExceptionNotice(const eMUCore::packet_t &packet) const;

private:
	dataServerProtocol_t();
	dataServerProtocol_t(const dataServerProtocol_t&);
	dataServerProtocol_t& operator=(const dataServerProtocol_t&);

	dataServerProtocolExecutorInterface_t &m_executorInterface;
};

#endif // eMU_GAMESERVER_DSPROTOCOL_H