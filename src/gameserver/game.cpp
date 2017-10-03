#include "game.h"
#include <boost\bind.hpp>
#include <iostream>

void game_t::startup() {
	m_versionConfiguration.read("version.xml");
	m_gameConfiguration.read("..\\data\\game.xml");

	m_mapManager.startup("..\\data\\worlds.xml");
	m_gateManager.startup("..\\data\\gates.xml");
	m_monsterAttributesManager.startup("..\\data\\monstersAttributes.xml");
	m_monsterManager.startup("..\\data\\monsters.xml");

	m_scheduler.insert(eMUCore::scheduler_t::_SCHEDULE_NONSYNCHRONIZED,
						boost::bind(&game_t::checkSelfClose, this),
						1);
}

void game_t::onHandshake(gameServerUser_t &user) const {
	eMUCore::packet_t packet;
	m_protocol.constructHandshake(packet,
									user.getIndex(),
									m_versionConfiguration.m_versionProtocol);
	m_networkInterface.send(user, packet);
}

void game_t::onLoginRequest(gameServerUser_t &user,
							const std::string &accountId,
							const std::string &password,
							const std::string &clientExeVersion,
							const std::string &clientExeSerial) {
	m_logger.in(eMUCore::logger_t::_MESSAGE_INFO) << user << " Login request :: account [" << accountId << "].";

	if(!user.isLoggedIn()) {
		if(clientExeVersion == m_versionConfiguration.m_versionProtocol
			&& clientExeSerial == m_versionConfiguration.m_serial) {
			m_logger.out();

			eMUCore::packet_t dsPacket;
			m_dataServerProtocol.constructAccountCheckRequest(dsPacket, 
																user.getConnectionStamp(), 
																accountId,
																password,
																user.getIpAddress());
			m_networkInterface.sendDataServer(dsPacket);
		} else {
			m_logger.append() << " Invalid version or serial [" << clientExeVersion << "][" << clientExeSerial << "].";
			m_logger.out();

			eMUCore::packet_t packet;
			m_protocol.constructLoginAnswer(packet, 0x06); // Bad version or serial.
			m_networkInterface.send(user, packet);
		}
	} else {
		m_logger.in(eMUCore::logger_t::_MESSAGE_ERROR) << user << " User already logged in.";
		m_logger.out();
		m_networkInterface.disconnect(user);
	}
}

void game_t::onAccountCheckAnswer(unsigned int connectionStamp,
							const std::string &accountId,
							unsigned char result) {
	gameServerUser_t &user = m_userManager.find(connectionStamp);

	m_logger.in(eMUCore::logger_t::_MESSAGE_INFO) << user << "[" << accountId << "]";

	if(result == 0x01) {
			m_logger.append() << " logged in.";
			m_logger.out();

			user.setAccountId(accountId);
			user.setLoggedIn();
	} else {
		switch(result) {
		case 0x00:
			m_logger.append() << " Invalid password.";
			break;

		case 0x02:
			m_logger.append() << " Not exists.";
			break;

		case 0x03:
			m_logger.append() << " Already in use.";
			break;

		case 0x05:
			m_logger.append() << " Banned.";
			break;
		}

		m_logger.out();

		user.incerementLoginAttempts();

		if(user.getLoginAttempts() >= 3) {
			result = 0x08; // to many attempts.
		}
	}

	eMUCore::packet_t packet;
	m_protocol.constructLoginAnswer(packet, result);
	m_networkInterface.send(user, packet);
}

void game_t::onCharacterListRequest(gameServerUser_t &user) {
	m_logger.in(eMUCore::logger_t::_MESSAGE_INFO) << user << " Character list request.";
	m_logger.out();

	if(user.isLoggedIn()) {
		eMUCore::packet_t dsPacket;
		m_dataServerProtocol.constructCharacterListRequest(dsPacket, user.getConnectionStamp(), user.getAccountId());
		m_networkInterface.sendDataServer(dsPacket);
	} else {
		m_logger.in(eMUCore::logger_t::_MESSAGE_ERROR) << user << " User is not logged on account.";
		m_logger.out();
		m_networkInterface.disconnect(user);
	}
}

void game_t::onCharacterListAnswer(unsigned int connectionStamp,
									const eMUShared::characterList_t &characterList) {
	gameServerUser_t &user = m_userManager.find(connectionStamp);

	user.mapCharacterList(characterList);
	user.setAvailableRaces(characterList, m_gameConfiguration.m_advancedRaceLevel);

	eMUCore::packet_t packet;
	m_protocol.constructCharacterListAnswer(packet, user.getAvailableRaces(), characterList);
	m_networkInterface.send(user, packet);
}

