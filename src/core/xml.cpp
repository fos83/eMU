#include "core.h"

using namespace eMUCore;

void xmlConfig_t::open(const std::string &fileName, const std::string &rootNodeName) throw(eMUCore::exception_t) {
	m_configFile = xmlReadFile(fileName.c_str(), NULL, XML_PARSE_NOBLANKS);

	if(m_configFile != NULL) {
		m_rootNode = xmlDocGetRootElement(m_configFile);

		if(m_rootNode != NULL) {
			if(xmlStrcmp(this->m_rootNode->name, reinterpret_cast<const xmlChar*>(rootNodeName.c_str())) == 0) {
				m_node = this->m_rootNode->children;
			} else {
				exception_t e;
				e.in() << __FILE__ << ":" << __LINE__ << "[xmlConfig_t::xmlConfig_t()] Could not find " << rootNodeName 
						<< " node in " << fileName << " file.";
				throw e;
			}
		} else {
			exception_t e;
			e.in() << __FILE__ << ":" << __LINE__ << "[xmlConfig_t::xmlConfig_t()] " << fileName << " is not valid xml file.";
			throw e;
		}
	} else {
		exception_t e;
		e.in() << __FILE__ << ":" << __LINE__ << "[xmlConfig_t::xmlConfig_t()] Could not open " << fileName << " file.";
		throw e;
	}
}

bool xmlConfig_t::nextNode() {
	if(m_isFirstIteration) {
		m_isFirstIteration = false;
	} else {
		m_node = m_node->next;
	}

	if(m_node != NULL) {
		return true;
	} else {
		m_node = m_rootNode->children; // Set to begin.
		return false;
	}
}