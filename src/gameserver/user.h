#ifndef eMU_GAMESERVER_USER_H
#define eMU_GAMESERVER_USER_H

#include <vector>
#include "core.h"
#include "shared.h"
#include "character.h"

class gameServerUser_t: public eMUCore::socketContext_t {
public:
	gameServerUser_t(int index):
	  socketContext_t(index),
	  m_character(index),
	  m_cryptSerial(0),
	  m_loginAttempts(0),
	  m_loggedIn(false),
	  m_availableRaces(0x02),
	  m_closeReason(0xFF),
	  m_timeToClose(0),
	  m_connectionStamp(0) {}

	friend std::ostream& operator<<(std::ostream &out, const gameServerUser_t &user) {
		out << reinterpret_cast<const eMUCore::socketContext_t&>(user);

		if(user.m_accountId != "") {
			out	<< "[" << user.m_accountId << "]";
		}

		return out;
	}

	void reset();

	inline unsigned char generateCryptSerial() { return m_cryptSerial++; }

	inline void incerementLoginAttempts() { ++m_loginAttempts; }
	inline int getLoginAttempts() const { return m_loginAttempts; }

	inline void setAccountId(const std::string &accountId) { m_accountId = accountId; }
	inline const std::string& getAccountId() const { return m_accountId; }

	inline void setLoggedIn() { m_loggedIn = true; }
	inline bool isLoggedIn() const { return m_loggedIn; }

	void setAvailableRaces(const eMUShared::characterList_t &characterList,
							unsigned short advancedRaceLevel);
	inline unsigned char getAvailableRaces() const { return m_availableRaces; }

	inline void setCloseReason(unsigned char closeReason) { m_closeReason = closeReason; }
	inline void resetCloseReason() { m_closeReason = 0xFF; }
	inline unsigned char getCloseReason() const { return m_closeReason; }

	inline void setTimeToClose(int timeToClose) { m_timeToClose = timeToClose; }
	inline void decrecemntTimeToClose() { --m_timeToClose; }
	inline int getTimeToClose() const { return m_timeToClose; }

	inline void setConnectionStamp(unsigned int connectionStamp) { m_connectionStamp = connectionStamp; }
	inline unsigned int getConnectionStamp() const { return m_connectionStamp; }

	inline character_t& getCharacter() { return m_character; }

	void initializeCharacterListMap();
	void mapCharacterList(const eMUShared::characterList_t &characterList);
	int insertToCharacterList(const std::string &name);
	void deleteFromCharacterList(const std::string &name);

	bool operator==(unsigned int connectionStamp) { return m_connectionStamp == connectionStamp; }
	bool operator==(const character_t &character) { return m_index == character.getIndex(); }

private:
	std::string							m_accountId;
	std::map<unsigned int, std::string>	m_characterListMap;
	unsigned char						m_cryptSerial;
	int									m_loginAttempts;
	bool								m_loggedIn;
	unsigned char						m_availableRaces;
	unsigned char						m_closeReason;
	int									m_timeToClose;
	unsigned int						m_connectionStamp;
	character_t							m_character;

	gameServerUser_t();
	gameServerUser_t(const gameServerUser_t&);
	gameServerUser_t& operator=(const gameServerUser_t&);
};

#endif // eMU_GAMESERVER_USER_H