void game_t::onClientClose(gameServerUser_t &user) {
	if(user.getCharacter().isActive()) {
		this->onCharacterLeave(user);
	}

	if(user.isLoggedIn()) {
		eMUCore::packet_t packet;
		m_dataServerProtocol.constructLogoutRequest(packet, user.getAccountId());
		m_networkInterface.sendDataServer(packet);
	}

	user.reset();
}

void game_t::onCharacterLeave(gameServerUser_t &user) {
	m_mapManager[user.getCharacter().getMapId()].clearStand(user.getCharacter().getPosX(),
															user.getCharacter().getPosY());
	this->saveCharacter(user);
	user.getCharacter().setInactive();
	user.getCharacter().getViewport().clear();
	m_objectList.remove(&user.getCharacter());
}

void game_t::onLogoutRequest(gameServerUser_t &user, unsigned char closeReason) {
	user.setCloseReason(closeReason);
	user.setTimeToClose(5);
}

void game_t::onCharacterCreateRequest(gameServerUser_t &user,
										const std::string &name,
										unsigned char race) {
	m_logger.in(eMUCore::logger_t::_MESSAGE_INFO) << user << " Create character request :: name [" << name 
													<< "] race [" << static_cast<int>(race) << "].";
	m_logger.out();

	if(user.isLoggedIn()) {
		if(race == 0 ||
			race == 16 ||
			race == 32 ||
			race == 48 ||
			race == 64) {
			eMUCore::packet_t dsPacket;
			m_dataServerProtocol.constructCharacterCreateRequest(dsPacket,
																	user.getConnectionStamp(),
																	user.getAccountId(),
																	name,
																	race);
			m_networkInterface.sendDataServer(dsPacket);
		} else {
			m_logger.in(eMUCore::logger_t::_MESSAGE_ERROR) << user << " Invalid race.";
			m_logger.out();
			m_networkInterface.disconnect(user);
		}
	} else {
		m_logger.in(eMUCore::logger_t::_MESSAGE_ERROR) << user << " User is not logged on account.";
		m_logger.out();
		m_networkInterface.disconnect(user);
	}
}

void game_t::onCharacterCreateAnswer(unsigned int connectionStamp,
										const std::string &name,
										unsigned char race,
										unsigned char result) {
	gameServerUser_t &user = m_userManager.find(connectionStamp);

	m_logger.in(eMUCore::logger_t::_MESSAGE_INFO) << user;

	if(result != 0x02) {
		m_logger.append() << " Character [" << name << "]";

		eMUCore::packet_t packet;
		int slot = 0;

		if(result == 0x00) {
			m_logger.append() << " already exists.";
		} else if(result == 0x01) {
			slot = user.insertToCharacterList(name);
			m_logger.append() << " created at slot [" << slot << "].";
		}

		m_logger.out();
		m_protocol.constructCharacterCreteAnswer(packet, result, name, slot, 1, race);
		m_networkInterface.send(user, packet);
	} else {
		m_logger.append() << " account already have 5 characters.";
		m_logger.out();
		m_networkInterface.disconnect(user);
	}
}

void game_t::onCharacterDeleteRequest(gameServerUser_t &user,
										const std::string &name,
										const std::string &pin) {
	m_logger.in(eMUCore::logger_t::_MESSAGE_INFO) << user << " Delete character request ::" << " name [" << name << "].";
	m_logger.out();

	if(user.isLoggedIn()) {
		eMUCore::packet_t packet;
		m_dataServerProtocol.constructCharacterDeleteRequest(packet, user.getConnectionStamp(), user.getAccountId(), name, pin);
		m_networkInterface.sendDataServer(packet);
	} else {
		m_logger.in(eMUCore::logger_t::_MESSAGE_ERROR) << user << " User is not logged on account.";
		m_logger.out();
		m_networkInterface.disconnect(user);
	}
}

void game_t::onCharacterDeleteAnswer(unsigned int connectionStamp,
										const std::string &name,
										unsigned char result) {
	gameServerUser_t &user = m_userManager.find(connectionStamp);

	m_logger.in(eMUCore::logger_t::_MESSAGE_INFO) << user << " Character [" << name << "]";

	if(result != 0x00) {
		if(result == 0x01) { // ok.
			m_logger.append() << " deleted.";
			user.deleteFromCharacterList(name);
		} else if(result == 0x02) { // invalid pin.
			m_logger.append() << " invalid PIN.";
		}

		m_logger.out();
		eMUCore::packet_t packet;
		m_protocol.constructCharacterDeleteAnswer(packet, result);
		m_networkInterface.send(user, packet);
	} else {
		m_logger.append() << " not associated with account.";
		m_logger.out();
		m_networkInterface.disconnect(user);
	}
}

