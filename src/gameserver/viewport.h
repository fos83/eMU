#ifndef GAMESERVER_VIEWPORT_H
#define GAMESERVER_VIEWPORT_H

#include <list>
#include <set>

class gameObject_t;

class viewport_t {
public:
	struct viewedObject_t {
		viewedObject_t(gameObject_t *object):
		  m_new(true),
		  m_visible(true),
		  m_object(object) {}

		bool m_new;
		bool m_visible;
		gameObject_t *m_object;

		bool operator==(const viewedObject_t &vo) const;
	};

	typedef std::set<viewedObject_t> viewportList_t;

	void generate(std::list<gameObject_t*> &objectList);
	void clear();

private:
	void clean();
	void update();

	 viewportList_t m_viewedObjects;
};

#endif GAMESERVER_VIEWPORT_H