// SPDX-License-Identifier: MIT
//
// Copyright (c) 2025-2025 Gregory Machefer (Fewnity)
//
// This file is part of DDA Extractor.

#pragma once

#include <string>
#include <memory>
#include <vector>

#include "dda_structures.h"

class Material;
class GameObject;

class DDAFileParser
{
public:
	DDAExtractedData LoadFile(const std::string& filePath, DDAGameFile gameFile, const std::string& exportFolder);
	void LaunchUnitTests(const std::string& gameFolderPath);

private:
	std::unique_ptr<uint8_t[]> ReadFile(const std::string& file);
	DDAGameFileType GetFileType();
	std::vector<DDAPacketAndTextureEntry> GetPacketAndTextureEntries();
	DDATextureTable GetTextureTable(uint32_t tableAddress, uint32_t textureInfoOffset, bool enableLogging);

	uint32_t GetSkyboxTextureTableHeader(uint32_t baseHeaderAddress);
	void CopyTextureData(const DDATextureCopyParams& params);
	std::unique_ptr<uint8_t[]> GetFixedPalette(const uint8_t* palette, DDAClutType clutType, DDAClutFixType fixType);
	std::vector<uint32_t> GetInGameDataBlockHeaders();
	std::vector<DDATextureHeader> GetMenuTextureHeaders(uint32_t tableAddress);
	std::string GetReducedName(const std::string& fullTextureName);
	std::vector<DDATextureHeader> GetMenuTextures(uint32_t textureTableAddress, bool usePalette, std::vector<DDATextureCopyParams>& textureCopyParamsList, const std::string& exportFolder);
	void CreateTextureCopyParams(std::vector<DDATextureCopyParams>& textureCopyParamsList, const DDATextureTableEntry& textureEntry, DDAGameFileType gameFileType);
	std::vector<DDAMesh> GenerateMeshes(const std::vector<DDAPacketAndTextureEntry>& packetAndTextureEntryList);
	

	void LaunchUnitTest(const std::string& gameFolderPath, DDAGameFile gameFile, size_t expectedFileSize, size_t expectedTextureCount, size_t expectedMeshPacketCount);
	
	size_t maxObjectToSpawn = 9999;
	std::unique_ptr<uint8_t[]> m_fileData;
	bool groupMeshByMaterial = false;
	DDAGameFileType m_fileType = DDAGameFileType::CAR;
	size_t m_fileSize = 0;
	std::vector<std::shared_ptr<Material>> materials;
	DDAGameFile m_gameFile;
};