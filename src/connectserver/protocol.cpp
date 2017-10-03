#include "protocol.h"

void protocol_t::core(connectServerUser_t &user,
						const eMUCore::packet_t &packet) const {
	switch(packet.getProtocolId()) {
	case 0xF4:
		switch(packet.getData()[3]) {
		case 0x03: // server details.
			this->parseServerSelectRequest(user, packet);
			break;

		case 0x06: // servers list.
			this->parseServerListRequest(user, packet);
			break;
		}
		break;
	}
}

void protocol_t::constructHandshake(eMUCore::packet_t &buff) const {
	buff.construct(0xC1, 0x00);
	buff.insert<unsigned char>(3, 0x01);
}

void protocol_t::parseServerListRequest(connectServerUser_t &user, 
											const eMUCore::packet_t &packet) const {
	unsigned char check = packet.read<unsigned char>(3);

	if(check != 0) {
		m_executorInterface.onServerListRequest(user);
	}
}

void protocol_t::constructServerListAnswer(eMUCore::packet_t &buff, 
											const serverList_t::serverAttributesList_t &list) const {
	//[0] HeaderId: 0xC2
	//[1] Size
	//[2] Size
	//[3] ProtocolId: 0xF4
	//[4] SubProtocolId: 0x06
	//[5] Count
	//[6] Count
	// --- Repeated block.
	//[7] ServerCode
	//[8] ServerCode
	//[9] Load
	//[10] Unknown
	// ------------------
	buff.construct(0xC2, 0xF4);
	buff.insert<unsigned char>(4, 0x06);

	size_t serversCount = 0;

	for(serverList_t::serverAttributesList_t::const_iterator i = list.begin(); i != list.end(); ++i) {
		const serverList_t::serverAttributes_t &attr = i->second;

		unsigned short step = serversCount * 4;

		if(attr.m_active) {
			buff.insert<unsigned short>(7 + step, attr.m_code);
			buff.insert<unsigned char>(9 + step, attr.m_load);
			buff.insert<unsigned char>(10 + step, 0x00);
			++serversCount;
		}
	}

	// WTF: Servers count as big endian?
	buff.insert<unsigned short>(5, _byteswap_ushort(serversCount));
}

void protocol_t::parseServerSelectRequest(connectServerUser_t &user, 
											const eMUCore::packet_t &packet) const {
	unsigned short serverCode = packet.read<unsigned short>(4);
	m_executorInterface.onServerSelectRequest(user, serverCode);
}

void protocol_t::constructServerSelectAnswer(eMUCore::packet_t &buff, 
												  const serverList_t::serverAttributes_t &attr) const {
	//[0] HeaderId: 0xC1
	//[1] Size
	//[2] ProtocolId: 0xF4
	//[3] SubProtocolId: 0x03
	//[4]-[19] IpAddress (16 bytes)
	//[20] Port
	//[21] Port

	buff.construct(0xC1, 0xF4);
	buff.insert<unsigned char>(3, 0x03);

	if(!eMUCore::isIpAddress(attr.m_address)) {
		std::string ipAddress = eMUCore::convertToIpAddress(attr.m_address);
		buff.insertString(4, ipAddress, 16);
	} else {
		buff.insertString(4, attr.m_address, 16);
	}

	buff.insert<unsigned short>(20, attr.m_port);
}