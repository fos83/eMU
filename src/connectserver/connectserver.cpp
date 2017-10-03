#include <iostream>
#include <signal.h>
#include "connectserver.h"

bool connectServer_t::m_interrupt = false;

void connectServer_t::startup() {
	m_logger.startup();

	m_logger.in(eMUCore::logger_t::_MESSAGE_INFO) << "Initializing game.";
	m_logger.out();
	m_game.startup();

	m_logger.in(eMUCore::logger_t::_MESSAGE_INFO) << "Starting iocp engine.";
	m_logger.out();
	m_iocpEngine.startup();

	m_logger.in(eMUCore::logger_t::_MESSAGE_INFO) << "Starting tcp server on port " << m_tcpServer.getListenPort() << ".";
	m_logger.out();
	m_tcpServer.startup();

	m_logger.in(eMUCore::logger_t::_MESSAGE_INFO) << "Starting udp socket on port " << m_udpSocket.getPort() << ".";
	m_logger.out();
	m_udpSocket.startup();

	this->updateWindowTitle();

	m_logger.in(eMUCore::logger_t::_MESSAGE_INFO) << "Waiting for connections.";
	m_logger.out();
}

void connectServer_t::worker() {
	while(!m_interrupt) {
		m_udpSocket.worker();
		m_scheduler.worker();
		Sleep(1);
	}
}

void connectServer_t::updateWindowTitle() const {
	std::stringstream titleStream;
	titleStream << "[ConnectServer] ::"
				<< " [Users: " << m_userCount << "/" << m_userManager.getCount() << "] ::"
				<< " [Port: " << m_tcpServer.getListenPort() << "]";

	SetConsoleTitle(titleStream.str().c_str());
}

void connectServer_t::disconnectAll() {
	for(size_t i = 0; i < m_userManager.getCount(); ++i) {
		if(m_userManager[i].isActive()) {
			this->disconnect(m_userManager[i]);
		}
	}
}

connectServerUser_t* connectServer_t::onContextAllocate() {
	connectServerUser_t *user = m_userManager.findFree();

	if(user == NULL) {
		m_logger.in(eMUCore::logger_t::_MESSAGE_ERROR) << "No free objects found.";
		m_logger.out();
	}

	return user;
}

void connectServer_t::onContextAttach(eMUCore::socketContext_t &context) {
	connectServerUser_t &user = reinterpret_cast<connectServerUser_t&>(context);

	try {
		m_logger.in(eMUCore::logger_t::_MESSAGE_INFO) << user << " Connected.";
		m_logger.out();

		// -----------------------
		++m_userCount;
		this->updateWindowTitle();
		// -----------------------

		// ---------------------
		m_game.onHandshake(user);
		// ---------------------
	} catch(eMUCore::exception_t &e) {
		m_logger.in(eMUCore::logger_t::_MESSAGE_ERROR) << "Exception: " << user << " " << e.what();
		m_logger.out();
		this->disconnect(user);
	} catch(std::exception &e) {
		m_logger.in(eMUCore::logger_t::_MESSAGE_ERROR) << "std exception: " << user << " " << e.what();
		m_logger.out();
		this->disconnect(user);
	} catch(...) {
		m_logger.in(eMUCore::logger_t::_MESSAGE_ERROR) << "Exception: " << user << " Unknown exception.";
		m_logger.out();
		this->disconnect(user);
	}
}

void connectServer_t::onContextReceive(eMUCore::socketContext_t &context) {
	connectServerUser_t &user = reinterpret_cast<connectServerUser_t&>(context);

	try {
		size_t parsedDataSize = 0;

		do {
			size_t rawDataSize = eMUCore::packet_t::getRawDataSize(&user.getRecvBuffer().m_data[parsedDataSize]);
			eMUCore::packet_t packet(&user.getRecvBuffer().m_data[parsedDataSize]);

			m_logger.in(eMUCore::logger_t::_MESSAGE_PROTOCOL) << user << " Received " << packet << ".";
			m_logger.out();

			// -------------------------------
			m_game.parsePacket(user, packet);
			// -------------------------------

			parsedDataSize += rawDataSize;
		} while(parsedDataSize < user.getRecvBuffer().m_dataSize);
	} catch(eMUCore::exception_t &e) {
		m_logger.in(eMUCore::logger_t::_MESSAGE_ERROR) << "Exception: " << user << " " << e.what();
		m_logger.out();
		this->disconnect(user);
	} catch(std::exception &e) {
		m_logger.in(eMUCore::logger_t::_MESSAGE_ERROR) << "std exception: " << user << " " << e.what();
		m_logger.out();
		this->disconnect(user);
	} catch(...) {
		m_logger.in(eMUCore::logger_t::_MESSAGE_ERROR) << "Exception: " << user << " Unknown exception.";
		m_logger.out();
		this->disconnect(user);
	}
}

