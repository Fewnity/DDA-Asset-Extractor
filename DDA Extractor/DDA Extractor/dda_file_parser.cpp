// SPDX-License-Identifier: MIT
//
// Copyright (c) 2025-2025 Gregory Machefer (Fewnity)
//
// This file is part of DDA Extractor.

#include "dda_file_parser.h"

#include <fstream>
#include <filesystem>
#include <iostream>

#include "mesh_generator.h"

/**
* @brief Get the address of the skybox texture table header
* @return Absolute address of the skybox texture table header
*/
uint32_t DDAFileParser::GetSkyboxTextureTableHeader(uint32_t baseHeaderAddress)
{
	// The skybox header has the same name as the base header
	// The "Dams" is the skybox header name of the Dam track, for some reason it's not the same name as the base header
	const DDATextureTableHeader baseHeader = *(DDATextureTableHeader*)(m_fileData.get() + baseHeaderAddress);
	uint32_t currentHeaderAddressOffset = baseHeader.blockDataSize + DATA_BLOCK_HEADER_SIZE;
	
	// Find the skybox header by comparing the name of all other headers
	DDATextureTableHeader currentHeader = *(DDATextureTableHeader*)(m_fileData.get() + baseHeaderAddress + currentHeaderAddressOffset);
	while (strncmp(baseHeader.name, currentHeader.name, DATA_BLOCK_HEADER_NAME_SIZE) != 0 && strncmp(currentHeader.name, "Dams", DATA_BLOCK_HEADER_NAME_SIZE) != 0)
	{
		currentHeaderAddressOffset += currentHeader.blockDataSize + DATA_BLOCK_HEADER_SIZE;
		currentHeader = *(DDATextureTableHeader*)(m_fileData.get() + baseHeaderAddress + currentHeaderAddressOffset);
	}

	return baseHeaderAddress + currentHeaderAddressOffset;
}

/**
* @brief Get all data blocks headers of the IN_GAME.UBR file, where mesh and texture tables are stored
* @return A vector of addresses of the data blocks headers relative to the first data block header (after the whole texture data)
*/
std::vector<uint32_t> DDAFileParser::GetInGameDataBlockHeaders()
{
	std::vector<uint32_t> headers;
	headers.push_back(0);

	const DDATextureTableHeader baseHeader = *(DDATextureTableHeader*)(m_fileData.get());
	uint32_t currentHeaderAddressOffset = baseHeader.blockDataSize + DATA_BLOCK_HEADER_SIZE;
	headers.push_back(currentHeaderAddressOffset + 0x70);

	// Get all headers until the end of the file
	DDATextureTableHeader currentHeader = *(DDATextureTableHeader*)(m_fileData.get() + currentHeaderAddressOffset);
	while (strncmp(baseHeader.name, currentHeader.name, DATA_BLOCK_HEADER_NAME_SIZE) != 0)
	{
		currentHeaderAddressOffset += currentHeader.bytesCountBeforeEndFile;
		headers.push_back(currentHeaderAddressOffset + 0x70);
		currentHeader = *(DDATextureTableHeader*)(m_fileData.get() + currentHeaderAddressOffset);
	}
	headers.erase(headers.end() - 1);

	return headers;
}

/**
* @brief Get all texture headers from the menu texture header list
* @brief There is no texture table for the menu textures, so we only have a list of texture headers
*/
std::vector<DDATextureHeader> DDAFileParser::GetMenuTextureHeaders(uint32_t tableAddress)
{
	const size_t textureCount = *(uint32_t*)(m_fileData.get() + tableAddress + sizeof(uint32_t) * 1);
	std::vector<DDATextureHeader> textureHeaders;
	size_t offset = 0;
	for (size_t i = 0; i < textureCount; i++)
	{
		const DDATextureHeader header = *(DDATextureHeader*)(m_fileData.get() + tableAddress + 0x28 + offset);
		textureHeaders.push_back(header);
		offset += 0x90;
	}

	return textureHeaders;
}