void game_t::onCharacterSelectRequest(gameServerUser_t &user,
									  const std::string &name) {
	m_logger.in(eMUCore::logger_t::_MESSAGE_INFO) << user << " Select character request ::" << " name [" << name << "].";
	m_logger.out();

	if(!user.getCharacter().isActive()) {
		eMUCore::packet_t packet;
		m_dataServerProtocol.constructCharacterSelectRequest(packet, user.getConnectionStamp(), user.getAccountId(), name);
		m_networkInterface.sendDataServer(packet);
	} else {
		m_logger.in(eMUCore::logger_t::_MESSAGE_ERROR) << user << " User is playing -> character [" << user.getCharacter().getName() << "].";
		m_logger.out();
		m_networkInterface.disconnect(user);
	}
}

void game_t::onCharacterSelectAnswer(unsigned int connectionStamp,
									 const eMUShared::characterAttributes_t &attr) {
	gameServerUser_t &user = m_userManager.find(connectionStamp);

	if(m_mapManager.isMapExists(attr.m_mapId)) {
		if(m_mapManager[attr.m_mapId].canStand(attr.m_posX, attr.m_posY)) {
			m_mapManager[attr.m_mapId].setStand(attr.m_posX, attr.m_posY);
			character_t &character = user.getCharacter();
			character.setAttributes(attr);
			character.setActive();
			character.getViewport().generate(m_objectList);
			m_objectList.push_back(&character);

			m_logger.in(eMUCore::logger_t::_MESSAGE_INFO) << user << " Preparing character [" << character.getName() << "].";
			m_logger.out();

			eMUCore::packet_t packet;
			m_protocol.constructCharacterSelectAnswer(packet, character);
			m_networkInterface.send(user, packet);

			packet.clear();
			m_protocol.constructTextNotice(packet, 0, m_gameConfiguration.m_welcomeNotice);
			m_networkInterface.send(user, packet);
		} else {
			m_logger.in(eMUCore::logger_t::_MESSAGE_ERROR) << user << " Invalid start coordinates [" << static_cast<int>(attr.m_posX) 
															<< "][" << static_cast<int>(attr.m_posY) << "].";
			m_logger.out();
			m_networkInterface.disconnect(user);
		}
	} else {
		m_logger.in(eMUCore::logger_t::_MESSAGE_ERROR) << user << " Invalid mapId [" << static_cast<int>(attr.m_mapId) << "].";
		m_logger.out();
		m_networkInterface.disconnect(user);
	}
}

void game_t::onCharacterMoveRequest(gameServerUser_t &user,
									unsigned char x,
									unsigned char y,
									unsigned char direction,
									const map_t::path_t &path) {
	character_t &character = user.getCharacter();

	if(GetTickCount() - character.getLastMoveTime() >= 100) {
		if(m_mapManager[character.getMapId()].isPathValid(path)) {
			m_mapManager[character.getMapId()].resetStand(character.getPosX(), character.getPosY(), x, y);

			character.setPosX(x);
			character.setPosY(y);
			character.setDirection(direction);
			character.setLastMoveTime(GetTickCount());
			character.getViewport().generate(m_objectList);
		} else {
			m_logger.in(eMUCore::logger_t::_MESSAGE_ERROR) << user << " [" << character.getName() << "] Invalid path for character.";
			m_logger.out();

			#ifdef _DEBUG
			m_logger.in(eMUCore::logger_t::_MESSAGE_DEBUG) << user << " [" << character.getName() << "] Path dump: " 
															<< m_mapManager[character.getMapId()].dumpPath(path) << ".";
			m_logger.out();
			#endif

			m_networkInterface.disconnect(user);
		}
	}
}

void game_t::onCharacterTeleportRequest(gameServerUser_t &user,
										unsigned short gateId) {
	character_t &character = user.getCharacter();

	m_logger.in(eMUCore::logger_t::_MESSAGE_INFO) << user << " [" << character.getName() << "] Requested teleport to gate [" << gateId << "].";
	m_logger.out();

	gate_t &gate = m_gateManager[gateId];

	if(gate.isInGate(character.getPosX(), character.getPosY())) {
		if(gate.getRequiredLevel() <= character.getLevel()) {
			gate_t &destGate = m_gateManager[gate.getDestId()];
			map_t::position_t position = destGate.getRandomPosition();

			#ifdef _DEBUG
			m_logger.in(eMUCore::logger_t::_MESSAGE_DEBUG) << "Source gate: " << gate << ", destination gate: " << destGate << ".";
			m_logger.out();
			#endif

			this->teleportCharacter(user,
									destGate.getMapId(),
									position.first,
									position.second,
									destGate.getDirection(),
									destGate.getId());
		} else {
			m_logger.in(eMUCore::logger_t::_MESSAGE_INFO) << user << " Character level not match to enter gate.";
			m_logger.out();
		}
	} else {
		m_logger.in(eMUCore::logger_t::_MESSAGE_ERROR) << user << " Character is not in gate range.";
		m_logger.out();
		m_networkInterface.disconnect(user);
	}
}

