#ifndef eMU_GAMESERVER_GAME_H
#define eMU_GAMESERVER_GAME_H

#include "protocol.h"
#include "dsprotocol.h"
#include "user.h"
#include "configuration.h"
#include "map.h"
#include "gate.h"
#include "monster.h"
#include "viewport.h"

#pragma warning(disable: 4355)

class gameNetworkInterface_t {
public:
	virtual ~gameNetworkInterface_t() {}
	virtual void send(gameServerUser_t &user, eMUCore::packet_t &packet) = 0;
	virtual void sendDataServer(const eMUCore::packet_t &packet) = 0;
	virtual void disconnect(gameServerUser_t &user) = 0;
};

class game_t: public protocolExecutorInterface_t,
				public dataServerProtocolExecutorInterface_t {
public:
	game_t(eMUCore::logger_t &logger,
			gameNetworkInterface_t &iface,
			eMUCore::socketContextManager_t<gameServerUser_t> &userManager,
			eMUCore::scheduler_t &scheduler):
	  m_monsterManager(m_monsterAttributesManager,
						m_objectList,
						userManager.getCount()),
	  m_logger(logger),
	  m_scheduler(scheduler),
	  m_networkInterface(iface),
	  m_userManager(userManager),
	  m_protocol(*this),
	  m_dataServerProtocol(*this) {}

	void startup();

	inline void parsePacket(gameServerUser_t &user, 
								const eMUCore::packet_t &packet) const { m_protocol.core(user, packet); }

	inline void parseDataServerPacket(const eMUCore::packet_t &packet) const { m_dataServerProtocol.core(packet); }

	void onHandshake(gameServerUser_t &user) const;
	void onClientClose(gameServerUser_t &user);
	void onCharacterLeave(gameServerUser_t &user);

	void checkSelfClose();
	void saveCharacter(gameServerUser_t &user) const;
	void teleportCharacter(gameServerUser_t &user,
							unsigned char mapId,
							unsigned char x,
							unsigned char y,
							unsigned char direction,
							unsigned char gateId = 1);

	// --------------------------------------------------------
	// Interface for protocolExecutor_t.
	void onLoginRequest(gameServerUser_t &user,
							const std::string &accountId,
							const std::string &password,
							const std::string &clientExeVersion,
							const std::string &clientExeSerial);
	void onCharacterListRequest(gameServerUser_t &user);
	void onLogoutRequest(gameServerUser_t &user, unsigned char closeReason);
	void onCharacterCreateRequest(gameServerUser_t &user,
									const std::string &name,
									unsigned char race);
	void onCharacterDeleteRequest(gameServerUser_t &user,
									const std::string &name,
									const std::string &pin);
	void onCharacterSelectRequest(gameServerUser_t &user,
									const std::string &name);
	void onCharacterMoveRequest(gameServerUser_t &user,
									unsigned char x,
									unsigned char y,
									unsigned char direction,
									const map_t::path_t &path);
	void onCharacterTeleportRequest(gameServerUser_t &user,
									unsigned short gateId);
	// --------------------------------------------------------

	// --------------------------------------------------------
	// Interface for dataServerProtocolExecutor_t.
	void onAccountCheckAnswer(unsigned int connectionStamp,
								const std::string &accountId,
								unsigned char result);

	void onCharacterListAnswer(unsigned int connectionStamp,
								const eMUShared::characterList_t &characterList);

	void onCharacterCreateAnswer(unsigned int connectionStamp,
									const std::string &name,
									unsigned char race,
									unsigned char result);

	void onCharacterDeleteAnswer(unsigned int connectionStamp,
									const std::string &name,
									unsigned char result);

	void onCharacterSelectAnswer(unsigned int connectionStamp,
									const eMUShared::characterAttributes_t &attr);

	void onQueryExceptionNotice(unsigned int connectionStamp,
							const std::string &what);
	// --------------------------------------------------------

private:
	game_t();
	game_t(const game_t&);
	game_t& operator=(const game_t&);

	versionConfiguration_t								m_versionConfiguration;
	gameConfiguration_t									m_gameConfiguration;
	protocol_t											m_protocol;
	dataServerProtocol_t								m_dataServerProtocol;
	gameObject_t::gameObjectList_t						m_objectList;
	mapManager_t										m_mapManager;
	gateManager_t										m_gateManager;
	monsterAttributesManager_t							m_monsterAttributesManager;
	monsterManager_t									m_monsterManager;
	eMUCore::logger_t									&m_logger;
	eMUCore::scheduler_t								&m_scheduler;
	gameNetworkInterface_t								&m_networkInterface;
	eMUCore::socketContextManager_t<gameServerUser_t>	&m_userManager;
};

#pragma warning(default: 4355)

#endif // eMU_GAMESERVER_GAME_H