/**
* @brief Get the file name from a full file path without the extension
*/
std::string DDAFileParser::GetReducedName(const std::string& fullTextureName)
{
	size_t lastSlash = fullTextureName.find_last_of("\\");
	if (lastSlash == std::string::npos)
	{
		lastSlash = fullTextureName.find_last_of("/");
	}
	std::string reducedFileName = fullTextureName.substr(lastSlash + 1);
	reducedFileName = reducedFileName.substr(0, reducedFileName.find_last_of('.'));
	return reducedFileName;
}

/**
* @brief Export textures from the menu texture header list
*/
std::vector<DDATextureHeader> DDAFileParser::GetMenuTextures(uint32_t textureHeaderListAddress, bool usePalette, std::vector<DDATextureCopyParams>& textureCopyParamsList, const std::string& exportFolder)
{
	const std::vector<DDATextureHeader> headers = GetMenuTextureHeaders(textureHeaderListAddress);

	if(!exportFolder.empty())
	{
		const uint32_t textureTableAddressBefore = textureHeaderListAddress - DATA_BLOCK_HEADER_SIZE;
		const uint32_t texturesDataSize = *(uint32_t*)(m_fileData.get() + textureTableAddressBefore + sizeof(uint32_t) * 2);
		const uint32_t textureHeaderListSize = *(uint32_t*)(m_fileData.get() + textureHeaderListAddress + sizeof(uint32_t) * 2);
		const uint32_t paletteAddress = textureHeaderListSize + texturesDataSize + textureTableAddressBefore;

		for (const DDATextureHeader& header : headers)
		{
			const std::string reducedFileName = GetReducedName(header.filePath);
			const std::string path = exportFolder + reducedFileName + ".png";

			DDATextureCopyParams& textureCopyParams = textureCopyParamsList.emplace_back();
			textureCopyParams.exportWidth = header.width;
			textureCopyParams.exportHeight = header.height;
			textureCopyParams.textureName = reducedFileName;
			std::unique_ptr<uint8_t[]> textureData = std::make_unique<uint8_t[]>(header.width * header.height * sizeof(uint32_t));
			if (!usePalette)
			{
				memcpy(textureData.get(), m_fileData.get() + textureHeaderListAddress + textureHeaderListSize + header.unkown0 - 0x10, header.width * header.height * sizeof(uint8_t) * 4);
				textureCopyParams.clutType = DDAClutType::CLUT_NONE;
			}
			else
			{
				uint8_t* texturePos = m_fileData.get() + textureHeaderListAddress + textureHeaderListSize + header.unkown0 - DATA_BLOCK_HEADER_SIZE;
				const uint8_t* palette = m_fileData.get() + paletteAddress + 0x200 * header.indexInDataChunk * 2;
				std::unique_ptr<uint8_t[]> fixedPalette = GetFixedPalette(palette, DDAClutType::CLUT_256, DDAClutFixType::CLUT_NORMAL);
				textureCopyParams.palette = std::move(fixedPalette);
				textureCopyParams.inputWidth = header.width;
				textureCopyParams.inputHeight = header.height;
				textureCopyParams.outputWidth = header.width;
				textureCopyParams.outputHeight = header.height;
				textureCopyParams.clutType = DDAClutType::CLUT_256;
				textureCopyParams.inputTextureData = texturePos;
			}
			textureCopyParams.outputTextureData = std::move(textureData);
		}
	}

	return headers;
}

