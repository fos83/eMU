#include "core.h"

using namespace eMUCore;

eMUCORE_DECLSPEC bool eMUCore::isIpAddress(std::string hostname) {
	if(inet_addr(hostname.c_str()) != INADDR_NONE) {
		return true;
	} else {
		return false;
	}
}

eMUCORE_DECLSPEC std::string eMUCore::convertToIpAddress(std::string hostname) {
	hostent *remote = gethostbyname(hostname.c_str());

	if(remote != NULL) {
		return inet_ntoa(*reinterpret_cast<in_addr*>(remote->h_addr_list[0]));
	} else {
		return "";
	}
}

void iocpEngine_t::startup() {
	m_ioCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, NULL, 0);

	if(m_ioCompletionPort != NULL) {
		SYSTEM_INFO systemInfo = {0};
		GetSystemInfo(&systemInfo);

		m_workerThreadCount = systemInfo.dwNumberOfProcessors * 2;
		m_workerThread = new HANDLE[m_workerThreadCount];
		memset(m_workerThread, NULL, sizeof(m_workerThread) * m_workerThreadCount);

		for(size_t i = 0; i < m_workerThreadCount; ++i) {
				m_workerThread[i] = CreateThread(NULL,
													0,
													reinterpret_cast<LPTHREAD_START_ROUTINE>(iocpEngine_t::worker),
													reinterpret_cast<LPVOID>(this),
													0,
													NULL);

				if(m_workerThread[i] == NULL) {
					exception_t e;
					e.in() << __FILE__ << ":" << __LINE__ << "[iocpEngine_t::iocpEngine_t()] Could not create worker thread. Error #" << GetLastError() << ".";
					throw e;
				}
		}
	} else {
		exception_t e;
		e.in() << __FILE__ << ":" << __LINE__ << "[iocpEngine_t::iocpEngine_t()] Could not create completion port. Error #" << GetLastError() << ".";
		throw e;
	}
}

void iocpEngine_t::attach(socketContext_t &context) const {
	m_synchronizer.lock();

	if(CreateIoCompletionPort(reinterpret_cast<HANDLE>(context.getSocket()),
								m_ioCompletionPort,
								reinterpret_cast<ULONG_PTR>(&context),
								0) == m_ioCompletionPort) {
		unsigned int flags = 0;

		if(WSARecv(context.getSocket(),
					&context.getRecvBuffer().m_wsaBuff,
					1,
					NULL,
					reinterpret_cast<LPDWORD>(&flags),
					reinterpret_cast<LPWSAOVERLAPPED>(&context.getRecvBuffer()),
					NULL) == SOCKET_ERROR) {
			int wsaLastError = WSAGetLastError();

			if(wsaLastError != WSA_IO_PENDING) {
				m_logger.in(logger_t::_MESSAGE_ERROR) << "[iocpEngine_t::attach()][" << context.getIpAddress() << ":" << context.getPort() << "]"
															<< " WSARecv() failed with error#" << wsaLastError << ".";
				m_logger.out();
				// Serwer nie wie, ze obiekt zostal polaczony, wiec wystarczy go tylko zamknac bez uruchamiania OnClose() callback.
				context.close();
			} else {
				context.setActive();
				context.onAttach();
			}
		} else {
			context.setActive();
			context.onAttach();
		}
	} else {
		m_logger.in(logger_t::_MESSAGE_ERROR) << "[iocpEngine_t::attach()][" << context.getIpAddress() << ":" << context.getPort() << "]"
												<< " Attach socket context to completion port failed with error#" << GetLastError() << ".";
		m_logger.out();
		// Serwer nie wie, ze obiekt zostal polaczony, wiec wystarczy tylko zamknac bez uruchamiania OnClose() callback.
		context.close();
	}

	m_synchronizer.unlock();
}

void iocpEngine_t::detach(socketContext_t &context) const {
	if(PostQueuedCompletionStatus(m_ioCompletionPort,
									0,
									reinterpret_cast<ULONG_PTR>(&context),
									reinterpret_cast<LPOVERLAPPED>(&context.getRecvBuffer())) == FALSE) {
		m_logger.in(logger_t::_MESSAGE_ERROR) << "[iocpEngine_t::detach()][" << context.getIpAddress() << ":" << context.getPort() << "]"
												<< " Detach socket context from completion port failed with error#" << GetLastError() << ".";
		m_logger.out();
		// ----------------
		context.onClose();
		context.close();
	}
}

