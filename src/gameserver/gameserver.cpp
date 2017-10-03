#include <iostream>
#include <signal.h>
#include "gameserver.h"

bool gameServer_t::m_interrupt = false;

void gameServer_t::startup() {
	m_logger.startup();

	m_logger.in(eMUCore::logger_t::_MESSAGE_INFO) << "Initializing server configuration.";
	m_logger.out();
	m_serverConfiguration.read("configuration.xml");

	m_logger.in(eMUCore::logger_t::_MESSAGE_INFO) << "Initializing crypt.";
	m_logger.out();
	m_crypt.startup("..\\data\\Enc2.dat", "..\\data\\Dec1.dat");

	m_logger.in(eMUCore::logger_t::_MESSAGE_INFO) << "Initializing game.";
	m_logger.out();
	m_game.startup();

	this->dataServerConnect();

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

	m_scheduler.insert(eMUCore::scheduler_t::_SCHEDULE_SYNCHRONIZED,
						boost::bind(&gameServer_t::sendServerInfo, this),
						1);

	m_logger.in(eMUCore::logger_t::_MESSAGE_INFO) << "Waiting for connections.";
	m_logger.out();
}

void gameServer_t::dataServerConnect() {
	bool connected = false;

	do {
		if(!(connected = m_tcpClient.connect(m_serverConfiguration.m_masterDataServerHost,
													m_serverConfiguration.m_masterDataServerPort))) {

			if(!(connected = m_tcpClient.connect(m_serverConfiguration.m_slaveDataServerHost,
														m_serverConfiguration.m_slaveDataServerPort))) {
				m_logger.in(eMUCore::logger_t::_MESSAGE_ERROR) << "Could not connect to any dataserver.";
				m_logger.out();
			}
		}
	} while(!connected && !m_interrupt);
}

void gameServer_t::worker() {
	while(!m_interrupt) {
		m_tcpClient.worker();
		m_udpSocket.worker();
		m_scheduler.worker();
		Sleep(1);
	}
}

void gameServer_t::updateWindowTitle() const {
	std::stringstream titleStream;
	titleStream << "[GameServer] ::"
				<< " [Users: " << m_userCount << "/" << m_userManager.getCount() << "] ::"
				<< " [Port: " << m_tcpServer.getListenPort() << "] ::"
				<< " [DataServer: " << m_tcpClient.getHostname() << ":" << m_tcpClient.getPort() << "]";

	SetConsoleTitle(titleStream.str().c_str());
}

void gameServer_t::disconnectAll() {
	for(size_t i = 0; i < m_userManager.getCount(); ++i) {
		if(m_userManager[i].isActive()) {
			this->disconnect(m_userManager[i]);
		}
	}
}

void gameServer_t::sendServerInfo() const {
	unsigned char serverLoad = static_cast<unsigned char>((static_cast<double>(m_userCount) / m_userManager.getCount()) * 100);

	eMUCore::packet_t packet;
	packet.construct(0xC1, 0x01);
	packet.insert<unsigned short>(3, m_serverConfiguration.m_serverCode);
	packet.insert<unsigned char>(5, serverLoad);
	m_udpSocket.send(m_serverConfiguration.m_connectServerHost,
						m_serverConfiguration.m_connectServerPort,
						packet.getData(),
						packet.getSize());
}

gameServerUser_t* gameServer_t::onContextAllocate() {
	gameServerUser_t *user = m_userManager.findFree();

	if(user == NULL) {
		m_logger.in(eMUCore::logger_t::_MESSAGE_ERROR) << "No free objects found.";
		m_logger.out();
	}

	return user;
}

