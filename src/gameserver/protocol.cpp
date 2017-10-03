#include "protocol.h"
#include <iostream>

void protocol_t::core(gameServerUser_t &user,
						const eMUCore::packet_t &packet) const {
	switch(packet.getProtocolId()) {
	case 0xF1:
		switch(packet.getData()[3]) {
		case 0x01:
			this->parseLoginRequest(user, packet);
			break;
		case 0x02:
			this->parseLogoutRequest(user, packet);
			break;
		}
		break;

	case 0xF3:
		switch(packet.getData()[3]) {
		case 0x00:
			this->parseCharacterListRequest(user, packet);
			break;

		case 0x01:
			this->parseCharacterCreateRequest(user, packet);
			break;

		case 0x02:
			this->parseCharacterDeleteRequest(user, packet);
			break;

		case 0x03:
			this->parseCharacterSelectRequest(user, packet);
			break;
		}
		break;

	case 0xD7:
		this->parseCharacterMoveRequest(user, packet);
		break;

	case 0x1C:
		this->parseCharacterTeleportRequest(user, packet);
		break;
	}
}

void protocol_t::constructHandshake(eMUCore::packet_t &buff, unsigned short userIndex, std::string version) const {
	//[0] HeaderId: 0xC1
	//[1] Size
	//[2] ProtocolId: 0xF1
	//[3] SubProtocolId: 0x00
	//[4] Result
	//[5] Index
	//[6] Index
	//[7] Version without '.'
	// ------------------

	buff.construct(0xC1, 0xF1);
	buff.insert<unsigned char>(3, 0x00);
	buff.insert<unsigned char>(4, 0x01);
	buff.insert<unsigned short>(5, _byteswap_ushort(userIndex));
	buff.insertString(7, version, 5);
}

void protocol_t::parseLoginRequest(gameServerUser_t &user, 
								   const eMUCore::packet_t &packet) const {
	// [0] HeaderId: 0xC3
	// [1] Size
	// [2] ProtocolId: 0xF1
	// [3] SubProtocolId: 0x01
	// [4] AccountId
	// [14] Password
	// [24] TickCount
	// [28] ClientExeVersion
	// [33] ClientExeSerial

	std::string accountId = this->xorString(packet.readString(4, 10));
	std::string password = this->xorString(packet.readString(14, 10));
	std::string clientExeVersion = packet.readString(28, 5);
	std::string clientExeSerial = packet.readString(33, 16);

	m_executorInterface.onLoginRequest(user,
										accountId,
										password,
										clientExeVersion,
										clientExeSerial);
}

void protocol_t::constructLoginAnswer(eMUCore::packet_t &buff, 
											unsigned char result) const {
	// [0] HeaderId: 0xC1
	// [1] Size
	// [2] ProtocolId: 0xF1
	// [3] SubProtocolId: 0x01
	// [4] Result
	buff.construct(0xC1, 0xF1);
	buff.insert<unsigned char>(3, 0x01);
	buff.insert<unsigned char>(4, result);
}

void protocol_t::parseCharacterListRequest(gameServerUser_t &user, 
											const eMUCore::packet_t &packet) const {
	m_executorInterface.onCharacterListRequest(user);
}

void protocol_t::constructCharacterListAnswer(eMUCore::packet_t &buff,
											   unsigned char availableRaces,
											   const eMUShared::characterList_t &characterList) const {
	buff.construct(0xC1, 0xF3);
	buff.insert<unsigned char>(3, 0x00); // SubProtocolId.
	buff.insert<unsigned char>(4, availableRaces);
	buff.insert<unsigned char>(5, 0); // ???
	buff.insert<unsigned char>(6, characterList.size());
	

	for(size_t i = 0; i < characterList.size(); ++i) {
		size_t step = i * 34;

		buff.insert<unsigned char>(7 + step, i);
		buff.insertString(8 + step, characterList[i].m_name, 10);
		buff.insert<unsigned char>(18 + step, 0x00); // ???
		buff.insert<unsigned short>(19 + step, characterList[i].m_level);
		buff.insert<unsigned char>(21 + step, characterList[i].m_controlCode);

		unsigned char protoRace = ((characterList[i].m_race >> 4) << 5) & 0xE0;
		protoRace |= ((characterList[i].m_race & 0x07) << 4) & 0x10;
		buff.insert<unsigned char>(22 + step, protoRace);
		buff.insert<unsigned char>(23 + step, 0xFF);
		buff.insert<unsigned char>(24 + step, 0xFF);
		buff.insert<unsigned char>(25 + step, 0xFF);
		buff.insert<unsigned char>(26 + step, 0xFF);
		buff.insert<unsigned char>(27 + step, 0xFF);
		buff.insert<unsigned char>(28 + step, 0x00);
		buff.insert<unsigned char>(29 + step, 0x00);
		buff.insert<unsigned char>(30 + step, 0x00);
		buff.insert<unsigned char>(31 + step, 0xF8);
		buff.insert<unsigned char>(32 + step, 0x00);
		buff.insert<unsigned char>(33 + step, 0x00);
		buff.insert<unsigned char>(34 + step, 0xF0);
		buff.insert<unsigned char>(35 + step, 0xFF);
		buff.insert<unsigned char>(36 + step, 0xFF);
		buff.insert<unsigned char>(37 + step, 0xFF);
		buff.insert<unsigned char>(38 + step, 0x00);
		buff.insert<unsigned char>(39 + step, 0x00);
		buff.insert<unsigned char>(40 + step, 0xFF); // guild status.
	}
}