void iocpEngine_t::write(socketContext_t &context, const unsigned char *data, size_t dataSize) const {
	m_synchronizer.lock();
	ioSendBuffer_t &sendBuffer = context.getSendBuffer();

	if(!sendBuffer.m_dataLocked) {
		if(dataSize <= c_ioDataMaxSize) {
			sendBuffer.clearData();
			memcpy(sendBuffer.m_data, data, dataSize);
			sendBuffer.m_dataSize = dataSize;
			sendBuffer.m_wsaBuff.len = dataSize;

			size_t sentSize = 0;

			if(WSASend(context.getSocket(),
						&sendBuffer.m_wsaBuff,
						1,
						reinterpret_cast<LPDWORD>(&sentSize),
						0,
						reinterpret_cast<LPOVERLAPPED>(&sendBuffer),
						NULL) == SOCKET_ERROR) {
				unsigned int wsaLastError = WSAGetLastError();

				if(wsaLastError != WSA_IO_PENDING) {
					m_logger.in(logger_t::_MESSAGE_ERROR) << "[iocpEngine_t::write()][" << context.getIpAddress() << ":" << context.getPort() << "]"
															<< " WSASend() failed with error#" << wsaLastError << ".";
					m_logger.out();
					this->detach(context);
				} else {
					sendBuffer.m_dataLocked = true;
				}
			} else {
				sendBuffer.m_dataLocked = false;
			}
		} else {
			m_logger.in(logger_t::_MESSAGE_ERROR) << "[iocpEngine_t::write()][" << context.getIpAddress() << ":" << context.getPort() << "]"
													<< "I/O buffer is overflowed.";
			m_logger.out();
			this->detach(context);
		}
	} else {
		if(sendBuffer.m_secondDataSize + dataSize <= c_ioDataMaxSize) {
			sendBuffer.mergeSecondData(data, dataSize);
		} else {
			m_logger.in(logger_t::_MESSAGE_ERROR) << "[iocpEngine_t::write()][" << context.getIpAddress() << ":" << context.getPort() << "]"
													<< "I/O buffer is overflowed.";
			m_logger.out();
			this->detach(context);
		}
	}

	m_synchronizer.unlock();
}

void iocpEngine_t::dequeueReceive(socketContext_t &context) const {
	if(context.getRecvBuffer().m_dataSize > 0) {
		context.onReceive();

		unsigned int flags = 0;

		if(WSARecv(context.getSocket(),
					&context.getRecvBuffer().m_wsaBuff,
					1,
					NULL,
					reinterpret_cast<LPDWORD>(&flags),
					reinterpret_cast<LPWSAOVERLAPPED>(&context.getRecvBuffer()),
					NULL) == SOCKET_ERROR) {
			int wsaLastError = WSAGetLastError();

			if(wsaLastError != WSA_IO_PENDING) {
				m_logger.in(logger_t::_MESSAGE_ERROR) << "[iocpEngine_t::dequeueReceive()][" << context.getIpAddress() << ":" << context.getPort() << "]"
														<< " WSARecv() failed with error#" << wsaLastError << ".";
				m_logger.out();
				this->detach(context);
			}
		}
	} else if(context.getRecvBuffer().m_dataSize == 0) {
		// ----------------
		if(context.isActive()) {
			context.onClose();
			context.close();
		}
	}
}

void iocpEngine_t::dequeueSend(socketContext_t &context) const {
	ioSendBuffer_t &sendBuffer = context.getSendBuffer();

	if(sendBuffer.m_secondDataSize > 0) {
		sendBuffer.clearData();
		sendBuffer.moveSecondData();
		sendBuffer.clearSecondData();

		size_t sentSize = 0;

		if(WSASend(context.getSocket(),
					&sendBuffer.m_wsaBuff,
					1,
					reinterpret_cast<LPDWORD>(&sentSize),
					0,
					reinterpret_cast<LPOVERLAPPED>(&sendBuffer),
					NULL) == SOCKET_ERROR) {
			unsigned int wsaLastError = WSAGetLastError();

			if(wsaLastError != WSA_IO_PENDING) {
				m_logger.in(logger_t::_MESSAGE_ERROR) << "[iocpEngine_t::dequeueSend()][" << context.getIpAddress() << ":" << context.getPort() << "]"
														<< " WSASend() failed with error#" << wsaLastError << ".";
				m_logger.out();
				this->detach(context);
			} else {
				sendBuffer.m_dataLocked = true;
			}
		} else {
			sendBuffer.m_dataLocked = false;
		}
	} else {
		sendBuffer.m_dataLocked = false;
	}
}

