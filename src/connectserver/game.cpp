#include "game.h"
#include <boost\bind.hpp>

void game_t::startup() {
	m_serverList.startup("serverList.xml");

	m_scheduler.insert(eMUCore::scheduler_t::_SCHEDULE_SYNCHRONIZED,
						boost::bind(&serverList_t::update, &m_serverList),
						1);
}

void game_t::onHandshake(connectServerUser_t &user) {
	eMUCore::packet_t packet;
	m_protocol.constructHandshake(packet);
	m_networkInterface.send(user, packet);
}

void game_t::onServerListRequest(connectServerUser_t &user) {
	if(m_serverList.getActiveServersCount() > 0) {
		eMUCore::packet_t packet;
		m_protocol.constructServerListAnswer(packet, m_serverList.getList());
		m_networkInterface.send(user, packet);
	} else {
		m_logger.in(eMUCore::logger_t::_MESSAGE_WARNING) << user << " No active gameservers.";
		m_logger.out();

		m_networkInterface.disconnect(user);
	}
}

void game_t::onServerSelectRequest(connectServerUser_t &user, unsigned short serverCode) {
	#ifdef _DEBUG
	m_logger.in(eMUCore::logger_t::_MESSAGE_DEBUG) << user << " Selected serverCode [" << serverCode << "].";
	m_logger.out();
	#endif

	const serverList_t::serverAttributes_t &attr = m_serverList.getServerAttributes(serverCode);

	m_logger.in(eMUCore::logger_t::_MESSAGE_INFO) << user << " Selected server " << attr << ".";
	m_logger.out();

	eMUCore::packet_t packet;
	m_protocol.constructServerSelectAnswer(packet, attr);
	m_networkInterface.send(user, packet);
}