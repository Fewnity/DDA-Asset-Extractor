// SPDX-License-Identifier: MIT
//
// Copyright (c) 2025-2025 Gregory Machefer (Fewnity)
//
// This file is part of DDA Extractor.

#pragma once

#include <string>
#include <memory>
#include <vector>

class DDAVector3
{
public:

	DDAVector3() : x(0), y(0), z(0) {}

	explicit DDAVector3(const float x, const float y, const float z) : x(x), y(y), z(z) {}

	float x;
	float y;
	float z;
};

class DDAColor
{
public:
	DDAColor() : r(0), g(0), b(0), a(0) {}
	explicit DDAColor(const float r, const float g, const float b, const float a) : r(r), g(g), b(b), a(a) {}

	float r;
	float g;
	float b;
	float a;
};

inline DDAVector3 operator-(const DDAVector3& left, const DDAVector3& right)
{
	return DDAVector3{ left.x - right.x, left.y - right.y, left.z - right.z };
}

inline DDAVector3 operator/(const DDAVector3& vec, const float value)
{
	return DDAVector3{ vec.x / value, vec.y / value, vec.z / value };
}

class DDAVector2
{
public:

	DDAVector2() : x(0), y(0) {}

	explicit DDAVector2(const float x, const float y) : x(x), y(y) {}

	float x;
	float y;
};

enum class DDAVertexElement : uint32_t // Do not change the uint32_t type
{
	NONE = 0,
	POSITION_32_BITS = 1 << 0,
	POSITION_16_BITS = 1 << 1, // Used for PSP
	POSITION_8_BITS = 1 << 2, // Used for PSP
	NORMAL_32_BITS = 1 << 3,
	NORMAL_16_BITS = 1 << 4, // Used for PSP
	NORMAL_8_BITS = 1 << 5, // Used for PSP
	UV_32_BITS = 1 << 6,
	UV_16_BITS = 1 << 7, // Used for PSP
	UV_8_BITS = 1 << 8, // Used for PSP
	COLOR_4_FLOATS = 1 << 9,
	COLOR_32_BITS_UINT = 1 << 10, // Used for PSP
};

constexpr DDAVertexElement operator|(DDAVertexElement lhs, DDAVertexElement rhs)
{
	using UnderlyingType = std::underlying_type_t<DDAVertexElement>;
	return static_cast<DDAVertexElement>(
		static_cast<UnderlyingType>(lhs) | static_cast<UnderlyingType>(rhs)
		);
}

constexpr DDAVertexElement& operator|=(DDAVertexElement& lhs, DDAVertexElement rhs)
{
	lhs = lhs | rhs;
	return lhs;
}

constexpr DDAVertexElement operator&(DDAVertexElement lhs, DDAVertexElement rhs)
{
	using UnderlyingType = std::underlying_type_t<DDAVertexElement>;
	return static_cast<DDAVertexElement>(
		static_cast<UnderlyingType>(lhs) & static_cast<UnderlyingType>(rhs)
		);
}

constexpr DDAVertexElement& operator&=(DDAVertexElement& lhs, DDAVertexElement rhs)
{
	lhs = lhs & rhs;
	return lhs;
}

struct DDAVertexDescriptor
{
public:
	DDAVertexElement elements = DDAVertexElement::NONE;
};

constexpr size_t DATA_BLOCK_HEADER_SIZE = 0x10;
constexpr size_t DATA_BLOCK_HEADER_NAME_SIZE = 4;

enum class DDAGameFile
{
	AIRPORT,
	BMOVIE,
	BRON_2ND,
	BRONX,
	CHIN_2ND,
	CHINATWN,
	CONSTR,
	DAM,
	ENGINE,
	GLADIATO,
	GODS,
	JUSTICE,
	REFINERY,
	SHIPYARD,
	STEELWRK,
	SUBWAY,
	VEGA_2ND,
	VEGAS,
	WINBOWL,

	AMSTAR,
	BLACK,
	CLOWN,
	DEVIL,
	FLAME,
	FLASH,
	FORMULA,
	GIRL,
	MIDNIGH,
	MONSTER,
	PEPSI,
	RIVER,
	SHARK,
	SKULL,
	SNAKE,
	STANG,
	STAR,
	TIGER,
	UNION,
	VOODOO,
	ZACE,
	ZGMC,
	ZHOTTIE,
	ZPOLICE,
	ZTAXI,

	FONT,
	INGAME,
	SPRITES,
	DD4FRONT,
	DD4GAME,
	DD4START
};

enum class DDAGameFileType : uint32_t
{
	MAP = 0x20000000, // File that contains the map data (Textures, meshes, etc)
	CAR = 0x20010000, // File that contains the car data (Car skin, meshes)
	FONT = 0x00030000, // File that contains the font data
	MENU = 0x00050003, // File that contains the menu data
	TEXTS = 0x00050004, // File that contains the texts
	IN_GAME = 0x04090000, // File that contains the sprites used in the game?
	SPRITES = 0x00050001, // File that contains the sprites used in the game?
	CAR_DATA = 0x000C0001, // File that contains multiplayer cars skins?
};

