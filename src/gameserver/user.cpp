#include "user.h"

void gameServerUser_t::reset() {
	m_cryptSerial = 0;
	m_loginAttempts = 0;
	m_accountId = "";
	m_loggedIn = false;
	m_availableRaces = 0x02;
	m_closeReason = 0xFF;
	m_timeToClose = 0;
	m_connectionStamp = 0xDEADC0DE;
}

void gameServerUser_t::setAvailableRaces(const eMUShared::characterList_t &characterList,
										   unsigned short advancedRaceLevel) {
	for(size_t i = 0; i < characterList.size(); ++i) {
		unsigned char shiftedRace = characterList[i].m_race >> 4;

		/* Dostepny Magic Gladiator. */
		if(shiftedRace == 3 
			|| shiftedRace < 3 
				&& characterList[i].m_level >= advancedRaceLevel) {
			m_availableRaces = 0x03;
		}

		/* Dostepny Dark Lord. */
		if(shiftedRace == 4
			|| shiftedRace == 3 
				&& characterList[i].m_level >= advancedRaceLevel) {
			m_availableRaces = 0x04;
		}
	}
}

void gameServerUser_t::initializeCharacterListMap() {
	for(int i = 0; i < 5; ++i) {
		m_characterListMap[i] = "";
	}
}

void gameServerUser_t::mapCharacterList(const eMUShared::characterList_t &characterList) {
	this->initializeCharacterListMap();

	for(size_t i = 0; i < characterList.size(); ++i) {
		m_characterListMap[i] = characterList[i].m_name;
	}
}

int gameServerUser_t::insertToCharacterList(const std::string &name) {
	for(int i = 0; i < 5; ++i) {
		if(m_characterListMap[i] == "") {
			m_characterListMap[i] = name;
			return i;
		}
	}

	eMUCore::exception_t e;
	e.in() << "[" << m_accountId << "] No free slot for character [" << name << "].";
	throw e;
}

void gameServerUser_t::deleteFromCharacterList(const std::string &name) {
	for(int i = 0; i < 5; ++i) {
		if(m_characterListMap[i] == name) {
			m_characterListMap[i] = "";
			return;
		}
	}

	eMUCore::exception_t e;
	e.in() << "[" << m_accountId << "] No character [" << name << "] in list.";
	throw e;
}