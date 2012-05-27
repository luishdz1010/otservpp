/*
 * WorldMap.h
 *
 *  Created on: 10/05/2012
 *      Author: mckazi
 */

#include <stdint.h>

#ifndef OTSERVPP_WORLDMAP_H_
#define OTSERVPP_WORLDMAP_H_

namespace otservpp {

/*!
 * The attributes of a tile.
 */
enum class TileAttribute : uint32_t{
	ProtectionZone 			= 1 << 0,
	NonPvpZone 				= 1 << 1,
	NoLogoutZone			= 1 << 2,
	PvpZone					= 1 << 3
};

//using TileAttributes = EnumFlags<TileAttribute>;

class Tile{
public:
	Tile();

	bool hasAttribute(TileAttribute attr);
	void setAttribute(TileAttribute attr);
	void removeAttribute(TileAttribute attr);

private:
	uint32_t attributes;
};

class MapSector{

};

class WorldMap {
public:
	WorldMap();


	//MapSector& operator[]();

private:

};

} /* namespace otservpp */
#endif // OTSERVPP_WORLDMAP_H_