// Structure to store the link between a mesh packet and a texture
struct DDAPacketAndTextureEntry
{
	uint32_t vifPacketListAddr = 0;
	uint32_t textureIndex = 0; // Texture index that is used by the vif packet list
};

// Structure to store data about parent packets that contains mesh packets
struct DDAParentDrawCommandEntry
{
	uint32_t vifPacketTexturePairCount = 0;
	uint32_t addr = 0;
	uint32_t unkownAddress = 0; // ???
};

// Enum for different palette types
enum class DDAClutType : uint32_t
{
	CLUT_NONE = 0, // Not a used value in game but some textures do not have a clut
	CLUT_256 = 19, // 256 colors
	CLUT_16 = 20, // 16 colors
};

enum class DDAClutFixType : uint32_t
{
	CLUT_NORMAL = 0, // 256 colors
	CLUT_CAR_SKIN = 1, // 16 colors
	CLUT_BROKEN_CAR_SKIN = 2, // 16 colors
};

// structure to store the texture data
struct DDATextureTableEntry
{
	uint32_t mipmapCount = 0;          // Mipmap count, but weird value if it's an animated texture
	DDAClutType clutType = DDAClutType::CLUT_16;             // Palette type : 19 = 256 colors, 20 = 16 colors
	uint32_t width = 0;                // Texture width in pixel with the mipmap included
	uint32_t height = 0;               // Texture height in pixel with the mipmap included
	uint32_t unknown0 = 0;             // Unknown, not an unique value, id in the next table?
	uint32_t unknown1 = 0;             // Always 1
	uint32_t texturePosition = 0;      // Generally it's a relative position in the file
	uint32_t unknown2 = 0;             // Always 0
	uint32_t unknown3 = 0;             // Unknown, not const (seen values: 16, 32) 32 only seen in winbowl
	uint32_t unknown4 = 0;             // ??? = 16 * clutCount, not true if unknown3 is not egals to 16
	uint32_t unknown5 = 0;             // Always 0
	uint32_t clutCount = 0;            // Palettes count
	uint32_t palettePosition = 0;      // Generally it's a relative position in the file
	uint32_t textureInfosPosition = 0; // Generally it's a relative position in the file + 0xF to get the file name
	// There is the width, height (without the mipmap) and the texture name in the texture infos
};

struct DDATextureTableHeader
{
	uint32_t unkown0 = 0;
	uint32_t blockDataSize = 0; // Looks like it's the size of the whole data (table, meshes, textures...)
	uint32_t bytesCountBeforeEndFile = 0;
	char name[DATA_BLOCK_HEADER_NAME_SIZE];

	uint32_t offset = 0;
	uint32_t size = 0;
	uint32_t unkown6 = 0;
	uint32_t unkown7 = 0;
};

// structure to store where the mesh's data is in the file
struct DDAFileMeshDataInfo
{
	std::vector<size_t> endOfStripAt;
	size_t vertexCount = 0;
	size_t verticesPositionLocation = 0;
	size_t uvPositionLocation = 0;
	size_t meshPositionB = 0;
	size_t meshPositionAndSizeA = 0;
	size_t verticesColorsLocation = 0;
	size_t parentPacketIndex = 0;
};

struct DDATextureHeader
{
	uint32_t unkown0;
	uint32_t width; // Texture width in pixel without the mipmap
	uint32_t height; // Texture height in pixel without the mipmap
	uint32_t indexInDataChunk; // In menu files, this is the index of the texture in the next big data chunk
	char filePath[0x80]; // Texture file path, not always present
};

struct DDATextureTable
{
	// Copied from memory
	DDATextureTableHeader header;
	std::vector<DDATextureTableEntry> entries;
	std::vector<DDATextureHeader> textureHeaders;
	std::vector<std::string> textureNames;
	// Calculated
	size_t textureCount = 0;
};

// structure to store the parameters to convert and copy the texture data
struct DDATextureCopyParams
{
	// Size with mipmap included (divided by 2 if using CLUT_16)
	size_t inputWidth = 0;
	size_t inputHeight = 0;
	// Size without the mipmap (divided by 2 if using CLUT_16)
	size_t outputWidth = 0;
	size_t outputHeight = 0;
	// Size of the final png file
	size_t exportWidth = 0;
	size_t exportHeight = 0;
	size_t xOffset = 0;
	size_t yOffset = 0;
	DDAClutType clutType = DDAClutType::CLUT_256;
	uint8_t* inputTextureData = nullptr;
	std::unique_ptr<uint8_t[]> outputTextureData;
	std::unique_ptr<uint8_t[]> palette;
	std::string textureName;
};