/**
* @brief Load a file and extract its data
* @param gameFile The game file to load
* @param exportFolder The folder to export the extracted data to
*/
DDAExtractedData DDAFileParser::LoadFile(const std::string& filePath, DDAGameFile gameFile, const std::string& exportFolder)
{
	DDAExtractedData extractedData;

	m_fileData = ReadFile(filePath);
	if (!m_fileData)
	{
		return extractedData;
	}

	extractedData.fileSize = m_fileSize;

	m_fileType = GetFileType();
	m_gameFile = gameFile;
	if (m_fileType == DDAGameFileType::MAP || m_fileType == DDAGameFileType::CAR)
	{
		const DDATextureTable textureTable = GetTextureTable(*(uint32_t*)(m_fileData.get() + sizeof(uint32_t) * 2), 0x80, true);
		const std::vector<DDAPacketAndTextureEntry> packetAndTextureEntryList = GetPacketAndTextureEntries();
		const uint32_t baseHeaderAddress = *(uint32_t*)(m_fileData.get() + sizeof(uint32_t) * 2);

		extractedData.packetAndTextureEntryList = packetAndTextureEntryList;
		extractedData.textureTables.push_back(textureTable);

		// Extract the skybox texture table if it exists
		// WINBOWL does not have a skybox
		if (gameFile != DDAGameFile::WINBOWL && m_fileType != DDAGameFileType::CAR)
		{
			const uint32_t baseSkyboxDataHeaderAddress = *(uint32_t*)(m_fileData.get() + sizeof(uint32_t) * 1);
			const uint32_t skyboxHeaderAddress = GetSkyboxTextureTableHeader(baseHeaderAddress);
			const DDATextureTable skyboxTextureTable = GetTextureTable(skyboxHeaderAddress, baseSkyboxDataHeaderAddress + 0x80 + 0x80, true);
			extractedData.textureTables.push_back(skyboxTextureTable);
		}
	}
	else if (m_fileType == DDAGameFileType::SPRITES)
	{
		const DDATextureTable spritesTextureTable = GetTextureTable(0, DATA_BLOCK_HEADER_SIZE, true);

		extractedData.textureTables.push_back(spritesTextureTable);
	}
	else if (m_fileType == DDAGameFileType::IN_GAME)
	{
		const std::vector<uint32_t> headers = GetInGameDataBlockHeaders();
		const uint32_t tableAddress = *(uint32_t*)(m_fileData.get() + sizeof(uint32_t) * 2);
		uint32_t offset = 0;
		size_t currentTextureTable = 0;
		do
		{
			const DDATextureTable mapTextureTable = GetTextureTable(tableAddress + offset, headers[currentTextureTable] + DATA_BLOCK_HEADER_SIZE, true);
			currentTextureTable++;
			offset += *(uint32_t*)(m_fileData.get() + tableAddress + sizeof(uint32_t) + offset) + DATA_BLOCK_HEADER_SIZE;

			extractedData.textureTables.push_back(mapTextureTable);
		} while (m_fileType == DDAGameFileType::IN_GAME && tableAddress + offset < m_fileSize - DATA_BLOCK_HEADER_SIZE);
	}
	else if (m_fileType == DDAGameFileType::MENU)
	{
		const std::string reducedFileName = GetReducedName(filePath);
		if (reducedFileName == "DD4FRONT")
		{
			extractedData.textureHeaders.push_back(GetMenuTextures(0x003BCFD0, false, extractedData.textureCopyParamsList, exportFolder));
			extractedData.textureHeaders.push_back(GetMenuTextures(0x0055D7C0, true, extractedData.textureCopyParamsList, exportFolder));
		}
		else if (reducedFileName == "DD4GAME")
		{
			extractedData.textureHeaders.push_back(GetMenuTextures(0x0008D9F0, false, extractedData.textureCopyParamsList, exportFolder));
			extractedData.textureHeaders.push_back(GetMenuTextures(0x0017D3C0, true, extractedData.textureCopyParamsList, exportFolder));
		}
		else if (reducedFileName == "DD4START")
		{
			extractedData.textureHeaders.push_back(GetMenuTextures(0x0009F890, false, extractedData.textureCopyParamsList, exportFolder));
			extractedData.textureHeaders.push_back(GetMenuTextures(0x002222E0, true, extractedData.textureCopyParamsList, exportFolder));
		}
	}

	for(const DDATextureTable& textureTable : extractedData.textureTables)
	{
		for (const DDATextureTableEntry& entry: textureTable.entries)
		{
			CreateTextureCopyParams(extractedData.textureCopyParamsList, entry, m_fileType);
		}
	}

	extractedData.meshes = GenerateMeshes(extractedData.packetAndTextureEntryList);

	return extractedData;
}

std::vector<DDAMesh> DDAFileParser::GenerateMeshes(const std::vector<DDAPacketAndTextureEntry>& packetAndTextureEntryList)
{
	MeshGenerator meshGenerator;
	const std::vector<DDAFileMeshDataInfo> fileMeshDataInfos = meshGenerator.GetMeshDataInfos(m_fileType, m_fileData, packetAndTextureEntryList, false);
	std::vector<DDAMesh> meshes;

	for (const DDAFileMeshDataInfo& vifPacket : fileMeshDataInfos)
	{
		const DDAMesh ddaMesh = meshGenerator.GenerateMeshFromVifPacket(vifPacket, packetAndTextureEntryList, m_fileData, m_fileType);
		meshes.push_back(ddaMesh);
	}

	return meshes;
}

