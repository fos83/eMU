#ifndef eMU_CONNECTSERVER_CONNECTSERVER_H
#define eMU_CONNECTSERVER_CONNECTSERVER_H

#include <vector>
#include <boost\bind.hpp>
#include "core.h"
#include "user.h"
#include "game.h"

#pragma warning(disable: 4355)

class connectServer_t: public gameNetworkInterface_t {
public:
	connectServer_t(size_t maxStoredLogsCount,
					size_t userMax,
					unsigned short port):
	  m_userManager(userMax,
					boost::bind(&connectServer_t::onContextAttach, this, _1),
					boost::bind(&connectServer_t::onContextReceive, this, _1),
					boost::bind(&connectServer_t::onContextClose, this, _1)),
	  m_logger("log\\logs.html",
				maxStoredLogsCount),
	  m_scheduler(m_synchronizer),
	  m_iocpEngine(m_logger,
					m_synchronizer),
	  m_tcpServer(m_logger,
					m_iocpEngine,
					boost::bind(&connectServer_t::onContextAllocate, this),
					port),
	  m_udpSocket(m_logger,
					boost::bind(&connectServer_t::onDatagramReceive, this, _1, _2, _3),
					port),
	  m_game(m_logger,
				m_scheduler,
				*this),
	  m_userCount(0) {}

	~connectServer_t() { this->disconnectAll();	}

	void startup();
	void worker();
	void updateWindowTitle() const;

	connectServerUser_t* onContextAllocate();

	void onContextAttach(eMUCore::socketContext_t &context);
	void onContextReceive(eMUCore::socketContext_t &context);
	void onContextClose(eMUCore::socketContext_t &context);

	void onDatagramReceive(sockaddr_in& inetAddr, unsigned char *data, size_t dataSize);

	// -----------------------------------------------------------------------
	// Interface for game_t.
	void send(connectServerUser_t &user, const eMUCore::packet_t &packet);
	void disconnect(connectServerUser_t &user) { m_iocpEngine.detach(user); }
	// -----------------------------------------------------------------------

	// metody sa statyczne z powodu console ctrl handlera...
	static void fatalHandler(int signalId);
	static DWORD WINAPI consoleEventHandler(unsigned int eventId);
	inline static void stop() { m_interrupt = true; }

private:
	connectServer_t();
	connectServer_t(const connectServer_t&);
	connectServer_t& operator=(const connectServer_t&);

	void disconnectAll();

	eMUCore::socketContextManager_t<connectServerUser_t>	m_userManager;
	eMUCore::logger_t										m_logger;
	eMUCore::synchronizer_t									m_synchronizer;
	eMUCore::scheduler_t									m_scheduler;
	eMUCore::iocpEngine_t									m_iocpEngine;
	eMUCore::tcpServer_t									m_tcpServer;
	eMUCore::udpSocket_t									m_udpSocket;
	game_t													m_game;
	size_t													m_userCount;
	static bool												m_interrupt; // zmienna jest statyczna z powodu console ctrl handlera...
};

void main(int argsCount, char *argsVect[]);

#pragma warning(default: 4355)

#endif // eMU_CONNECTSERVER_CONNECTSERVER_H