void connectServer_t::onContextClose(eMUCore::socketContext_t &context) {
	connectServerUser_t &user = reinterpret_cast<connectServerUser_t&>(context);

	m_logger.in(eMUCore::logger_t::_MESSAGE_INFO) << user << " Disconnected.";
	m_logger.out();

	// -----------------------
	--m_userCount;
	this->updateWindowTitle();
	// -----------------------
}

void connectServer_t::onDatagramReceive(sockaddr_in& /*inetAddr*/, unsigned char* data, size_t dataSize) {
	try {
		size_t parsedDataSize = 0;

		do {
			size_t rawDataSize = eMUCore::packet_t::getRawDataSize(&data[parsedDataSize]);

			eMUCore::packet_t packet(&data[parsedDataSize]);

			if(packet.getProtocolId() == 0x01) {
				unsigned short gameServerCode = packet.read<unsigned short>(3);
				size_t gameServerLoad = packet.read<unsigned char>(5);
				m_game.serverListUpdate(gameServerCode, gameServerLoad);
			}

			parsedDataSize += rawDataSize;
		} while(parsedDataSize < dataSize);
	} catch(eMUCore::exception_t &e) {
		m_logger.in(eMUCore::logger_t::_MESSAGE_ERROR) << "Exception: [UDP] " << e.what();
		m_logger.out();
	} catch(std::exception &e) {
		m_logger.in(eMUCore::logger_t::_MESSAGE_ERROR) << "std exception: [UDP] " << e.what();
		m_logger.out();
	} catch(...) {
		m_logger.in(eMUCore::logger_t::_MESSAGE_ERROR) << "Unknown exception: [UDP].";
		m_logger.out();
	}
}

void connectServer_t::send(connectServerUser_t &user, const eMUCore::packet_t &packet) {
	m_logger.in(eMUCore::logger_t::_MESSAGE_PROTOCOL) << user << " Sending " << packet << ".";
	m_logger.out();
	m_iocpEngine.write(user, packet.getData(), packet.getSize());
}

void connectServer_t::fatalHandler(int /*signalId*/) {
	//eMUCore::stackTrace_t::dumpCallTree("log\\stackTrace.txt");
}

DWORD WINAPI connectServer_t::consoleEventHandler(unsigned int /*eventId*/) {
	if(!connectServer_t::m_interrupt) {
		int ret = MessageBox(0, "Do you really want to quit the server?", "eMU::ConnectServer", MB_ICONQUESTION | MB_YESNO);
		if(ret == IDYES) {
			connectServer_t::stop();
		}
	}

    return TRUE;
}

void main(int argsCount, char *argsVect[]) {
	signal(SIGABRT, connectServer_t::fatalHandler);
	signal(SIGFPE, connectServer_t::fatalHandler);
	signal(SIGILL, connectServer_t::fatalHandler);
	signal(SIGSEGV, connectServer_t::fatalHandler);
	signal(SIGTERM, connectServer_t::fatalHandler);

	//system("COLOR 0F");
	std::cout << "		+-----------------------------------------------+" << std::endl;
	std::cout << "		|						|" << std::endl;
	std::cout << "		|		eMU :: ConnectServer		|"  << std::endl;
	std::cout << "		|		 Build: " << __DATE__ << "		|" << std::endl;
	std::cout << "		|						|" << std::endl;
	std::cout << "		+-----------------------------------------------+" << std::endl << std::endl;

	try {
		if(SetConsoleCtrlHandler(reinterpret_cast<PHANDLER_ROUTINE>(connectServer_t::consoleEventHandler), TRUE) != 0) {
			WSADATA wsaData = {0};
			if(WSAStartup(MAKEWORD(2, 2), &wsaData) == 0) {
				if(argsCount == 4) {
					connectServer_t *connectServer = new connectServer_t(boost::lexical_cast<size_t>(argsVect[1]),
																			boost::lexical_cast<size_t>(argsVect[2]),
																			boost::lexical_cast<unsigned short>(argsVect[3]));
					connectServer->startup();
					connectServer->worker();
					delete connectServer;
				} else {
					eMUCore::exception_t e;
					e.in() << __FILE__ << ":" << __LINE__ << "[main()] Invalid command line.";
					throw e;
				}
			} else {
				eMUCore::exception_t e;
				e.in() << __FILE__ << ":" << __LINE__ << "[main()] WSAStartup failed with error#" << WSAGetLastError() << ".";
				throw e;
			}
		} else {
			eMUCore::exception_t e;
			e.in() << __FILE__ << ":" << __LINE__ << "[main()] SetConsoleCtrlHandler failed with error#" << GetLastError() << ".";
			throw e;
		}
	} catch(eMUCore::exception_t &e) {
		std::cout << "[Exception] " << e.what() << std::endl;
	} catch(std::exception &e) {
		std::cout << "[std exception] " << e.what() << std::endl;
	}

	system("PAUSE");
}