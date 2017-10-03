#include "map.h"
#include "..\core\core.h"
#include <fstream>

void map_t::startup(const std::string &filename, int mapId) {
	std::ifstream mapFile(filename.c_str(), std::ios::in | std::ios::binary);

	if(!mapFile.fail()) {
		mapFileHeader_t header;
		mapFile.read(reinterpret_cast<char*>(&header), sizeof(header));

		if(header.m_mapWidth == (c_mapWidth - 1) && header.m_mapHeight == (c_mapHeight - 1)) {
			mapFile.read(reinterpret_cast<char*>(m_mapTiles), c_mapSize);
		} else {
			eMUCore::exception_t e;
			e.in() << __FILE__ << ":" << __LINE__ << "[map_t::startup()] File [" << filename << "] is corrupted.";
			throw e;
		}
	} else {
		eMUCore::exception_t e;
		e.in() << __FILE__ << ":" << __LINE__ << "[map_t::startup()] Could not open [" << filename << "] file.";
		throw e;
	}
}

bool map_t::isPathValid(const path_t &path) const {
	for(size_t i = 0; i < path.size(); ++i) {
		if(!this->canStand(path[i].first, path[i].second)) {
			return false;
		}
	}

	return true;
}

std::string map_t::dumpPath(const path_t &path) const {
	std::stringstream stream;
	for(size_t i = 0; i < path.size(); ++i) {
		stream << "[" << static_cast<int>(path[i].first) << "/" << static_cast<int>(path[i].second)
				<< "][" << static_cast<int>(this->getTileAttribute(path[i].first, path[i].second)) << "] ";
	}

	return stream.str();
}

void mapManager_t::startup(const std::string &filename) {
	eMUCore::xmlConfig_t mapFile;
	mapFile.open(filename, "worlds");

	while(mapFile.nextNode()) {
		int id = mapFile.readFromProperty<int>("world", "id", -1);
		std::string mapFilename = mapFile.readFromProperty<std::string>("world", "stream", "");

		map_t *map = new map_t;
		map->startup(mapFilename, id);

		m_mapList[id] = map;
	}
}

map_t& mapManager_t::operator[](int mapId) {
	if(this->isMapExists(mapId)) {
		return *m_mapList[mapId];
	} else {
		eMUCore::exception_t e;
		e.in() << __FILE__ << ":" << __LINE__ << "[mapManager_t::operator[]()] No map [" << mapId << "] found on map list.";
		throw e;
	}
}

bool mapManager_t::isMapExists(int mapId) {
	mapList_t::iterator iter = m_mapList.find(mapId);

	if(iter != m_mapList.end()) {
		return true;
	} else {
		return false;
	}
}