// SPDX-License-Identifier: MIT
//
// Copyright (c) 2025-2025 Gregory Machefer (Fewnity)
//
// This file is part of DDA Extractor.

#pragma once

#include <vector>

#include "dda_structures.h"

class MeshGenerator
{
public:
	DDAMesh GenerateMeshFromVifPacket(const DDAFileMeshDataInfo& vifPacket, const std::vector<DDAPacketAndTextureEntry>& packetAndTextureEntryList, const std::unique_ptr<uint8_t[]>& fileData, DDAGameFileType fileType);
	std::vector<DDAFileMeshDataInfo> GetMeshDataInfos(DDAGameFileType fileType, const std::unique_ptr<uint8_t[]>& fileData, const std::vector<DDAPacketAndTextureEntry>& packetAndTextureEntryList, bool enableLogging);

private:
	DDAVector3 GetMeshCenter(const uint8_t* posPart0, const uint8_t* posPart1);
	float GetScaleAxis(uint8_t multiplier, uint8_t scaleValue);
	float ShortToFloat(const uint8_t* data);
	void AddTriangleToMesh(DDASubMesh& subMesh, DDAGameFileType fileType, const std::vector<DDAVector3>& verticesPositions, const std::vector<DDAVector2>& verticesUVs, const std::vector<DDAVector3>& verticesNormals, const std::vector<DDAColor>& verticesColors, int currentVertexIndex, int currentVertexInStripIndex);
};

