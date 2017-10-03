#include "protocol.h"

void protocol_t::core(dataServerUser_t &user,
						const eMUCore::packet_t &packet) const {
	switch(packet.getProtocolId()) {
	case 0xF1:
		switch(packet.getData()[3]) {
		case 0x01:
			this->parseAccountCheckRequest(user, packet);
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

		case 0x04:
			this->parseCharacterSaveRequest(user, packet);
			break;
		}
		break;
	}
}

void protocol_t::parseAccountCheckRequest(dataServerUser_t &user,
										const eMUCore::packet_t &packet) const {
	unsigned int connectionStamp = packet.read<unsigned int>(4);
	std::string accountId = packet.readString(8, 10);
	std::string password = packet.readString(18, 10);
	std::string ipAddress = packet.readString(28, 16);

	m_executorInterface.onAccountCheckRequest(user, connectionStamp, accountId, password, ipAddress);
}

void protocol_t::constructAccountCheckAnswer(eMUCore::packet_t &buff,
											unsigned int connectionStamp,
											const std::string &accountId,
											unsigned char result) const {
	buff.construct(0xC1, 0xF1);
	buff.insert<unsigned char>(3, 0x01);
	buff.insert<unsigned int>(4, connectionStamp);
	buff.insertString(8, accountId, 10);
	buff.insert<unsigned char>(18, result);
}

void protocol_t::parseCharacterListRequest(dataServerUser_t &user,
											const eMUCore::packet_t &packet) const {
	unsigned int connectionStamp = packet.read<unsigned int>(4);
	std::string accountId = packet.readString(8, 10);

	m_executorInterface.onCharacterListRequest(user, connectionStamp, accountId);
}

void protocol_t::constructCharacterListAnswer(eMUCore::packet_t &buff,
											   unsigned int connectionStamp,
											   const eMUShared::characterList_t &characterList) const {
	buff.construct(0xC1, 0xF3);
	buff.insert<unsigned char>(3, 0x00);
	buff.insert<unsigned int>(4, connectionStamp);
	buff.insert<unsigned char>(8, characterList.size());

	for(size_t i = 0; i < characterList.size(); ++i) {
		size_t step = i * 14;

		buff.insertString(9 + step, characterList[i].m_name, 10);
		buff.insert<unsigned char>(19 + step, characterList[i].m_race);
		buff.insert<unsigned short>(20 + step, characterList[i].m_level);
		buff.insert<unsigned char>(22 + step, characterList[i].m_controlCode);
	}
}

void protocol_t::parseLogoutRequest(dataServerUser_t &user,
									const eMUCore::packet_t &packet) const {
	std::string accountId = packet.readString(4, 10);
	m_executorInterface.onLogoutRequest(user, accountId);
}

void protocol_t::parseCharacterCreateRequest(dataServerUser_t &user,
												const eMUCore::packet_t &packet) const {
	unsigned int connectionStamp = packet.read<unsigned int>(4);
	std::string accountId = packet.readString(8, 10);
	std::string name = packet.readString(18, 10);
	unsigned char race = packet.read<unsigned char>(28);

	m_executorInterface.onCharacterCreateRequest(user,
													connectionStamp,
													accountId,
													name,
													race);
}

void protocol_t::constructCharacterCreateAnswer(eMUCore::packet_t &buff,
												unsigned int connectionStamp,
												const std::string &name,
												unsigned char race,
												unsigned char result) const {
	buff.construct(0xC1, 0xF3);
	buff.insert<unsigned char>(3, 0x01);
	buff.insert<unsigned int>(4, connectionStamp);
	buff.insertString(8, name, 10);
	buff.insert<unsigned char>(18, race);
	buff.insert<unsigned char>(19, result);
}



void protocol_t::parseCharacterDeleteRequest(dataServerUser_t &user,
											 const eMUCore::packet_t &packet) const {
	unsigned int connectionStamp = packet.read<unsigned int>(4);
	std::string accountId = packet.readString(8, 10);
	std::string name = packet.readString(18, 10);
	std::string pin = packet.readString(28, 7);

	m_executorInterface.onCharacterDeleteRequest(user, connectionStamp, accountId, name, pin);
}

void protocol_t::constructCharacterDeleteAnswer(eMUCore::packet_t &buff,
												unsigned int connectionStamp,
												const std::string &name,
												unsigned char result) const {
	buff.construct(0xC1, 0xF3);
	buff.insert<unsigned char>(3, 0x02);
	buff.insert<unsigned int>(4, connectionStamp);
	buff.insertString(8, name, 10);
	buff.insert<unsigned char>(18, result);
}

void protocol_t::parseCharacterSelectRequest(dataServerUser_t &user,
											const eMUCore::packet_t &packet) const {
	unsigned int connectionStamp = packet.read<unsigned int>(4);
	std::string accountId = packet.readString(8, 10);
	std::string name = packet.readString(18, 10);

	m_executorInterface.onCharacterSelectRequest(user, connectionStamp, accountId, name);
}

