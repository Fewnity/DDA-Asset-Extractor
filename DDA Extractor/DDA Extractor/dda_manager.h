// SPDX-License-Identifier: MIT
//
// Copyright (c) 2025-2025 Gregory Machefer (Fewnity)
//
// This file is part of DDA Extractor.

#pragma once

#include <vector>
#include <memory>

#include "dda_structures.h"

class DDAManager
{
public:
	DDAManager(const std::string& gameFolderPath)
		: m_gameFolderPath(gameFolderPath) {
	}

	/**
	* @brief Extracts data from the specified game file and saves it to the export folder.
	* @brief Extracts meshes and textures.
	*/
	void ExtractData(DDAGameFile gameFile, const std::string& exportFolder);

private:
	void CreateFXBMesh(const std::vector<DDAMesh>& meshes, const std::vector<DDATextureTable>& textureTableList, const std::string& exportFolder);
	std::string m_gameFolderPath;
};