void iocpEngine_t::dequeueError(socketContext_t &context, int lastError) const {
	if(lastError != ERROR_NETNAME_DELETED
		&& lastError != ERROR_CONNECTION_ABORTED
		&& lastError != ERROR_OPERATION_ABORTED) {
		m_logger.in(logger_t::_MESSAGE_ERROR) << "[iocpEngine_t::worker()] GetQueuedCompletionStatus() failed with error# " << lastError 
																		<< " but dequeued completion packet."; m_logger.out();
	}

	// ---------------------------------
	// Error - check is context cleaned.
	if(context.isActive()) {
		context.onClose();
		context.close();
	}
}

DWORD iocpEngine_t::worker(iocpEngine_t *instance) {
	while(true) {
		socketContext_t *context = NULL;
		ioBuffer_t *buffer = NULL;
		size_t transferredBytes = 0;

		BOOL completionStatus = GetQueuedCompletionStatus(instance->m_ioCompletionPort,
															reinterpret_cast<LPDWORD>(&transferredBytes),
															reinterpret_cast<PULONG_PTR>(&context),
															reinterpret_cast<LPOVERLAPPED*>(&buffer),
															INFINITE);
		if(completionStatus) {
			if(context == NULL) {
				break;
			}

			instance->m_synchronizer.lock();

			if(buffer->m_type == ioBuffer_t::_IO_RECV_BUFFER) {
				buffer->m_dataSize = transferredBytes;
				instance->dequeueReceive(*context);
			} else if(buffer->m_type == ioBuffer_t::_IO_SEND_BUFFER) {
				instance->dequeueSend(*context);
			}
			else {
				instance->m_logger.in(logger_t::_MESSAGE_ERROR) << "[iocpEngine_t::worker()][" 
																<< context->getIpAddress() << ":" << context->getPort()
																<< "] Invalid buffer type " << buffer->m_type << ".";
				instance->m_logger.out();
			}

			instance->m_synchronizer.unlock();

		} else {
			int lastError = GetLastError();

			if(buffer != NULL) {
				instance->m_synchronizer.lock();
				instance->dequeueError(*context, lastError);
				instance->m_synchronizer.unlock();
			} else {
				instance->m_logger.in(logger_t::_MESSAGE_ERROR) << "[iocpEngine_t::worker()] GetQueuedCompletionStatus() failed with error# " << lastError 
																	<< " without dequeue completion packet.";
				instance->m_logger.out();
			}
		}
	}

	return 0;
}

void tcpServer_t::startup() {
	m_socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, NULL, WSA_FLAG_OVERLAPPED);

	if(m_socket != INVALID_SOCKET) {
		sockaddr_in inetAddr = {0};
		inetAddr.sin_family = AF_INET;
		inetAddr.sin_addr.S_un.S_addr = htonl(ADDR_ANY);
		inetAddr.sin_port = htons(m_listenPort);

		if(bind(m_socket, reinterpret_cast<sockaddr*>(&inetAddr), sizeof(inetAddr)) == 0) {
			if(listen(m_socket, SOMAXCONN) == 0) {
				m_acceptWorkerThread = CreateThread(NULL,
													0,
													reinterpret_cast<LPTHREAD_START_ROUTINE>(tcpServer_t::worker),
													reinterpret_cast<LPVOID>(this),
													0,
													NULL);
				if(m_acceptWorkerThread == NULL) {
					exception_t e;
					e.in() << __FILE__ << ":" << __LINE__ << "[tcpServer_t::tcpServer_t()] Could not create accept worker thread. Error #" << GetLastError() << ".";
					throw e;
				}
			} else {
				exception_t e;
				e.in() << __FILE__ << ":" << __LINE__ << "[tcpServer_t::tcpServer_t()] Could not listen on socket. Error #" << WSAGetLastError() << ".";
				throw e;
			}
		} else {
			exception_t e;
			e.in() << __FILE__ << ":" << __LINE__ << "[tcpServer_t::tcpServer_t()] Could not bind socket. Error #" << WSAGetLastError() << ".";
			throw e;
		}
	} else {
		exception_t e;
		e.in() << __FILE__ << ":" << __LINE__ << "[tcpServer_t::tcpServer_t()] Could not create listen socket. Error #" << WSAGetLastError() << ".";
		throw e;
	}
}