void DDAFileParser::CreateTextureCopyParams(std::vector<DDATextureCopyParams>& textureCopyParamsList, const DDATextureTableEntry& textureEntry, DDAGameFileType gameFileType)
{
	const size_t realWidth = *((uint32_t*)(m_fileData.get() + textureEntry.textureInfosPosition) + 1);
	const size_t realHeight = *((uint32_t*)(m_fileData.get() + textureEntry.textureInfosPosition) + 2);

	const std::string textureFileName = std::string((char*)m_fileData.get() + textureEntry.textureInfosPosition + 16);

	const uint8_t* palette = m_fileData.get() + textureEntry.palettePosition;
	// Cars have two textures with only one texture entry in the table
	size_t subTexturesCount = 1;
	if (gameFileType == DDAGameFileType::CAR && textureEntry.width == 512)
	{
		subTexturesCount = 2;
	}

	for (size_t i = 0; i < subTexturesCount; i++)
	{
		bool isBrokenCarSkin = false;
		if (gameFileType == DDAGameFileType::CAR && textureEntry.width == 512 && i == 1)
		{
			isBrokenCarSkin = true;
		}

		std::unique_ptr<uint8_t[]> textureData = std::make_unique<uint8_t[]>(textureEntry.width * textureEntry.height * sizeof(uint32_t));

		DDAClutFixType paletteFixType = DDAClutFixType::CLUT_NORMAL;
		if (gameFileType == DDAGameFileType::CAR && textureEntry.width == 512)
		{
			// Broken car skin has a different palette fix/swizzle type
			if (isBrokenCarSkin)
			{
				paletteFixType = DDAClutFixType::CLUT_BROKEN_CAR_SKIN;
			}
			else
			{
				paletteFixType = DDAClutFixType::CLUT_CAR_SKIN;
			}
		}

		std::unique_ptr<uint8_t[]> fixedPalette = GetFixedPalette(palette, textureEntry.clutType, paletteFixType);
		DDATextureCopyParams& textureCopyParams = textureCopyParamsList.emplace_back();
		textureCopyParams.palette = std::move(fixedPalette);
		textureCopyParams.inputWidth = textureEntry.width;
		textureCopyParams.inputHeight = textureEntry.height;
		textureCopyParams.outputWidth = realWidth;
		textureCopyParams.outputHeight = realHeight;
		textureCopyParams.exportWidth = realWidth;
		textureCopyParams.exportHeight = realHeight;
		textureCopyParams.clutType = textureEntry.clutType;
		textureCopyParams.inputTextureData = m_fileData.get() + textureEntry.texturePosition;
		textureCopyParams.outputTextureData = std::move(textureData);
		// With 16 colors palette, it's two pixels per byte
		if (textureEntry.clutType == DDAClutType::CLUT_16)
		{
			textureCopyParams.inputWidth /= 2;
			textureCopyParams.outputWidth /= 2;
		}

		// If the texture is a broken car skin, we need to offset the texture data by 256 pixels
		if (isBrokenCarSkin)
		{
			textureCopyParams.xOffset = 256;
		}

		std::string reducedFileName = GetReducedName(textureFileName);
		if (isBrokenCarSkin)
		{
			reducedFileName += "_Broken";
		}
		textureCopyParams.textureName = reducedFileName;
	}
}