void gameServer_t::onContextAttach(eMUCore::socketContext_t &context) {
	gameServerUser_t &user = reinterpret_cast<gameServerUser_t&>(context);

	try {
		m_logger.in(eMUCore::logger_t::_MESSAGE_INFO) << user << " Connected.";
		m_logger.out();

		// -----------------------
		++m_userCount;
		this->updateWindowTitle();
		// -----------------------

		// ---------------------
		user.setConnectionStamp(GetTickCount());
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

void gameServer_t::onContextReceive(eMUCore::socketContext_t &context) {
	gameServerUser_t &user = reinterpret_cast<gameServerUser_t&>(context);

	try {
		size_t parsedDataSize = 0;

		do {
			unsigned char *rawData = &user.getRecvBuffer().m_data[parsedDataSize];
			size_t rawDataSize = eMUCore::packet_t::getRawDataSize(rawData);

			if(eMUCore::packet_t::isRawDataCrypted(rawData)) {
				unsigned char decryptedBuff[eMUCore::c_ioDataMaxSize] = {0};
				size_t dataPtr = eMUCore::packet_t::getCryptedDataPointer(rawData);
				int decryptedBuffSize = m_crypt.decrypt(&decryptedBuff[dataPtr - 1], &rawData[dataPtr], rawDataSize - dataPtr);

				if(decryptedBuffSize > 0) {
					decryptedBuffSize += static_cast<int>(dataPtr);
					decryptedBuff[0] = rawData[0] - 2;

					if(rawData[0] == 0xC3) {
						decryptedBuff[1] = static_cast<unsigned char>(decryptedBuffSize);
					} else if(rawData[0] == 0xC4) {
						decryptedBuff[1] = HIBYTE(decryptedBuffSize);
						decryptedBuff[2] = LOBYTE(decryptedBuffSize);
					}

					m_crypt.extract(decryptedBuff, decryptedBuffSize);
					eMUCore::packet_t packet(decryptedBuff);

					m_logger.in(eMUCore::logger_t::_MESSAGE_PROTOCOL) << user << " Received " << packet << ".";
					m_logger.out();

					// -------------------------------
					m_game.parsePacket(user, packet);
					// -------------------------------
				} else {
					eMUCore::exception_t e;
					e.in() << __FILE__ << ":" << __LINE__ << " [gameServer_t::onContextReceive()] stream decryption error.";
					throw e;
				}
			} else {
				m_crypt.extract(rawData, rawDataSize);
				eMUCore::packet_t packet(rawData);

				m_logger.in(eMUCore::logger_t::_MESSAGE_PROTOCOL) << user << " Received " << packet << ".";
				m_logger.out();

				// -------------------------------
				m_game.parsePacket(user, packet);
				// -------------------------------
			}

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

void gameServer_t::onContextClose(eMUCore::socketContext_t &context) {
	gameServerUser_t &user = reinterpret_cast<gameServerUser_t&>(context);

	try {
		m_logger.in(eMUCore::logger_t::_MESSAGE_INFO) << user << " Disconnected.";
		m_logger.out();

		m_game.onClientClose(user);

		// -----------------------
		--m_userCount;
		this->updateWindowTitle();
		// -----------------------
	} catch(eMUCore::exception_t &e) {
		m_logger.in(eMUCore::logger_t::_MESSAGE_ERROR) << "Exception: " << user << " " << e.what();
		m_logger.out();
	} catch(std::exception &e) {
		m_logger.in(eMUCore::logger_t::_MESSAGE_ERROR) << "std exception: " << user << " " << e.what();
		m_logger.out();
	} catch(...) {
		m_logger.in(eMUCore::logger_t::_MESSAGE_ERROR) << "Exception: " << user << " Unknown exception.";
		m_logger.out();
	}
}

void gameServer_t::onTcpClientConnect() {
	m_logger.in(eMUCore::logger_t::_MESSAGE_INFO) << "Connected to dataserver " 
													<< m_tcpClient.getHostname() << ":" << m_tcpClient.getPort() << ".";
	m_logger.out();

	if(!m_packetQueue.empty()) {
		#ifdef _DEBUG
		m_logger.in(eMUCore::logger_t::_MESSAGE_DEBUG) << "Stored " << m_packetQueue.size() << " DataServer packets in queue.";
		m_logger.out();
		#endif

		std::for_each(m_packetQueue.begin(),
						m_packetQueue.end(),
						boost::bind(&gameServer_t::sendDataServer,
									this,
									_1));
	}

	this->updateWindowTitle();
}

void gameServer_t::onTcpClientReceive(unsigned char* data, size_t dataSize) {
	try {
		size_t parsedDataSize = 0;

		do {
			eMUCore::packet_t packet(&data[parsedDataSize]);

			m_logger.in(eMUCore::logger_t::_MESSAGE_PROTOCOL) << "[DataServer] Received " << packet << ".";
			m_logger.out();

			// -----------------------------------
			m_game.parseDataServerPacket(packet);
			// -----------------------------------

			parsedDataSize += packet.getSize();
		} while(parsedDataSize < dataSize);
	} catch(eMUCore::exception_t &e) {
		m_logger.in(eMUCore::logger_t::_MESSAGE_ERROR) << "Exception: [DataServer] " << e.what();
		m_logger.out();
	} catch(std::exception &e) {
		m_logger.in(eMUCore::logger_t::_MESSAGE_ERROR) << "std exception: [DataServer] " << e.what();
		m_logger.out();
	} catch(...) {
		m_logger.in(eMUCore::logger_t::_MESSAGE_ERROR) << "Unknown exception: [DataServer].";
		m_logger.out();
	}
}

void gameServer_t::onTcpClientClose() {
	m_logger.in(eMUCore::logger_t::_MESSAGE_INFO) << "Connection to dataserver closed.";
	m_logger.out();
	
	// Reconnect.
	if(!m_interrupt) {
		this->dataServerConnect();
	}
}

void gameServer_t::send(gameServerUser_t &user, eMUCore::packet_t &packet) {
	m_logger.in(eMUCore::logger_t::_MESSAGE_PROTOCOL) << user << " Sending " << packet << ".";
	m_logger.out();

	if(packet.isCrypted()) {
		unsigned char encryptedBuff[eMUCore::c_ioDataMaxSize] = {0};
		size_t dataPtr = packet.getHeaderSize() - 2;

		packet.setCryptSerial(user.generateCryptSerial());
		size_t encryptedBuffSize = m_crypt.encrypt(&encryptedBuff[dataPtr + 1],
													&packet.getData()[dataPtr],
													packet.getSize() - dataPtr) + dataPtr + 1;

		encryptedBuff[0] = packet.getHeaderId();

		if(packet.getHeaderId() == 0xC3) {
			encryptedBuff[1] = encryptedBuffSize;
		} else if(packet.getHeaderId() == 0xC4) {
			encryptedBuff[1] = HIBYTE(encryptedBuffSize);
			encryptedBuff[2] = LOBYTE(encryptedBuffSize);
		}

		m_iocpEngine.write(user, encryptedBuff, encryptedBuffSize);
	} else {
		m_iocpEngine.write(user, packet.getData(), packet.getSize());
	}
}

void gameServer_t::sendDataServer(const eMUCore::packet_t &packet) {
	if(m_tcpClient.isActive()) {
		m_logger.in(eMUCore::logger_t::_MESSAGE_PROTOCOL) << "[DataServer] Sending " << packet << ".";
		m_logger.out();

		m_tcpClient.send(packet.getData(), packet.getSize());
	}
	else {
		m_packetQueue.push_back(packet);
	}
}

void gameServer_t::fatalHandler(int signalId) {
	//eMUCore::stackTrace_t::dumpCallTree("log\\stackTrace.txt");
}

DWORD WINAPI gameServer_t::consoleEventHandler(unsigned int eventId) {
	if(!gameServer_t::m_interrupt) {
		int ret = MessageBox(0, "Do you really want to quit the server?", "eMU::GameServer", MB_ICONQUESTION | MB_YESNO);
		if(ret == IDYES) {
			gameServer_t::stop();
		}
	}

    return TRUE;
}

void main(int argsCount, char *argsVect[]) {
	signal(SIGABRT, gameServer_t::fatalHandler);
	signal(SIGFPE, gameServer_t::fatalHandler);
	signal(SIGILL, gameServer_t::fatalHandler);
	signal(SIGSEGV, gameServer_t::fatalHandler);
	signal(SIGTERM, gameServer_t::fatalHandler);

	srand(GetTickCount());

	//system("COLOR 0F");
	std::cout << "		+-----------------------------------------------+" << std::endl;
	std::cout << "		|						|" << std::endl;
	std::cout << "		|		eMU :: GameServer		|"  << std::endl;
	std::cout << "		|		 Build: " << __DATE__ << "		|" << std::endl;
	std::cout << "		|						|" << std::endl;
	std::cout << "		+-----------------------------------------------+" << std::endl << std::endl;

	try {
		if(SetConsoleCtrlHandler(reinterpret_cast<PHANDLER_ROUTINE>(gameServer_t::consoleEventHandler), TRUE) != 0) {
			WSADATA wsaData = {0};
			if(WSAStartup(MAKEWORD(2, 2), &wsaData) == 0) {
				if(argsCount == 4) {
					gameServer_t *gameServer = new gameServer_t(boost::lexical_cast<size_t>(argsVect[1]),
																boost::lexical_cast<size_t>(argsVect[2]),
																boost::lexical_cast<unsigned short>(argsVect[3]));
					gameServer->startup();
					gameServer->worker();
					delete gameServer;
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