DWORD tcpServer_t::worker(tcpServer_t *instance) {
	while(true) {
		sockaddr_in incomingInetAddr = {0};
		size_t incomingInetAddrSize = sizeof(incomingInetAddr);

		SOCKET incomingSocket = WSAAccept(instance->m_socket,
											reinterpret_cast<sockaddr*>(&incomingInetAddr),
											reinterpret_cast<LPINT>(&incomingInetAddrSize),
											NULL,
											NULL);

		if(incomingSocket != INVALID_SOCKET) {
			instance->m_logger.in(logger_t::_MESSAGE_INFO) << "Connection attempt from [" << inet_ntoa(incomingInetAddr.sin_addr) << "].";
			instance->m_logger.out();

			socketContext_t *context = instance->m_onAllocate();

			if(context != NULL) {
				context->setSocket(incomingSocket);
				context->setIpAddress(inet_ntoa(incomingInetAddr.sin_addr));
				context->setPort(instance->m_listenPort);
				instance->m_iocpEngine.attach(*context);
			} else {
				instance->m_logger.in(logger_t::_MESSAGE_ERROR) << "[tcpServer_t::worker()] got NULL socket context.";
				instance->m_logger.out();

				closesocket(incomingSocket);
			}
		} else {
			instance->m_logger.in(logger_t::_MESSAGE_ERROR) << "[tcpServer_t::worker()] WSAAccept failed with error#" << WSAGetLastError << ".";
			instance->m_logger.out();			
		}
	}

	return 0;
}

bool tcpClient_t::connect(const std::string &hostname, unsigned short port) throw(eMUCore::exception_t) {
	m_socket = WSASocket(AF_INET,
							SOCK_STREAM,
							IPPROTO_TCP,
							NULL,
							NULL,
							WSA_FLAG_OVERLAPPED);

	if(m_socket != INVALID_SOCKET) {
		sockaddr_in inetAddr = {0};
		inetAddr.sin_family = AF_INET;
		inetAddr.sin_port = htons(port);
		inetAddr.sin_addr.S_un.S_addr = inet_addr(hostname.c_str());

		if(inetAddr.sin_addr.S_un.S_addr == INADDR_NONE) {
			hostent *h = gethostbyname(hostname.c_str());
			if(h != NULL) {
				inetAddr.sin_addr.S_un.S_addr = *(reinterpret_cast<u_long*>(h->h_addr_list[0]));
			} else {
				exception_t e;
				e.in() << __FILE__ << ":" << __LINE__ << "[tcpClient_t::connect()] Invalid hostname " << hostname << " specified.";
				throw e;
			}
		}

		if(::connect(m_socket, (sockaddr*)&inetAddr, sizeof(inetAddr)) != SOCKET_ERROR) {
			m_hostname = hostname;
			m_port = port;
			m_active = true;
			m_onConnect();
			return true;
		} else {
			unsigned int wsaLastError = WSAGetLastError();

			if(wsaLastError != WSAETIMEDOUT 
				&& wsaLastError != WSAECONNREFUSED 
				&& wsaLastError != WSAECONNRESET) {
				exception_t e;
				e.in() << __FILE__ << ":" << __LINE__ << "[tcpClient_t::connect()] ::connect() failed with error#" << WSAGetLastError() << ".";
				throw e;
			} else {
				return false;
			}
		}

	} else {
		exception_t e;
		e.in() << __FILE__ << ":" << __LINE__ << "[tcpClient_t::connect()] Could not create client socket. Error #" << WSAGetLastError() << ".";
		throw e;
	}
}

void tcpClient_t::close() {
	if(m_socket != INVALID_SOCKET) {
		closesocket(m_socket);
		m_socket = INVALID_SOCKET;
		m_hostname = "";
		m_port = 0;
		m_active = false;
	}
}

void tcpClient_t::send(const unsigned char *data, size_t dataSize) const {
	size_t sentSize = 0;

	do {
		int ret = ::send(m_socket, reinterpret_cast<const char*>(&data[sentSize]), static_cast<int>(dataSize - sentSize), 0);

		if(ret != SOCKET_ERROR) {
			sentSize += ret;
		} else {
			m_logger.in(logger_t::_MESSAGE_ERROR) << "[tcpClient_t::send()] send() failed with error#" << WSAGetLastError() << "."; m_logger.out();
			break;
		}
	} while(sentSize < dataSize);
}

