#ifndef eMU_GAMESERVER_GAMESERVER_H
#define eMU_GAMESERVER_GAMESERVER_H

#include "..\core\core.h"
#include "user.h"
#include "game.h"
#include "configuration.h"
#include "crypt.h"
#include <boost/bind.hpp>

#pragma warning(disable: 4355)

typedef std::vector<eMUCore::packet_t> dataServerPacketQueue_t;

class gameServer_t:	public gameNetworkInterface_t {
public:
	gameServer_t(size_t maxStoredLogsCount,
					size_t usersMax,
					unsigned short port):
	  m_userManager(usersMax,
					 boost::bind(&gameServer_t::onContextAttach, this, _1),
					 boost::bind(&gameServer_t::onContextReceive, this, _1),
					 boost::bind(&gameServer_t::onContextClose, this, _1)),
	  m_logger("log\\logs.html",
				maxStoredLogsCount),
	  m_scheduler(m_synchronizer),
	  m_iocpEngine(m_logger,
					m_synchronizer),
	  m_tcpClient(m_logger,
					m_synchronizer,
					boost::bind(&gameServer_t::onTcpClientConnect, this),
					boost::bind(&gameServer_t::onTcpClientReceive, this, _1, _2),
					boost::bind(&gameServer_t::onTcpClientClose, this)),
	  m_tcpServer(m_logger,
					m_iocpEngine,
					boost::bind(&gameServer_t::onContextAllocate, this),
					port),
	  m_udpSocket(m_logger,
				  boost::bind(&gameServer_t::onDatagramReceive, this, _1, _2, _3),
				  port),
	  m_game(m_logger,
				*this,
				m_userManager,
				m_scheduler),
	  m_userCount(0) {}

	~gameServer_t() {
		this->disconnectAll();
	}

	void startup();
	void worker();
	void updateWindowTitle() const;

	gameServerUser_t* onContextAllocate();

	void onContextAttach(eMUCore::socketContext_t &context);
	void onContextReceive(eMUCore::socketContext_t &context);
	void onContextClose(eMUCore::socketContext_t &context);

	void onDatagramReceive(sockaddr_in& /*inetAddr*/, unsigned char * /*data*/, size_t /*dataSize*/) {}

	void onTcpClientConnect();
	void onTcpClientReceive(unsigned char* data, size_t dataSize);
	void onTcpClientClose();

	// -----------------------------------------------------------------------
	// Interface for game_t.
	void send(gameServerUser_t &user, eMUCore::packet_t &packet);
	void sendDataServer(const eMUCore::packet_t &packet);
	void disconnect(gameServerUser_t &user) { m_iocpEngine.detach(user); }
	// -----------------------------------------------------------------------

	// metody sa statyczne z powodu console ctrl handlera...
	static void fatalHandler(int signalId);
	static DWORD WINAPI consoleEventHandler(unsigned int eventId);
	inline static void stop() { m_interrupt = true; }

private:
	gameServer_t();
	gameServer_t(const gameServer_t&);
	gameServer_t& operator=(const gameServer_t&);

	void dataServerConnect();
	void disconnectAll();
	void sendServerInfo() const;

	eMUCore::socketContextManager_t<gameServerUser_t>	m_userManager;
	eMUCore::logger_t									m_logger;
	eMUCore::synchronizer_t								m_synchronizer;
	eMUCore::scheduler_t								m_scheduler;
	eMUCore::tcpClient_t								m_tcpClient;
	eMUCore::iocpEngine_t								m_iocpEngine;
	eMUCore::tcpServer_t								m_tcpServer;
	eMUCore::udpSocket_t								m_udpSocket;
	dataServerPacketQueue_t								m_packetQueue;
	serverConfiguration_t								m_serverConfiguration;
	crypt_t												m_crypt;
	game_t												m_game;
	size_t												m_userCount;
	static bool											m_interrupt; // zmienna jest statyczna z powodu console ctrl handlera...
};

void main(int argsCount, char *argsVect[]);

#pragma warning(default: 4355)

#endif // eMU_GAMESERVER_GAMESERVER_H