/**
* @brief Palette data is "swizzled" and is "unswizzled" with this function
* @param palette The palette data to fix
* @param clutType The type of the CLUT (16 or 256 colors)
* @param fixType The type of the fix to apply (car skin, broken car skin, etc.)
*/
std::unique_ptr<uint8_t[]> DDAFileParser::GetFixedPalette(const uint8_t* palette, DDAClutType clutType, DDAClutFixType fixType)
{
	size_t colorCount = 0;
	if (clutType == DDAClutType::CLUT_16)
	{
		colorCount = 16;
	}
	else if (clutType == DDAClutType::CLUT_256)
	{
		colorCount = 256;
	}

	std::unique_ptr<uint8_t[]> fixedPalette = std::make_unique<uint8_t[]>(colorCount * sizeof(uint32_t));
	if (clutType == DDAClutType::CLUT_16)
	{
		memcpy(fixedPalette.get(), palette, 8 * sizeof(uint32_t));
		memcpy(fixedPalette.get() + 8 * sizeof(uint32_t), palette + 16 * sizeof(uint32_t), 8 * sizeof(uint32_t));
	}
	else if (clutType == DDAClutType::CLUT_256)
	{
		const uint8_t* paletteToUse = palette;
		std::unique_ptr<uint8_t[]> fixedPalette2;
		if (fixType == DDAClutFixType::CLUT_CAR_SKIN || fixType == DDAClutFixType::CLUT_BROKEN_CAR_SKIN)
		{
			fixedPalette2 = std::make_unique<uint8_t[]>(colorCount * sizeof(uint32_t));

			size_t offsetTotal = 0;
			for (uint32_t fixStep = 0; fixStep < 16; fixStep++)
			{
				const uint32_t colorOffset = 16 * fixStep * sizeof(uint32_t);
				if (fixType == DDAClutFixType::CLUT_CAR_SKIN)
				{
					memcpy(fixedPalette2.get() + colorOffset, paletteToUse + colorOffset + offsetTotal, 16 * sizeof(uint32_t)); // For normal texture
				}
				else
				{
					memcpy(fixedPalette2.get() + colorOffset, paletteToUse + colorOffset + offsetTotal + 16 * sizeof(uint32_t), 16 * sizeof(uint32_t)); // For crashed texture
				}
				offsetTotal += 16 * sizeof(uint32_t);
			}
			paletteToUse = fixedPalette2.get();
		}

		// - 8 first colors are correct
		// --- (For every next 32 colors 8 times)
		// --- The 16 next colors are in the wrong order (need to swap 8 colors first color with the 8 next one)
		// --- Then 16 next colors are correct (only the next 7 times)
		// - 8 last colors are correct
		memcpy(fixedPalette.get(), paletteToUse, 8 * sizeof(uint32_t));
		const uint32_t fixStepCount = 8;
		for (uint32_t fixStep = 0; fixStep < fixStepCount; fixStep++)
		{
			const uint32_t colorOffset = 32 * fixStep * sizeof(uint32_t);
			memcpy(fixedPalette.get() + 16 * sizeof(uint32_t) + colorOffset, paletteToUse + 8 * sizeof(uint32_t) + colorOffset, 8 * sizeof(uint32_t));
			memcpy(fixedPalette.get() + 8 * sizeof(uint32_t) + colorOffset, paletteToUse + 16 * sizeof(uint32_t) + colorOffset, 8 * sizeof(uint32_t));
			if (fixStep != fixStepCount - 1)
			{
				memcpy(fixedPalette.get() + 24 * sizeof(uint32_t) + colorOffset, paletteToUse + 24 * sizeof(uint32_t) + colorOffset, 16 * sizeof(uint32_t));
			}
		}
		memcpy(fixedPalette.get() + 248 * sizeof(uint32_t), paletteToUse + 248 * sizeof(uint32_t), 8 * sizeof(uint32_t));
	}

	return fixedPalette;
}

/**
* @brief Read all bytes of a file and return them as a unique pointer
*/
std::unique_ptr<uint8_t[]> DDAFileParser::ReadFile(const std::string& file)
{
	const std::string fileToOpen = file;

	std::fstream fileSteam;
	fileSteam.open(fileToOpen, std::ios::in | std::ios::binary);

	if (!fileSteam.is_open())
	{
		std::cout << "[ERROR] File not opened: " + file << std::endl;
		return nullptr;
	}

	// Get file size
	fileSteam.seekg(0, fileSteam.end);
	m_fileSize = fileSteam.tellg();
	fileSteam.seekg(0, fileSteam.beg);

	// Read file
	std::unique_ptr<uint8_t[]> data = std::make_unique<uint8_t[]>(m_fileSize);
	fileSteam.read((char*)data.get(), m_fileSize);
	fileSteam.close();

	return data;
}