void game_t::onQueryExceptionNotice(unsigned int connectionStamp,
								const std::string &what) {
	gameServerUser_t &user = m_userManager.find(connectionStamp);

	m_logger.in(eMUCore::logger_t::_MESSAGE_ERROR) << "DataServer query error: " << user << " " << what;
	m_logger.out();
	m_networkInterface.disconnect(user);
}

void game_t::checkSelfClose() {
	for(size_t i = 0; i < m_userManager.getCount(); ++i) {
		gameServerUser_t &user = m_userManager[i];

		if(user.isLoggedIn()) {
			if(user.getCloseReason() != 0xFF) {
				if(user.getTimeToClose() == 0) {
					if(user.getCloseReason() == 0x01 && user.getCharacter().isActive()) { // 0x01 - switch character, 0x00 - exit game, 0x02 - select server.
						this->onCharacterLeave(user);
					}

					eMUCore::packet_t packet;
					m_protocol.constructLogoutAnswer(packet, user.getCloseReason());
					m_networkInterface.send(user, packet);

					user.resetCloseReason();
				} else {
					std::stringstream notice;
					notice << "You will left the game after " << user.getTimeToClose() << " seconds.";

					eMUCore::packet_t packet;
					m_protocol.constructTextNotice(packet, 1, notice.str()); // 1 - blue, 0 - gold
					m_networkInterface.send(user, packet);

					user.decrecemntTimeToClose();
				}
			}
		}
	}
}

void game_t::saveCharacter(gameServerUser_t &user) const {
	m_logger.in(eMUCore::logger_t::_MESSAGE_INFO) << user << " Saving character :: name [" << user.getCharacter().getName() << "].";
	m_logger.out();

	character_t &character = user.getCharacter();
	eMUShared::characterAttributes_t attr;

	attr.m_name = character.getName();
	attr.m_race = character.getRace();
	attr.m_posX = character.getPosX();
	attr.m_posY = character.getPosY();
	attr.m_mapId = character.getMapId();
	attr.m_direction = character.getDirection();
	attr.m_experience = character.getExperience();
	attr.m_levelUpPoints = character.getLevelUpPoints();
	attr.m_level = character.getLevel();
	attr.m_strength = character.getStrength();
	attr.m_agility = character.getAgility();
	attr.m_vitality = character.getVitality();
	attr.m_energy = character.getEnergy();
	attr.m_health = character.getHealth();
	attr.m_maxHealth = character.getMaxHealth();
	attr.m_mana = character.getMana();
	attr.m_maxMana = character.getMaxMana();
	attr.m_shield = character.getShield();
	attr.m_maxShield = character.getMaxShield();
	attr.m_stamina = character.getStamina();
	attr.m_maxStamina = character.getMaxStamina();
	attr.m_money = character.getMoney();
	attr.m_pkLevel = character.getPkLevel();
	attr.m_controlCode = character.getControlCode();
	attr.m_addPoints = character.getAddPoints();
	attr.m_maxAddPoints = character.getMaxAddPoints();
	attr.m_command = character.getCommand();
	attr.m_minusPoints = character.getMinusPoints();
	attr.m_maxMinusPoints = character.getMaxMinusPoints();

	eMUCore::packet_t packet;
	m_dataServerProtocol.constructCharacterSaveRequest(packet,
														user.getAccountId(),
														attr);
	m_networkInterface.sendDataServer(packet);
}

void game_t::teleportCharacter(gameServerUser_t &user,
								unsigned char mapId,
								unsigned char x,
								unsigned char y,
								unsigned char direction,
								unsigned char gateId) {
	character_t &character = user.getCharacter();

	m_logger.in(eMUCore::logger_t::_MESSAGE_INFO) << user << " Character ["	<< character.getName() << "] Teleporting to"
													<< " [" << static_cast<int>(mapId) << "]["
													<< static_cast<int>(x) << "][" << static_cast<int>(y) << "].";
	m_logger.out();

	m_mapManager[character.getMapId()].clearStand(character.getPosX(), character.getPosY());
	m_mapManager[mapId].setStand(x, y);

	character.setMapId(mapId);
	character.setPosX(x);
	character.setPosY(y);
	character.setDirection(direction);
	character.getViewport().clear();
	character.getViewport().generate(m_objectList);

	eMUCore::packet_t packet;
	m_protocol.constructCharacterTeleportAnswer(packet, mapId, x, y, direction, gateId);
	m_networkInterface.send(user, packet);
}