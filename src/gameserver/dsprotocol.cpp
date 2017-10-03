#include "dsprotocol.h"

#include <iostream>

void dataServerProtocol_t::core(const eMUCore::packet_t &packet) const {
	switch(packet.getProtocolId()) {
	case 0x00:
		this->parseQueryExceptionNotice(packet);
		break;

	case 0xF1:
		switch(packet.getData()[3]) {
		case 0x01:
			this->parseAccountCheckAnswer(packet);
			break;
		}
		break;

	case 0xF3:
		switch(packet.getData()[3]) {
		case 0x00:
			this->parseCharacterListAnswer(packet);
			break;

		case 0x01:
			this->parseCharacterCreateAnswer(packet);
			break;

		case 0x02:
			this->parseCharacterDeleteAnswer(packet);
			break;

		case 0x03:
			this->parseCharacterSelectAnswer(packet);
			break;
		}
		break;
	}
}

void dataServerProtocol_t::constructAccountCheckRequest(eMUCore::packet_t &buff,
														unsigned int connectionStamp,
														const std::string &accountId,
														const std::string &password,
														const std::string &ipAddress) const {
	buff.construct(0xC1, 0xF1);
	buff.insert<unsigned char>(3, 0x01); // SubProtocolId
	buff.insert<unsigned int>(4, connectionStamp);
	buff.insertString(8, accountId, 10);
	buff.insertString(18, password, 10);
	buff.insertString(28, ipAddress, 16);
}

void dataServerProtocol_t::parseAccountCheckAnswer(const eMUCore::packet_t &packet) const {
	unsigned int connectionStamp = packet.read<unsigned int>(4);
	std::string accountId = packet.readString(8, 10);
	unsigned char result = packet.read<unsigned char>(18);

	m_executorInterface.onAccountCheckAnswer(connectionStamp, accountId, result);
}

void dataServerProtocol_t::constructCharacterListRequest(eMUCore::packet_t &buff,
															unsigned int connectionStamp,
															const std::string &accountId) const {
	buff.construct(0xC1, 0xF3);
	buff.insert<unsigned char>(3, 0x00); // SubProtocolId
	buff.insert<unsigned int>(4, connectionStamp);
	buff.insertString(8, accountId, 10);
}

void dataServerProtocol_t::parseCharacterListAnswer(const eMUCore::packet_t &packet) const {
	unsigned int connectionStamp = packet.read<unsigned int>(4);
	size_t charactersCount = packet.read<unsigned char>(8);

	eMUShared::characterList_t characterList;

	for(size_t i = 0; i < charactersCount; ++i) {
		size_t step = i * 14;

		eMUShared::characterListAttributes_t character;
		character.m_name = packet.readString(9 + step, 10);
		character.m_race = packet.read<unsigned char>(19 + step);
		character.m_level = packet.read<unsigned short>(20 + step);
		character.m_controlCode = packet.read<unsigned char>(22 + step);

		characterList.push_back(character);
	}

	m_executorInterface.onCharacterListAnswer(connectionStamp, characterList);
}

void dataServerProtocol_t::constructLogoutRequest(eMUCore::packet_t &buff,
												  const std::string &accountId) const {
	buff.construct(0xC1, 0xF1);
	buff.insert<unsigned char>(3, 0x02); // SubProtocolId.
	buff.insertString(4, accountId, 10);
}

void dataServerProtocol_t::constructCharacterCreateRequest(eMUCore::packet_t &buff,
															unsigned int connectionStamp,
															const std::string &accountId,
															const std::string &name,
															unsigned char race) const {
	buff.construct(0xC1, 0xF3);
	buff.insert<unsigned char>(3, 0x01);
	buff.insert<unsigned int>(4, connectionStamp);
	buff.insertString(8, accountId, 10);
	buff.insertString(18, name, 10);
	buff.insert<unsigned char>(28, race);
}

void dataServerProtocol_t::parseCharacterCreateAnswer(const eMUCore::packet_t &packet) const {
	unsigned int connectionStamp = packet.read<unsigned int>(4);
	std::string name = packet.readString(8, 10);
	unsigned char race = packet.read<unsigned char>(18);
	unsigned char result = packet.read<unsigned char>(19);

	m_executorInterface.onCharacterCreateAnswer(connectionStamp,
													name,
													race,
													result);
}

void dataServerProtocol_t::constructCharacterDeleteRequest(eMUCore::packet_t &buff,
															unsigned int connectionStamp,
															const std::string &accountId,
															const std::string &name,
															const std::string &pin) const {
	buff.construct(0xC1, 0xF3);
	buff.insert<unsigned char>(3, 0x02);
	buff.insert<unsigned int>(4, connectionStamp);
	buff.insertString(8, accountId, 10);
	buff.insertString(18, name, 10);
	buff.insertString(28, pin, 7);
}

void dataServerProtocol_t::parseCharacterDeleteAnswer(const eMUCore::packet_t &packet) const {
	unsigned int connectionStamp = packet.read<unsigned int>(4);
	std::string name = packet.readString(8, 10);
	unsigned char result = packet.read<unsigned char>(18);

	m_executorInterface.onCharacterDeleteAnswer(connectionStamp, name, result);
}