void protocol_t::parseLogoutRequest(gameServerUser_t &user,
									const eMUCore::packet_t &packet) const {
	unsigned char closeReason = packet.read<unsigned char>(4);
	m_executorInterface.onLogoutRequest(user, closeReason);
}

void protocol_t::constructLogoutAnswer(eMUCore::packet_t &buff,
									   unsigned char closeReason) const {
	buff.construct(0xC3, 0xF1);
	buff.insert<unsigned char>(3, 0x02); // SubProtocolId.
	buff.insert<unsigned char>(4, closeReason);
}

void protocol_t::parseCharacterCreateRequest(gameServerUser_t &user,
												const eMUCore::packet_t &packet) const {
	std::string name = packet.readString(4, 10);
	unsigned char race = packet.read<unsigned char>(14);

	m_executorInterface.onCharacterCreateRequest(user, name, race);
}

void protocol_t::constructCharacterCreteAnswer(eMUCore::packet_t &buff,
												unsigned char result,
												const std::string &name,
												int slot,
												unsigned short level,
												unsigned char race) const {
	buff.construct(0xC1, 0xF3);
	buff.insert<unsigned char>(3, 0x01);
	buff.insert<unsigned char>(4, result);
	buff.insertString(5, name, 10);
	buff.insert<unsigned char>(15, slot);
	buff.insert<unsigned short>(16, level);
	buff.insert<unsigned char>(18, race << 1); // I don't know for what is left shifting ;/.

	// temporary :). Inventory here.
	for(size_t i = 0; i < 24; ++i) {
		buff.insert<unsigned char>(19 + i, 0x00);
	}
}

void protocol_t::parseCharacterDeleteRequest(gameServerUser_t &user,
											 const eMUCore::packet_t &packet) const {
	std::string name = packet.readString(4, 10);
	std::string pin = packet.readString(14, 7);

	m_executorInterface.onCharacterDeleteRequest(user, name, pin);
}

void protocol_t::constructCharacterDeleteAnswer(eMUCore::packet_t &buff,
												unsigned char result) const {
	buff.construct(0xC1, 0xF3);
	buff.insert<unsigned char>(3, 0x02);
	buff.insert<unsigned char>(4, result);
}

void protocol_t::parseCharacterSelectRequest(gameServerUser_t &user,
											 const eMUCore::packet_t &packet) const {
	std::string name = packet.readString(4, 10);
	m_executorInterface.onCharacterSelectRequest(user, name);
}

