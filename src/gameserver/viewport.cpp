#include "viewport.h"
#include "gameobject.h"
#include "core.h"

bool viewport_t::viewedObject_t::operator ==(const viewport_t::viewedObject_t &vo) const {
	return (m_object->getIndex() == vo.m_object->getIndex());
}

void viewport_t::clean() { _PROFILE;

}

void viewport_t::update() { _PROFILE;
	this->clean();
}

void viewport_t::generate(std::list<gameObject_t*> &objectList) { _PROFILE;
	this->update();

	for(gameObject_t::gameObjectList_t::iterator i = objectList.begin(); i != objectList.end(); ++i) {

	}
}

void viewport_t::clear() { _PROFILE;

}