void dataServerProtocol_t::constructCharacterSelectRequest(eMUCore::packet_t &buff,
															unsigned int connectionStamp,
															const std::string &accountId,
															const std::string &name) const {
	buff.construct(0xC1, 0xF3);
	buff.insert<unsigned char>(3, 0x03);
	buff.insert<unsigned int>(4, connectionStamp);
	buff.insertString(8, accountId, 10);
	buff.insertString(18, name, 10);
}

void dataServerProtocol_t::parseCharacterSelectAnswer(const eMUCore::packet_t &packet) const {
	unsigned int connectionStamp = packet.read<unsigned int>(4);

	eMUShared::characterAttributes_t attr;
	attr.m_name = packet.readString(8, 10);
	attr.m_race = packet.read<unsigned char>(18);
	attr.m_posX = packet.read<unsigned char>(19);
	attr.m_posY = packet.read<unsigned char>(20);
	attr.m_mapId = packet.read<unsigned char>(21);
	attr.m_direction = packet.read<unsigned char>(22);
	attr.m_experience = packet.read<unsigned int>(23);
	attr.m_levelUpPoints = packet.read<unsigned short>(27);
	attr.m_level = packet.read<unsigned short>(29);
	attr.m_strength = packet.read<unsigned short>(31);
	attr.m_agility = packet.read<unsigned short>(33);
	attr.m_vitality = packet.read<unsigned short>(35);
	attr.m_energy = packet.read<unsigned short>(37);
	attr.m_health = packet.read<unsigned short>(39);
	attr.m_maxHealth = packet.read<unsigned short>(41);
	attr.m_mana = packet.read<unsigned short>(43);
	attr.m_maxMana = packet.read<unsigned short>(45);
	attr.m_shield = packet.read<unsigned short>(47);
	attr.m_maxShield = packet.read<unsigned short>(49);
	attr.m_stamina = packet.read<unsigned short>(51);
	attr.m_maxStamina = packet.read<unsigned short>(53);
	attr.m_money = packet.read<unsigned int>(55);
	attr.m_pkLevel = packet.read<unsigned char>(59);
	attr.m_controlCode = packet.read<unsigned char>(60);
	attr.m_addPoints = packet.read<unsigned short>(61);
	attr.m_maxAddPoints = packet.read<unsigned short>(63);
	attr.m_command = packet.read<unsigned short>(65);
	attr.m_minusPoints = packet.read<unsigned short>(67);
	attr.m_maxMinusPoints = packet.read<unsigned short>(69);

	m_executorInterface.onCharacterSelectAnswer(connectionStamp, attr);
}

void dataServerProtocol_t::constructCharacterSaveRequest(eMUCore::packet_t &buff,
															const std::string &accountId,
															const eMUShared::characterAttributes_t &attr) const {
	buff.construct(0xC1, 0xF3);
	buff.insert<unsigned char>(3, 0x04);
	buff.insertString(4, accountId, 10);
	buff.insertString(14, attr.m_name, 10);
	buff.insert<unsigned char>(24, attr.m_race);
	buff.insert<unsigned char>(25, attr.m_posX);
	buff.insert<unsigned char>(26, attr.m_posY);
	buff.insert<unsigned char>(27, attr.m_mapId);
	buff.insert<unsigned char>(28, attr.m_direction);
	buff.insert<unsigned int>(29, attr.m_experience);
	buff.insert<unsigned short>(33, attr.m_levelUpPoints);
	buff.insert<unsigned short>(35, attr.m_level);
	buff.insert<unsigned short>(37, attr.m_strength);
	buff.insert<unsigned short>(39, attr.m_agility);
	buff.insert<unsigned short>(41, attr.m_vitality);
	buff.insert<unsigned short>(43, attr.m_energy);
	buff.insert<unsigned short>(45, attr.m_health);
	buff.insert<unsigned short>(47, attr.m_maxHealth);
	buff.insert<unsigned short>(49, attr.m_mana);
	buff.insert<unsigned short>(51, attr.m_maxMana);
	buff.insert<unsigned short>(53, attr.m_shield);
	buff.insert<unsigned short>(55, attr.m_maxShield);
	buff.insert<unsigned short>(57, attr.m_stamina);
	buff.insert<unsigned short>(59, attr.m_maxStamina);
	buff.insert<unsigned int>(61, attr.m_money);
	buff.insert<unsigned char>(65, attr.m_pkLevel);
	buff.insert<unsigned char>(66, attr.m_controlCode);
	buff.insert<unsigned short>(67, attr.m_addPoints);
	buff.insert<unsigned short>(69, attr.m_maxAddPoints);
	buff.insert<unsigned short>(71, attr.m_command);
	buff.insert<unsigned short>(73, attr.m_minusPoints);
	buff.insert<unsigned short>(75, attr.m_maxMinusPoints);
}

void dataServerProtocol_t::parseQueryExceptionNotice(const eMUCore::packet_t &packet) const {
	unsigned int connectionStamp = packet.read<unsigned int>(4);
	std::string what = packet.readString(8, 512);

	m_executorInterface.onQueryExceptionNotice(connectionStamp, what);
}