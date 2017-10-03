#include "core.h"

using namespace eMUCore;

void scheduler_t::insert(scheduleType_t type, const boost::function0<void> &callback, time_t delay) {
	schedule_t schedule = {callback, type, delay, 0};
	m_list.push_back(schedule);
}

void scheduler_t::worker() {
	for(std::vector<schedule_t>::iterator i = m_list.begin(); i != m_list.end(); ++i) {
		if((GetTickCount() - i->m_lastExecuteTime) >= (i->m_delay * 1000)) {
			if(i->m_type == _SCHEDULE_SYNCHRONIZED) {
				m_synchronizer.lock();
				i->m_callback();
				m_synchronizer.unlock();
			} else {
				i->m_callback();
			}

			i->m_lastExecuteTime = GetTickCount();
		}
	}
}