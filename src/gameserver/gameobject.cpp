#include "gameobject.h"
#include <math.h>
#include <algorithm>

void gameObject_t::reset() {
	m_mapId = 0;
	m_posX = 0;
	m_posY = 0;
	m_direction = 0;
	m_viewRange = 0;
}

unsigned int gameObject_t::calculateDistance(const gameObject_t &object) const {
	if(m_posX == object.getPosX() && m_posY == object.getPosY()) {
		return 0;
	} else {
		unsigned int distanceX = abs(m_posX - object.getPosX());
		unsigned int distanceY = abs(m_posY - object.getPosY());

		return static_cast<unsigned int>(sqrt(static_cast<float>(distanceX * distanceX + distanceY * distanceY)));
	}
}