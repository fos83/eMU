#ifndef eMU_GAMESERVER_PROTOCOL_H
#define eMU_GAMESERVER_PROTOCOL_H

#include "..\core\core.h"
#include "..\shared\shared.h"
#include "user.h"
#include "character.h"
#include "map.h"

class protocolExecutorInterface_t {
public:
	virtual void onLoginRequest(gameServerUser_t &user,
								const std::string &accountId,
								const std::string &password,
								const std::string &clientExeVersion,
								const std::string &clientExeSerial) = 0;

	virtual void onCharacterListRequest(gameServerUser_t &user) = 0;
	virtual void onLogoutRequest(gameServerUser_t &user, unsigned char closeReason) = 0;
	virtual void onCharacterCreateRequest(gameServerUser_t &user,
											const std::string &name,
											unsigned char race) = 0;
	virtual void onCharacterDeleteRequest(gameServerUser_t &user,
											const std::string &name,
											const std::string &pin) = 0;
	virtual void onCharacterSelectRequest(gameServerUser_t &user,
											const std::string &name) = 0;
	virtual void onCharacterMoveRequest(gameServerUser_t &user,
										unsigned char x,
										unsigned char y,
										unsigned char direction,
										const map_t::path_t &path) = 0;
	virtual void onCharacterTeleportRequest(gameServerUser_t &user,
											unsigned short gateId) = 0;

};

class protocol_t {
public:
	protocol_t(protocolExecutorInterface_t &iface):
	  m_executorInterface(iface) {}

	void core(gameServerUser_t &user, 
				const eMUCore::packet_t &packet) const;

	void constructHandshake(eMUCore::packet_t &buff, unsigned short userIndex, std::string version) const;

	void parseLoginRequest(gameServerUser_t &user, 
									const eMUCore::packet_t &packet) const;

	void constructLoginAnswer(eMUCore::packet_t &buff, 
										unsigned char result) const;

	void parseCharacterListRequest(gameServerUser_t &user, 
									const eMUCore::packet_t &packet) const;

	void constructCharacterListAnswer(eMUCore::packet_t &buff,
										unsigned char availableRaces,
										const eMUShared::characterList_t &characterList) const;

	void parseLogoutRequest(gameServerUser_t &user,
							const eMUCore::packet_t &packet) const;

	void constructLogoutAnswer(eMUCore::packet_t &buff,
								unsigned char closeReason) const;

	void parseCharacterCreateRequest(gameServerUser_t &user,
										const eMUCore::packet_t &packet) const;

	void constructCharacterCreteAnswer(eMUCore::packet_t &buff,
										unsigned char result,
										const std::string &name,
										int slot,
										unsigned short level,
										unsigned char race) const;

	void parseCharacterDeleteRequest(gameServerUser_t &user,
										const eMUCore::packet_t &packet) const;

	void constructCharacterDeleteAnswer(eMUCore::packet_t &buff,
										unsigned char result) const;

	void parseCharacterSelectRequest(gameServerUser_t &user,
										const eMUCore::packet_t &packet) const;

	void constructCharacterSelectAnswer(eMUCore::packet_t &buff,
										const character_t &character) const;

	void constructTextNotice(eMUCore::packet_t &buff,
								unsigned char type,
								const std::string &notice,
								unsigned char loopCount = 0,
								unsigned short loopDelay = 0,
								unsigned int color = 0,
								unsigned char speed = 0) const;

	void parseCharacterMoveRequest(gameServerUser_t &user,
									const eMUCore::packet_t &packet) const;

	void parseCharacterTeleportRequest(gameServerUser_t &user,
										const eMUCore::packet_t &packet) const;

	void constructCharacterTeleportAnswer(eMUCore::packet_t &buff,
											unsigned char mapId,
											unsigned char x,
											unsigned char y,
											unsigned char direction,
											unsigned char gateId = 1) const;

private:
	std::string xorString(const std::string &buff) const;

	protocol_t();
	protocol_t(const protocol_t&);
	protocol_t& operator=(const protocol_t&);

	protocolExecutorInterface_t &m_executorInterface;
};

#endif // eMU_GAMESERVER_PROTOCOL_H