void tcpClient_t::worker() {
	if(m_socket != INVALID_SOCKET) {
		timeval tv = {0, 1};
		fd_set fdSet = {0};
		FD_ZERO(&fdSet);
		FD_SET(m_socket, &fdSet);
		
		int ret = select(1, &fdSet, NULL, NULL, &tv);

		if(ret > 0) {
			if(FD_ISSET(m_socket, &fdSet)) {
				unsigned char buffer[c_ioDataMaxSize];

				int ret = recv(m_socket, reinterpret_cast<char*>(buffer), c_ioDataMaxSize, 0);

				if(ret > 0)	{
					m_synchronizer.lock();
					m_onReceive(buffer, ret);
					m_synchronizer.unlock();
				} else if(ret == 0) {
					this->close();
					m_onClose();
				} else {
					int wsaLastError = WSAGetLastError();

					if(wsaLastError != WSAECONNRESET) {
						m_logger.in(logger_t::_MESSAGE_ERROR) << "[tcpClient_t::worker()] recv() failed with error#" << wsaLastError << ".";
						m_logger.out();
					}

					this->close();
					m_onClose();			
				}
			}
		} else if(ret == SOCKET_ERROR) {
			int wsaLastError = WSAGetLastError();

			if(wsaLastError != WSAECONNRESET) {
				m_logger.in(logger_t::_MESSAGE_ERROR) << "[tcpClient_t::worker()] select() failed with error#" << wsaLastError << ".";
				m_logger.out();
			} else {
				this->close();
				m_onClose();
			}
		}
	}
}

void udpSocket_t::startup() {
	m_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	if(m_socket != INVALID_SOCKET) {
		sockaddr_in inetAddr = {0};
		inetAddr.sin_family	= AF_INET;
		inetAddr.sin_port = htons(m_port);
		inetAddr.sin_addr.S_un.S_addr = htonl(ADDR_ANY);

		if(bind(m_socket, reinterpret_cast<sockaddr*>(&inetAddr), sizeof(inetAddr)) != 0) {
			exception_t e;
			e.in() << __FILE__ << ":" << __LINE__ << "[udpSocket_t::udpSocket_t()] Could not bind udp socket. Error #" << WSAGetLastError() << ".";
			throw e;
		}
	} else {
		exception_t e;
		e.in() << __FILE__ << ":" << __LINE__ << "[udpSocket_t::udpSocket_t()] Could not create udp socket. Error #" << WSAGetLastError() << ".";
		throw e;
	}
}

void udpSocket_t::worker() const {
	timeval tv = {0, 1};
	fd_set fdSet = {0};
	FD_ZERO(&fdSet);
	FD_SET(m_socket, &fdSet);
	
	int ret = select(1, &fdSet, NULL, NULL, &tv);

	if(ret > 0) {
		if(FD_ISSET(m_socket, &fdSet)) {
			unsigned char buffer[c_ioDataMaxSize];

			sockaddr_in inetAddr;
			size_t inetAddrSize = sizeof(inetAddr);

			int ret = recvfrom(m_socket,
								reinterpret_cast<char*>(buffer),
								c_ioDataMaxSize,
								0,
								reinterpret_cast<sockaddr*>(&inetAddr),
								reinterpret_cast<int*>(&inetAddrSize));

			if(ret > 0)	{
				m_onReceive(inetAddr, buffer, ret);
			} else if(ret == SOCKET_ERROR) {
				m_logger.in(logger_t::_MESSAGE_ERROR) << "[udpSocket_t::worker()] recvfrom() failed with error#" << WSAGetLastError() << "."; m_logger.out();
			}
		}
	} else if(ret == SOCKET_ERROR) {
		m_logger.in(logger_t::_MESSAGE_ERROR) << "[udpSocket_t::worker()] select() failed with error#" << WSAGetLastError() << "."; m_logger.out();
	}
}

void udpSocket_t::send(std::string hostname,
				unsigned short port,
				const unsigned char *buffer,
				size_t bufferSize) const {
	sockaddr_in receiver = {0};
	receiver.sin_family = AF_INET;
	receiver.sin_port = htons(port);

	if(isIpAddress(hostname)) {
		receiver.sin_addr.S_un.S_addr = inet_addr(hostname.c_str());
	} else {
		std::string ipAddress = convertToIpAddress(hostname);

		if(ipAddress != "") {
			receiver.sin_addr.S_un.S_addr = inet_addr(ipAddress.c_str());
		} else {
			m_logger.in(logger_t::_MESSAGE_ERROR)  << "[udpSocket_t::send()] Invalid hostname " << hostname << " specified."; m_logger.out();
			return;
		}
	}

	size_t sentSize = 0;

	do {
		int ret = sendto(m_socket,
							reinterpret_cast<const char*>(&buffer[sentSize]),
							static_cast<int>(bufferSize - sentSize),
							0,
							reinterpret_cast<const sockaddr*>(&receiver),
							sizeof(receiver));

		if(ret != SOCKET_ERROR) {
			sentSize += ret;
		} else {
			m_logger.in(logger_t::_MESSAGE_ERROR) << "[udpSocket_t::send()] sendto() failed with error#" << WSAGetLastError() << "."; m_logger.out();
			break;
		}
	} while(sentSize < bufferSize);
}