/**
* Get the file type of the loaded file
*/
DDAGameFileType DDAFileParser::GetFileType()
{
	const DDAGameFileType fileType = (DDAGameFileType)(*((uint32_t*)m_fileData.get()));

	return fileType;
}

/**
* @brief Get the packet and texture pair entries from the file
*/
std::vector<DDAPacketAndTextureEntry> DDAFileParser::GetPacketAndTextureEntries()
{
	if (m_fileType != DDAGameFileType::MAP && m_fileType != DDAGameFileType::CAR)
	{
		return std::vector<DDAPacketAndTextureEntry>();
	}

	std::vector<DDAPacketAndTextureEntry> list;

	//------------------------------------------------------------------- Read packet mesh and texture id table
	const uint32_t meshPacketTableEntryCount = *(uint32_t*)(m_fileData.get() + GetHeaderOffset(m_fileType) + sizeof(uint32_t));
	const uint32_t meshPacketTableAddr = static_cast<uint32_t>(*(m_fileData.get() + GetHeaderOffset(m_fileType) + 2 * sizeof(uint32_t)) + GetHeaderOffset(m_fileType)); // CHECK IF TWO HEADEROFFSET IS CORRECT
	const DDAParentDrawCommandEntry* meshPacketTablePtr = (DDAParentDrawCommandEntry*)(m_fileData.get() + meshPacketTableAddr);

	std::vector<DDAParentDrawCommandEntry> meshPacketEntryList;

	for (size_t i = 0; i < meshPacketTableEntryCount; i++)
	{
		const DDAParentDrawCommandEntry meshPacketEntry = *(meshPacketTablePtr + i);
		meshPacketEntryList.push_back(meshPacketEntry);

		for (size_t packetIndex = 0; packetIndex < meshPacketEntry.vifPacketTexturePairCount; packetIndex++)
		{
			DDAPacketAndTextureEntry packetAndTextureEntry = *((DDAPacketAndTextureEntry*)(m_fileData.get() + meshPacketEntry.addr + GetHeaderOffset(m_fileType)) + packetIndex);
			list.push_back(packetAndTextureEntry);
		}
	}

	return list;
}

DDATextureTable DDAFileParser::GetTextureTable(uint32_t tableAddress, uint32_t textureInfoOffset, bool enableLogging)
{
	DDATextureTable textureTable;
	if (m_fileType == DDAGameFileType::MAP || m_fileType == DDAGameFileType::IN_GAME || m_fileType == DDAGameFileType::CAR)
	{
		textureTable.header = *(DDATextureTableHeader*)(m_fileData.get() + tableAddress);

		textureTable.textureCount = textureTable.header.size / sizeof(DDATextureTableEntry);
		textureTable.entries.resize(textureTable.textureCount);
		textureTable.textureNames.resize(textureTable.textureCount);
		const DDATextureTableEntry* entries = (DDATextureTableEntry*)(m_fileData.get() + tableAddress + textureTable.header.offset + DATA_BLOCK_HEADER_SIZE);

		memcpy(textureTable.entries.data(), entries, textureTable.textureCount * sizeof(DDATextureTableEntry));

		size_t index = 0;
		for (DDATextureTableEntry& entry : textureTable.entries)
		{
			textureTable.textureHeaders.push_back(*(DDATextureHeader*)(m_fileData.get() + entry.textureInfosPosition + textureInfoOffset));
			entry.textureInfosPosition += textureInfoOffset;
			entry.palettePosition += textureInfoOffset;
			entry.texturePosition += textureInfoOffset;
			const std::string textureFilePath = std::string((char*)m_fileData.get() + entry.textureInfosPosition + 16);
			const std::string textureName = GetReducedName(textureFilePath);
			textureTable.textureNames[index] = textureName;
			index++;
		}
	}	
	else 
	{
		uint32_t offset = 0;
		do
		{
			DDATextureTableEntry entry = *(DDATextureTableEntry*)(m_fileData.get() + tableAddress + DATA_BLOCK_HEADER_SIZE + offset);
			entry.textureInfosPosition += offset + textureInfoOffset;
			entry.palettePosition += offset + textureInfoOffset;
			entry.texturePosition += offset + textureInfoOffset;

			offset += *(uint32_t*)(m_fileData.get() + tableAddress + sizeof(uint32_t) + offset) + DATA_BLOCK_HEADER_SIZE;
			textureTable.entries.push_back(entry);

			const std::string textureFilePath = std::string((char*)m_fileData.get() + entry.textureInfosPosition + 16);
			const std::string textureName = GetReducedName(textureFilePath);

			textureTable.textureNames.push_back(textureName);
		} while (offset < m_fileSize - DATA_BLOCK_HEADER_SIZE);
	}

	return textureTable;
}