struct DDASubMesh
{
public:
	std::vector<DDAVector3> verticesPositions;
	std::vector<DDAVector2> verticesUVs;
	std::vector<DDAVector3> verticesNormals;
	std::vector<DDAColor> verticesColors;
	uint32_t materialIndex = 0; // Index of the material used by this mesh

	void SetVertex(float u, float v, const DDAColor& color, float x, float y, float z, uint32_t vertexIndex)
	{
		verticesPositions[vertexIndex] = DDAVector3(x, y, z);
		verticesUVs[vertexIndex] = DDAVector2(u, v);
		verticesColors[vertexIndex] = color;
	}

	void SetVertex(float u, float v, float nx, float ny, float nz, float x, float y, float z, uint32_t vertexIndex)
	{
		verticesPositions[vertexIndex] = DDAVector3(x, y, z);
		verticesUVs[vertexIndex] = DDAVector2(u, v);
		verticesNormals[vertexIndex] = DDAVector3(nx, ny, nz);
	}
};

struct DDAMesh
{
public:
	std::vector<DDASubMesh> subMeshes;
	DDAVertexDescriptor vertexDescriptor;
};

struct DDAExtractedData
{
	std::vector<DDATextureTable> textureTables;
	std::vector<std::vector<DDATextureHeader>> textureHeaders; // Used for menu textures because there is no texture table
	std::vector<DDAPacketAndTextureEntry> packetAndTextureEntryList;
	std::vector<DDAMesh> meshes;

	std::vector< DDATextureCopyParams> textureCopyParamsList;
	size_t fileSize = 0;
};

const std::string filesNames[50] =
{
	// Maps
	"TRACKS\\AIRPORT.UBR", // 0
	"TRACKS\\BMOVIE.UBR", // 1
	"TRACKS\\BRON_2ND.UBR", // 2
	"TRACKS\\BRONX.UBR", // 3
	"TRACKS\\CHIN_2ND.UBR", // 4
	"TRACKS\\CHINATWN.UBR", // 5
	"TRACKS\\CONSTR.UBR", // 6
	"TRACKS\\DAM.UBR", // 7
	"TRACKS\\ENGINE.UBR", // 8
	"TRACKS\\GLADIATO.UBR", // 9
	"TRACKS\\GODS.UBR", // 10
	"TRACKS\\JUSTICE.UBR", // 11
	"TRACKS\\REFINERY.UBR", // 12
	"TRACKS\\SHIPYARD.UBR", // 13
	"TRACKS\\STEELWRK.UBR", // 14
	"TRACKS\\SUBWAY.UBR", // 15
	"TRACKS\\VEGA_2ND.UBR", // 16
	"TRACKS\\VEGAS.UBR", // 17
	"TRACKS\\WINBOWL.UBR", // 18

	// Cars
	"SOLOCARS\\AMSTAR.UBR", // 19
	"SOLOCARS\\BLACK.UBR", // 20
	"SOLOCARS\\CLOWN.UBR", // 21
	"SOLOCARS\\DEVIL.UBR", // 22
	"SOLOCARS\\FLAME.UBR", // 23
	"SOLOCARS\\FLASH.UBR", // 24
	"SOLOCARS\\FORMULA.UBR", // 25
	"SOLOCARS\\GIRL.UBR", // 26
	"SOLOCARS\\MIDNIGH.UBR", // 27
	"SOLOCARS\\MONSTER.UBR", // 28
	"SOLOCARS\\PEPSI.UBR", // 29
	"SOLOCARS\\RIVER.UBR", // 30
	"SOLOCARS\\SHARK.UBR", // 31
	"SOLOCARS\\SKULL.UBR", // 32
	"SOLOCARS\\SNAKE.UBR", // 33
	"SOLOCARS\\STANG.UBR", // 34
	"SOLOCARS\\STAR.UBR", // 35
	"SOLOCARS\\TIGER.UBR", // 36
	"SOLOCARS\\UNION.UBR", // 37
	"SOLOCARS\\VOODOO.UBR", // 38
	"SOLOCARS\\ZACE.UBR", // 39
	"SOLOCARS\\ZGMC.UBR", // 40
	"SOLOCARS\\ZHOTTIE.UBR", // 41
	"SOLOCARS\\ZPOLICE.UBR", // 42
	"SOLOCARS\\ZTAXI.UBR", // 43

	// Other
	"FONT.UBR", // 44
	"INGAME.UBR", // 45
	"SPRITES.UBR", // 46

	"FLASH\\DD4FRONT\\DD4FRONT.UBR", // 47
	"FLASH\\DD4GAME\\DD4GAME.UBR", // 48
	"FLASH\\DD4START\\DD4START.UBR", // 49
};

inline uint32_t GetHeaderOffset(DDAGameFileType fileType)
{
	if (fileType == DDAGameFileType::SPRITES)
	{
		return 0x10;
	}
	if (fileType == DDAGameFileType::IN_GAME)
	{
		return 0x10;
	}
	else // Car and Maps
	{
		return 0x80;
	}
}