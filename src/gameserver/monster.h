#ifndef eMU_GAMESERVER_MONSTER_H
#define eMU_GAMESERVER_MONSTER_H

#include "gameobject.h"
#include <vector>
#include <map>
#include <string>
#include <boost/function.hpp>

class monsterAttributesManager_t {
public:
	struct monsterAttributes_t {
		std::string		m_name;
		unsigned short	m_level;
		unsigned int	m_maxHealth;
		unsigned int	m_maxMana;
		unsigned int	m_magicDefense;
		unsigned int	m_attackSuccessRate;
		unsigned int	m_defenseSuccessRate;
		unsigned int	m_attackSpeed;
		unsigned int	m_attackType;
		unsigned short	m_attackRange;
		unsigned short  m_viewRange;
		unsigned short	m_moveRange;
		unsigned int	m_moveSpeed;
		unsigned int	m_respawnTime;
		unsigned int	m_itemDropRate;
		unsigned int	m_maxItemDropLevel;
		unsigned int	m_moneyDropRate;	
		unsigned int	m_windProtect;
		unsigned int	m_poisonProtect;
		unsigned int	m_iceProtect;
		unsigned int	m_electricProtect;
		unsigned int	m_fireProtect;
		unsigned int	m_minDamage;
		unsigned int	m_maxDamage;
		unsigned int	m_defense;
	};

	void startup(const std::string &fileName);
	const monsterAttributes_t& operator[](int monsterId);

private:
	std::map<int, monsterAttributes_t> m_attributesList;
};

class monster_t: public gameObject_t {
public:
	monster_t(int index):
	  gameObject_t(index, gameObject_t::_OBJECT_MONSTER),
	  m_id(0),
	  m_magicDefense(0),
	  m_attackSuccessRate(0),
	  m_defenseSuccessRate(0),
	  m_attackSpeed(0),
	  m_attackType(0),
	  m_attackRange(0),
	  m_viewRange(0),
	  m_moveRange(0),
	  m_moveSpeed(0),
	  m_respawnTime(0),
	  m_itemDropRate(0),
	  m_maxItemDropLevel(0),
	  m_moneyDropRate(0),
	  m_windProtect(0),
	  m_poisonProtect(0),
	  m_iceProtect(0),
	  m_electricProtect(0),
	  m_fireProtect(0),
	  m_minDamage(0),
	  m_maxDamage(0),
	  m_defense(0),
	  m_isNpc(false) {}

	void setAttributes(const monsterAttributesManager_t::monsterAttributes_t &attr);

	inline void setId(int id) { m_id = id; }
	inline int getId() const { return m_id; }

private:
	int				m_id;
	unsigned int	m_magicDefense;
	unsigned int	m_attackSuccessRate;
	unsigned int	m_defenseSuccessRate;
	unsigned int	m_attackSpeed;
	unsigned int	m_attackType;
	unsigned short	m_attackRange;
	unsigned short  m_viewRange;
	unsigned short	m_moveRange;
	unsigned int	m_moveSpeed;
	unsigned int	m_respawnTime;
	unsigned int	m_itemDropRate;
	unsigned int	m_maxItemDropLevel;
	unsigned int	m_moneyDropRate;	
	unsigned int	m_windProtect;
	unsigned int	m_poisonProtect;
	unsigned int	m_iceProtect;
	unsigned int	m_electricProtect;
	unsigned int	m_fireProtect;
	unsigned int	m_minDamage;
	unsigned int	m_maxDamage;
	unsigned int	m_defense;
	bool			m_isNpc;
};

class monsterManager_t {
public:
	monsterManager_t(monsterAttributesManager_t	&monsterAttributesManager,
						gameObject_t::gameObjectList_t &objectList,
						int startIndex):
	  m_monsterAttributesManager(monsterAttributesManager),
	  m_monsterList(objectList),
	  m_startIndex(startIndex) {}

	~monsterManager_t() {
		for(gameObject_t::gameObjectList_t::iterator i = m_monsterList.begin(); i != m_monsterList.end(); ++i) {
			gameObject_t *object = (*i);

			if(object->getObjectType() == gameObject_t::_OBJECT_MONSTER) {
				delete object;
			}
		}
	}

	void startup(const std::string &monstersFileName);
	size_t getCount() const { return m_monsterList.size(); }

private:
	monsterManager_t();
	monsterManager_t(const monsterManager_t&);
	monsterManager_t& operator=(const monsterManager_t&);

	monsterAttributesManager_t			&m_monsterAttributesManager;
	gameObject_t::gameObjectList_t		&m_monsterList;
	int									m_startIndex;
};

#endif // eMU_GAMESERVER_MONSTER_H