#ifndef eMU_DATASERVER_DATASERVER_H
#define eMU_DATASERVER_DATASERVER_H

#include "core.h"
#include "user.h"
#include "game.h"
#include "configuration.h"
#include <boost\bind.hpp>

#pragma warning(disable: 4355)

class dataServer_t:	public gameNetworkInterface_t {
public:
	dataServer_t(size_t maxStoredLogsCount,
					size_t userMax,
					unsigned short port):
	  m_userManager(userMax,
					boost::bind(&dataServer_t::onContextAttach, this, _1),
					boost::bind(&dataServer_t::onContextReceive, this, _1),
					boost::bind(&dataServer_t::onContextClose, this, _1)),
	  m_logger("log\\logs.html",
				maxStoredLogsCount),
	  m_scheduler(m_synchronizer),
	  m_iocpEngine(m_logger,
					m_synchronizer),
	  m_tcpServer(m_logger,
					m_iocpEngine,
					boost::bind(&dataServer_t::onContextAllocate, this),
					port),
	  m_game(m_logger,
				m_scheduler,
				*this),
	  m_userCount(0) {}

	~dataServer_t() { this->disconnectAll(); }

	void startup();
	void worker();
	void updateWindowTitle() const;

	dataServerUser_t* onContextAllocate();

	void onContextAttach(eMUCore::socketContext_t &context);
	void onContextReceive(eMUCore::socketContext_t &context);
	void onContextClose(eMUCore::socketContext_t &context);

	// -----------------------------------------------------------------------
	// Interface for game_t.
	void send(dataServerUser_t &user, const eMUCore::packet_t &packet);
	void disconnect(dataServerUser_t &user) { m_iocpEngine.detach(user); }
	// -----------------------------------------------------------------------

	// metody sa statyczne z powodu console ctrl handlera...
	static void fatalHandler(int signalId);
	static DWORD WINAPI consoleEventHandler(unsigned int eventId);
	inline static void stop() { m_interrupt = true; }

private:
	dataServer_t();
	dataServer_t(const dataServer_t&);
	dataServer_t& operator=(const dataServer_t&);

	void disconnectAll();

	eMUCore::socketContextManager_t<dataServerUser_t>	m_userManager;
	eMUCore::logger_t									m_logger;
	eMUCore::synchronizer_t								m_synchronizer;
	eMUCore::scheduler_t								m_scheduler;
	eMUCore::iocpEngine_t								m_iocpEngine;
	eMUCore::tcpServer_t								m_tcpServer;
	allowedHostList_t									m_allowedHostList;
	game_t												m_game;
	size_t												m_userCount;
	static bool											m_interrupt; // zmienna jest statyczna z powodu console ctrl handlera...
};

void main(int argsCount, char *argsVect[]);

#pragma warning(default: 4355)

#endif // eMU_DATASERVER_DATASERVER_H