void protocol_t::constructCharacterSelectAnswer(eMUCore::packet_t &buff,
												unsigned int connectionStamp,
												const eMUShared::characterAttributes_t &attr) const {
	buff.construct(0xC1, 0xF3);
	buff.insert<unsigned char>(3, 0x03);
	buff.insert<unsigned int>(4, connectionStamp);
	buff.insertString(8, attr.m_name, 10);
	buff.insert<unsigned char>(18, attr.m_race);
	buff.insert<unsigned char>(19, attr.m_posX);
	buff.insert<unsigned char>(20, attr.m_posY);
	buff.insert<unsigned char>(21, attr.m_mapId);
	buff.insert<unsigned char>(22, attr.m_direction);
	buff.insert<unsigned int>(23, attr.m_experience);
	buff.insert<unsigned short>(27, attr.m_levelUpPoints);
	buff.insert<unsigned short>(29, attr.m_level);
	buff.insert<unsigned short>(31, attr.m_strength);
	buff.insert<unsigned short>(33, attr.m_agility);
	buff.insert<unsigned short>(35, attr.m_vitality);
	buff.insert<unsigned short>(37, attr.m_energy);
	buff.insert<unsigned short>(39, attr.m_health);
	buff.insert<unsigned short>(41, attr.m_maxHealth);
	buff.insert<unsigned short>(43, attr.m_mana);
	buff.insert<unsigned short>(45, attr.m_maxMana);
	buff.insert<unsigned short>(47, attr.m_shield);
	buff.insert<unsigned short>(49, attr.m_maxShield);
	buff.insert<unsigned short>(51, attr.m_stamina);
	buff.insert<unsigned short>(53, attr.m_maxStamina);
	buff.insert<unsigned int>(55, attr.m_money);
	buff.insert<unsigned char>(59, attr.m_pkLevel);
	buff.insert<unsigned char>(60, attr.m_controlCode);
	buff.insert<unsigned short>(61, attr.m_addPoints);
	buff.insert<unsigned short>(63, attr.m_maxAddPoints);
	buff.insert<unsigned short>(65, attr.m_command);
	buff.insert<unsigned short>(67, attr.m_minusPoints);
	buff.insert<unsigned short>(69, attr.m_maxMinusPoints);
}

void protocol_t::parseCharacterSaveRequest(dataServerUser_t &user,
										   const eMUCore::packet_t &packet) const {
	std::string accountId = packet.readString(4, 10);

	eMUShared::characterAttributes_t attr;
	attr.m_name = packet.readString(14, 10);
	attr.m_race = packet.read<unsigned char>(24);
	attr.m_posX = packet.read<unsigned char>(25);
	attr.m_posY = packet.read<unsigned char>(26);
	attr.m_mapId = packet.read<unsigned char>(27);
	attr.m_direction = packet.read<unsigned char>(28);
	attr.m_experience = packet.read<unsigned int>(29);
	attr.m_levelUpPoints = packet.read<unsigned short>(33);
	attr.m_level = packet.read<unsigned short>(35);
	attr.m_strength = packet.read<unsigned short>(37);
	attr.m_agility = packet.read<unsigned short>(39);
	attr.m_vitality = packet.read<unsigned short>(41);
	attr.m_energy = packet.read<unsigned short>(43);
	attr.m_health = packet.read<unsigned short>(45);
	attr.m_maxHealth = packet.read<unsigned short>(47);
	attr.m_mana = packet.read<unsigned short>(49);
	attr.m_maxMana = packet.read<unsigned short>(51);
	attr.m_shield = packet.read<unsigned short>(53);
	attr.m_maxShield = packet.read<unsigned short>(55);
	attr.m_stamina = packet.read<unsigned short>(57);
	attr.m_maxStamina = packet.read<unsigned short>(59);
	attr.m_money = packet.read<unsigned int>(61);
	attr.m_pkLevel = packet.read<unsigned char>(65);
	attr.m_controlCode = packet.read<unsigned char>(66);
	attr.m_addPoints = packet.read<unsigned short>(67);
	attr.m_maxAddPoints = packet.read<unsigned short>(69);
	attr.m_command = packet.read<unsigned short>(71);
	attr.m_minusPoints = packet.read<unsigned short>(73);
	attr.m_maxMinusPoints = packet.read<unsigned short>(75);

	m_executorInterface.onCharacterSaveRequest(user, accountId, attr);
}

void protocol_t::constructQueryExceptionNotice(eMUCore::packet_t &buff,
												unsigned int connectionStamp,
												const std::string &what) const {
	buff.construct(0xC2, 0x00);
	buff.insert<unsigned int>(4, connectionStamp);
	buff.insertString(8, what, 512);
}