void DDAFileParser::LaunchUnitTests(const std::string& gameFolderPath)
{
	std::cout << "Lauching tests:" << std::endl;

	// Maps
	LaunchUnitTest(gameFolderPath, DDAGameFile::AIRPORT, 0x641710, 283, 4233);
	LaunchUnitTest(gameFolderPath, DDAGameFile::BMOVIE, 0x2C5890, 145, 1090);
	LaunchUnitTest(gameFolderPath, DDAGameFile::BRON_2ND, 0x826890, 312, 4116);
	LaunchUnitTest(gameFolderPath, DDAGameFile::BRONX, 0x8DD3B0, 351, 5415);
	LaunchUnitTest(gameFolderPath, DDAGameFile::CHIN_2ND, 0x790A90, 334, 5191);
	LaunchUnitTest(gameFolderPath, DDAGameFile::CHINATWN, 0x7B1EB0, 317, 5379);
	LaunchUnitTest(gameFolderPath, DDAGameFile::CONSTR, 0x59CB90, 187, 4002);
	LaunchUnitTest(gameFolderPath, DDAGameFile::DAM, 0x9AB1D0, 183, 6719);
	LaunchUnitTest(gameFolderPath, DDAGameFile::ENGINE, 0x30FC10, 65, 1760);
	LaunchUnitTest(gameFolderPath, DDAGameFile::GLADIATO, 0x44B680, 84, 1924);
	LaunchUnitTest(gameFolderPath, DDAGameFile::GODS, 0x247F50, 42, 829);
	LaunchUnitTest(gameFolderPath, DDAGameFile::JUSTICE, 0x3EF860, 128, 1709);
	LaunchUnitTest(gameFolderPath, DDAGameFile::REFINERY, 0x8C8610, 217, 6037);
	LaunchUnitTest(gameFolderPath, DDAGameFile::SHIPYARD, 0x6F82A0, 272, 4779);
	LaunchUnitTest(gameFolderPath, DDAGameFile::STEELWRK, 0x4ECAB0, 147, 3534);
	LaunchUnitTest(gameFolderPath, DDAGameFile::SUBWAY, 0x67F470, 194, 4800);
	LaunchUnitTest(gameFolderPath, DDAGameFile::VEGA_2ND, 0x88CBC0, 417, 7436);
	LaunchUnitTest(gameFolderPath, DDAGameFile::VEGAS, 0x6B97C0, 364, 5531);
	LaunchUnitTest(gameFolderPath, DDAGameFile::WINBOWL, 0x1DF2E0, 54, 1287);

	// Cars
	LaunchUnitTest(gameFolderPath, DDAGameFile::AMSTAR, 0x00040480, 1, 0);
	LaunchUnitTest(gameFolderPath, DDAGameFile::BLACK, 0x00048890, 1, 0);
	LaunchUnitTest(gameFolderPath, DDAGameFile::CLOWN, 0x000428D0, 1, 0);
	LaunchUnitTest(gameFolderPath, DDAGameFile::DEVIL, 0x00046210, 1, 0);
	LaunchUnitTest(gameFolderPath, DDAGameFile::FLAME, 0x00041E20, 1, 0);
	LaunchUnitTest(gameFolderPath, DDAGameFile::FLASH, 0x00049A50, 1, 0);
	LaunchUnitTest(gameFolderPath, DDAGameFile::FORMULA, 0x0004CAC0, 1, 0);
	LaunchUnitTest(gameFolderPath, DDAGameFile::GIRL, 0x00047BB0, 1, 0);
	LaunchUnitTest(gameFolderPath, DDAGameFile::MIDNIGH, 0x0004A460, 1, 0);
	LaunchUnitTest(gameFolderPath, DDAGameFile::MONSTER, 0x00046020, 1, 0);
	LaunchUnitTest(gameFolderPath, DDAGameFile::PEPSI, 0x00045650, 1, 0);
	LaunchUnitTest(gameFolderPath, DDAGameFile::RIVER, 0x000490A0, 1, 0);
	LaunchUnitTest(gameFolderPath, DDAGameFile::SHARK, 0x00047630, 1, 0);
	LaunchUnitTest(gameFolderPath, DDAGameFile::SKULL, 0x0004B100, 1, 0);
	LaunchUnitTest(gameFolderPath, DDAGameFile::SNAKE, 0x0004A060, 1, 0);
	LaunchUnitTest(gameFolderPath, DDAGameFile::STANG, 0x00048130, 1, 0);
	LaunchUnitTest(gameFolderPath, DDAGameFile::STAR, 0x0003C0B0, 1, 0);
	LaunchUnitTest(gameFolderPath, DDAGameFile::TIGER, 0x00048F30, 1, 0);
	LaunchUnitTest(gameFolderPath, DDAGameFile::UNION, 0x000428C0, 1, 0);
	LaunchUnitTest(gameFolderPath, DDAGameFile::VOODOO, 0x00046520, 2, 0);
	LaunchUnitTest(gameFolderPath, DDAGameFile::ZACE, 0x0004A2D0, 2, 0);
	LaunchUnitTest(gameFolderPath, DDAGameFile::ZGMC, 0x0004FEF0, 1, 0);
	LaunchUnitTest(gameFolderPath, DDAGameFile::ZHOTTIE, 0x00047690, 2, 0);
	LaunchUnitTest(gameFolderPath, DDAGameFile::ZPOLICE, 0x0004C990, 2, 0);
	LaunchUnitTest(gameFolderPath, DDAGameFile::ZTAXI, 0x0004E340, 1, 0);

	// Other files
	LaunchUnitTest(gameFolderPath, DDAGameFile::SPRITES, 0x000912E0, 67, 0);
	LaunchUnitTest(gameFolderPath, DDAGameFile::INGAME, 0x00196D00, 102, 0);

	// Menu files
	LaunchUnitTest(gameFolderPath, DDAGameFile::DD4FRONT, 0xA30F40, 985, 0);
	LaunchUnitTest(gameFolderPath, DDAGameFile::DD4GAME, 0x1F4B60, 314, 0);
	LaunchUnitTest(gameFolderPath, DDAGameFile::DD4START, 0x2BC160, 326, 0);
}

