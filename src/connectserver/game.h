#ifndef eMU_CONNECTSERVER_GAME_H
#define eMU_CONNECTSERVER_GAME_H

#include "core.h"
#include "protocol.h"
#include "serverlist.h"
#include "user.h"

#pragma warning(disable: 4355)

class gameNetworkInterface_t {
public:
	virtual ~gameNetworkInterface_t() {}
	virtual void send(connectServerUser_t &user, const eMUCore::packet_t &packet) = 0;
	virtual void disconnect(connectServerUser_t &user) = 0;
};

class game_t: public protocolExecutorInterface_t {
public:
	game_t(eMUCore::logger_t &logger,
			eMUCore::scheduler_t &scheduler,
			gameNetworkInterface_t &iface):
	  m_logger(logger),
	  m_scheduler(scheduler),
	  m_networkInterface(iface),
	  m_serverList(m_logger, 5),
	  m_protocol(*this) {}

	void startup();

	inline void parsePacket(connectServerUser_t &user, 
								const eMUCore::packet_t &packet) const { m_protocol.core(user, packet); }

	inline void serverListUpdate(int serverCode, 
									size_t load) { m_serverList.serverUpdate(serverCode, load); }

	void onHandshake(connectServerUser_t &user);

	// ----------------------------------------------------
	// Interface for protocolExecutor_t.
	void onServerListRequest(connectServerUser_t &user);

	void onServerSelectRequest(connectServerUser_t &user,
								unsigned short serverCode);
	// ----------------------------------------------------

private:
	game_t();
	game_t(const game_t &game);
	game_t& operator=(const game_t &game);

	protocol_t					m_protocol;
	eMUCore::logger_t			&m_logger;
	eMUCore::scheduler_t		&m_scheduler;
	gameNetworkInterface_t		&m_networkInterface;
	serverList_t				m_serverList;
};

#pragma warning(default: 4355)

#endif // eMU_CONNECTSERVER_GAME_H