void protocol_t::constructCharacterSelectAnswer(eMUCore::packet_t &buff,
												const character_t &character) const {

	buff.construct(0xC3, 0xF3);
	buff.insert<unsigned char>(3, 0x03);
	buff.insert<unsigned char>(4, character.getPosX());
	buff.insert<unsigned char>(5, character.getPosY());
	buff.insert<unsigned char>(6, character.getMapId());
	buff.insert<unsigned char>(7, character.getDirection());
	buff.insert<unsigned int>(8, character.getExperience());
	buff.insert<unsigned int>(12, character.getNextExperience());
	buff.insert<unsigned short>(16, character.getLevelUpPoints());
	buff.insert<unsigned short>(18, character.getStrength());
	buff.insert<unsigned short>(20, character.getAgility());
	buff.insert<unsigned short>(22, character.getVitality());
	buff.insert<unsigned short>(24, character.getEnergy());
	buff.insert<unsigned short>(26, character.getHealth());
	buff.insert<unsigned short>(28, character.getMaxHealth());
	buff.insert<unsigned short>(30, character.getMana());
	buff.insert<unsigned short>(32, character.getMaxMana());
	buff.insert<unsigned short>(34, character.getShield());
	buff.insert<unsigned short>(36, character.getMaxShield());
	buff.insert<unsigned short>(38, character.getStamina());
	buff.insert<unsigned short>(40, character.getMaxStamina());
	buff.insert<unsigned int>(42, character.getMoney());
	buff.insert<unsigned char>(46, character.getPkLevel());
	buff.insert<unsigned char>(47, character.getControlCode());
	buff.insert<unsigned short>(48, character.getAddPoints());
	buff.insert<unsigned short>(50, character.getMaxAddPoints());
	buff.insert<unsigned short>(52, character.getCommand());
	buff.insert<unsigned short>(54, character.getMinusPoints());
	buff.insert<unsigned short>(56, character.getMaxMinusPoints());
}

void protocol_t::constructTextNotice(eMUCore::packet_t &buff,
										unsigned char type,
										const std::string &notice,
										unsigned char loopCount,
										unsigned short loopDelay,
										unsigned int color,
										unsigned char speed) const {
	buff.construct(0xC1, 0x0D);
	buff.insert<unsigned char>(3, type);
	buff.insert<unsigned char>(4, loopCount);
	buff.insert<unsigned short>(5, loopDelay);
	buff.insert<unsigned char>(7, 0); // unk.
	buff.insert<unsigned int>(8, color);
	buff.insert<unsigned char>(12, speed);

	size_t noticeLen = std::min<size_t>(notice.size(), 241);

	buff.insertString(13, notice, noticeLen); // 241 - max notice size.
	buff.insert<unsigned char>(13 + noticeLen, 0);	// cause webzen client want null terminated string
													// I don't know why if login, password or name haven't to be null terminated. 
}

void protocol_t::parseCharacterMoveRequest(gameServerUser_t &user,
										   const eMUCore::packet_t &packet) const {
	int stepsCount = packet.read<unsigned char>(5) & 0x0F;

	if(stepsCount <= 15) {
		if(stepsCount > 0) {
			++stepsCount;
		}

		unsigned char x = packet.read<unsigned char>(3);
		unsigned char y = packet.read<unsigned char>(4);
		unsigned char direction = packet.read<unsigned char>(5) >> 4;
		map_t::path_t path;

		char directions[] = {-1, -1, 0, -1, 1, -1, 1, 0, 1, 1, 0, 1, -1, 1, -1, 0};
		const unsigned char *data = &packet.getData()[5];

		for(int i = 1; i < stepsCount; ++i) {
			int directionId = 0;

			if((i & 1) == 1) {
				directionId = data[(i + 1) >> 1] >> 4;
			} else {
				directionId = data[(i + 1) >> 1] & 0x0F;
			}

			x += directions[directionId << 1];
			y += directions[(directionId << 1) + 1];

			path.push_back(map_t::position_t(x, y));
		}

		m_executorInterface.onCharacterMoveRequest(user, x, y, direction, path);
	}
}

void protocol_t::parseCharacterTeleportRequest(gameServerUser_t &user,
											   const eMUCore::packet_t &packet) const {
	unsigned short gateId = packet.read<unsigned short>(3);
	m_executorInterface.onCharacterTeleportRequest(user, gateId);
}

void protocol_t::constructCharacterTeleportAnswer(eMUCore::packet_t &buff,
													unsigned char mapId,
													unsigned char x,
													unsigned char y,
													unsigned char direction,
													unsigned char gateId) const {
	buff.construct(0xC3, 0x1C);
	buff.insert<unsigned char>(3, gateId);
	buff.insert<unsigned char>(4, mapId);
	buff.insert<unsigned char>(5, x);
	buff.insert<unsigned char>(6, y);
	buff.insert<unsigned char>(7, direction);
}

std::string protocol_t::xorString(const std::string &buff) const {
	unsigned char xorLoginKeys[3] = {0xFC, 0xCF, 0xAB};

	std::string encrypted;

	for(size_t i = 0; i < 10; ++i) {
		char value = buff[i] ^ xorLoginKeys[i % 3];

		if(value != 0) {
			encrypted.push_back(value);
		}
	}

	return encrypted;
}