#ifndef eMU_GAMESERVER_GAMEOBJECT_H
#define eMU_GAMESERVER_GAMEOBJECT_H

#include "viewport.h"
#include <string>

class gameObject_t {
public:
	enum gameObjectType_t {
		_OBJECT_CHARACTER = 0xE1,
		_OBJECT_MONSTER,
	};

	typedef std::list<gameObject_t*> gameObjectList_t;

	gameObject_t(int index, gameObjectType_t type):
	  m_index(index),
	  m_objectType(type),
	  m_level(0),
	  m_health(0),
	  m_maxHealth(0),
	  m_mana(0),
	  m_maxMana(0),
	  m_mapId(0),
	  m_posX(0),
	  m_posY(0),
	  m_direction(0),
	  m_viewRange(0) {}

	virtual ~gameObject_t() {}

	virtual gameObjectType_t getObjectType() const { return m_objectType; }
	//virtual void setObjectType(gameObjectType_t objectType) { m_objectType = objectType; }

	virtual int getIndex() const { return m_index; }

	virtual const std::string& getName() const { return m_name; }

	virtual unsigned short getLevel() const { return m_level; }
	virtual void setLevel(unsigned short level) { m_level = level; }

	virtual unsigned int getHealth() const { return m_health; }
	virtual void setHealth(unsigned short health) { m_health = health; }

	virtual unsigned int getMaxHealth() const { return m_maxHealth; }
	virtual void setMaxHealth(unsigned short maxHealth) { m_maxHealth = maxHealth; }

	virtual unsigned int getMana() const { return m_mana; }
	virtual void setMana(unsigned short mana) { m_mana = mana; }

	virtual unsigned int getMaxMana() const { return m_maxMana; }
	virtual void setMaxMana(unsigned short maxMana) { m_maxMana = maxMana;}

	virtual unsigned char getMapId() const { return m_mapId; }
	virtual void setMapId(unsigned char mapId) {  m_mapId = mapId; }

	virtual unsigned char getPosX() const { return m_posX; }
	virtual void setPosX(unsigned char posX) {  m_posX = posX; }

	virtual unsigned char getPosY() const { return m_posY; }
	virtual void setPosY(unsigned char posY) {  m_posY = posY; }

	virtual unsigned char getDirection() const { return m_direction; }
	virtual void setDirection(unsigned char direction) {  m_direction = direction; }

	virtual unsigned short getViewRange() const { return m_viewRange; }
	virtual void setViewRange(unsigned char viewRange) {  m_viewRange = viewRange; }

	virtual viewport_t& getViewport() { return m_viewport; }

	virtual unsigned int calculateDistance(const gameObject_t &object) const;

	virtual void reset();

protected:
	gameObjectType_t	m_objectType;
	int					m_index;
	std::string			m_name;
	unsigned short		m_level;
	unsigned int		m_health;
	unsigned int		m_maxHealth;
	unsigned int		m_mana;
	unsigned int		m_maxMana;
	unsigned char		m_mapId;
	unsigned char		m_posX;
	unsigned char		m_posY;
	unsigned char		m_direction;
	unsigned short		m_viewRange;
	viewport_t			m_viewport;
};

#endif // eMU_GAMESERVER_GAMEOBJECT_H