void DDAFileParser::LaunchUnitTest(const std::string& gameFolderPath, DDAGameFile gameFile, size_t expectedFileSize, size_t expectedTextureCount, size_t expectedMeshPacketCount)
{
	bool passed = true;
	DDAExtractedData data = LoadFile(gameFolderPath, gameFile, "");

	if (m_fileSize != expectedFileSize)
	{
		std::cout << "[ERROR] Test not passed: wrong file size for " + filesNames[(int)gameFile] + ", expected: " + std::to_string(expectedFileSize) + ", actual: " + std::to_string(m_fileSize) << std::endl;
	}

	bool hasTextures = !data.textureTables.empty() || !data.textureHeaders.empty();
	if(data.textureTables.empty() && data.textureHeaders.empty())
	{
		std::cout << "[ERROR] Test not passed: No texture found for " + filesNames[(int)gameFile] << std::endl;
		passed = false;
	}

	if (hasTextures)
	{
		size_t textureCount = 0;
		if (!data.textureTables.empty())
		{
			for(const DDATextureTable& textureTable : data.textureTables)
			{
				textureCount += textureTable.entries.size();
			}
		}
		else 
		{
			for (const std::vector<DDATextureHeader>& textureHeaderList : data.textureHeaders)
			{
				textureCount += textureHeaderList.size();
			}
		}
		if (textureCount != expectedTextureCount)
		{
			std::cout << "[ERROR] Test not passed: No wrong texture count for " + filesNames[(int)gameFile] + ", expected: " + std::to_string(expectedTextureCount) + ", actual: " + std::to_string(textureCount) << std::endl;
		}
	}

	if (passed)
	{
		std::cout << "Test passed " + filesNames[(int)gameFile] << std::endl;
	}
}