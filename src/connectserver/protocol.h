#ifndef eMU_CONNECTSERVER_PROTOCOL_H
#define eMU_CONNECTSERVER_PROTOCOL_H

#include "..\core\core.h"
#include "user.h"
#include "serverlist.h"

class protocolExecutorInterface_t {
public:
	virtual ~protocolExecutorInterface_t() {}
	virtual void onServerListRequest(connectServerUser_t &user) = 0;
	virtual void onServerSelectRequest(connectServerUser_t &user, 
										unsigned short serverCode) = 0;
};

class protocol_t {
public:
	protocol_t(protocolExecutorInterface_t &iface):
	  m_executorInterface(iface) {}

	void core(connectServerUser_t &user, 
				const eMUCore::packet_t &packet) const;

	void constructHandshake(eMUCore::packet_t &buff) const;

	void parseServerListRequest(connectServerUser_t &user, 
									const eMUCore::packet_t &packet) const;

	void constructServerListAnswer(eMUCore::packet_t &buff, 
									const serverList_t::serverAttributesList_t &list) const;

	void parseServerSelectRequest(connectServerUser_t &user, 
									const eMUCore::packet_t &packet) const;

	void constructServerSelectAnswer(eMUCore::packet_t &buff, 
										const serverList_t::serverAttributes_t &attr) const;

private:
	protocol_t();
	protocol_t(const protocol_t&);
	protocol_t& operator=(const protocol_t&);

	protocolExecutorInterface_t &m_executorInterface;
};

#endif // eMU_CONNECTSERVER_PROTOCOL_H