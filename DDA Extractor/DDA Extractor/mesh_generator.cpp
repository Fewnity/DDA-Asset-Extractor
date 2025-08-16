// SPDX-License-Identifier: MIT
//
// Copyright (c) 2025-2025 Gregory Machefer (Fewnity)
//
// This file is part of DDA Extractor.

#include "mesh_generator.h"

#include <iostream>

DDAVector3 MeshGenerator::GetMeshCenter(const uint8_t* posPart0, const uint8_t* posPart1)
{
	const DDAVector3 positionA = DDAVector3(*((float*)(posPart0)+0), *((float*)(posPart0)+1), *((float*)(posPart0)+2));
	const DDAVector3 positionB = DDAVector3(*((float*)(posPart1)+0), *((float*)(posPart1)+1), *((float*)(posPart1)+2));
	DDAVector3 finalPosition = (positionB - positionA);
	finalPosition.y = -finalPosition.y;
	finalPosition.z = -finalPosition.z;
	return finalPosition / 4.0f;
}

float MeshGenerator::GetScaleAxis(uint8_t multiplier, uint8_t scaleValue)
{
	// See if there is another way to get the scale, like reading the float value
	float finalScale = 0;

	finalScale = 64.0f;
	uint8_t i = scaleValue;
	while (0x49 - i > 0)
	{
		finalScale /= 4.0f;
		i++;
	}

	if (multiplier >= 0x80)
	{
		finalScale *= 2.0f;
	}

	return finalScale;
}

float MeshGenerator::ShortToFloat(const uint8_t* data)
{
	const float v = (int16_t)((data[0 + 1] & 0xff) << 8 | (data[0] & 0xff));
	return v / std::powf(2, 8);
}

DDAMesh MeshGenerator::GenerateMeshFromVifPacket(const DDAFileMeshDataInfo& vifPacket, const std::vector<DDAPacketAndTextureEntry>& packetAndTextureEntryList, const std::unique_ptr<uint8_t[]>& fileData, DDAGameFileType fileType)
{
	float uvDiviserX = 16;
	float uvDiviserY = 16;
	float uvOffset = 0;

	DDAMesh mesh;

	const uint8_t* meshScaleData = (uint8_t*)fileData.get() + vifPacket.meshPositionAndSizeA;
	const uint8_t* boundingBoxData = (uint8_t*)fileData.get() + vifPacket.meshPositionB;

	// Get mesh position and scale
	const uint8_t scaleMultiplierX = *((uint8_t*)(meshScaleData)+2);
	const uint8_t scaleMultiplierY = *((uint8_t*)(meshScaleData)+6);
	const uint8_t scaleMultiplierZ = *((uint8_t*)(meshScaleData)+10);
	const uint8_t scaleX2 = *((uint8_t*)(meshScaleData)+2 + 1);
	const uint8_t scaleY2 = *((uint8_t*)(meshScaleData)+6 + 1);
	const uint8_t scaleZ2 = *((uint8_t*)(meshScaleData)+10 + 1);

	const DDAVector3 boxSize = DDAVector3(GetScaleAxis(scaleMultiplierX, scaleX2), GetScaleAxis(scaleMultiplierY, scaleY2), GetScaleAxis(scaleMultiplierZ, scaleZ2));
	const DDAVector3 position = GetMeshCenter(meshScaleData, boundingBoxData);

	// ------------------------------------------------------ Read mesh vertices data

	// Get pointers to the data
	const uint8_t* verticesPosData = (uint8_t*)fileData.get() + vifPacket.verticesPositionLocation;
	const uint8_t* uvdata = (uint8_t*)fileData.get() + vifPacket.uvPositionLocation;
	const uint8_t* colordata = (uint8_t*)fileData.get() + vifPacket.verticesColorsLocation;
	const uint8_t* normalData = (uint8_t*)fileData.get() + vifPacket.verticesColorsLocation;

	std::vector<DDAVector3> verticesPositions;
	std::vector<DDAVector2> verticesUVs;
	std::vector<DDAVector3> verticesNormals;
	std::vector<DDAColor> verticesColors;

	const size_t stripVertexCount = vifPacket.vertexCount;

	std::vector<int> endOfStripAt;

	// ------------------------------------------------------ Get vertices positions
	for (size_t vertexIndex = 0; vertexIndex < stripVertexCount; vertexIndex++)
	{
		const size_t byteOffset = vertexIndex * (sizeof(uint16_t) * 3);

		// Position are unsigned shorts
		const int32_t x = (int32_t)(*(uint16_t*)(verticesPosData + byteOffset));
		const int32_t y = (int32_t)(*(uint16_t*)(verticesPosData + 2 + byteOffset));
		const int32_t z = (int32_t)(*(uint16_t*)(verticesPosData + 4 + byteOffset));

		// Reduce the scale of the mesh
		const float scaledX = (x / 4096.0f * boxSize.x) - position.x;
		const float scaledY = (y / 4096.0f * boxSize.y) + position.y;
		const float scaledZ = (z / 4096.0f * boxSize.z) + position.z;

		verticesPositions.push_back(DDAVector3(scaledX, scaledY, scaledZ));
	}

	// ------------------------------------------------------ Get vertices uv and detect triangle strips
	bool firstStrip = true;
	bool secondStripVertex = false;
	for (size_t vertexIndex = 0; vertexIndex < stripVertexCount; vertexIndex++)
	{
		const size_t byteOffset = vertexIndex * (sizeof(uint16_t) * 2);

		// A flag is hidden in the first byte of the vertex uvs to end a triangle strip 
		const uint8_t firstByte = *(uvdata + byteOffset);

		// Check first bit
		if (firstByte & 0x1)
		{
			if (firstStrip)
			{
				firstStrip = false;
			}
			else
			{
				if (!secondStripVertex)
				{
					secondStripVertex = true;
				}
				else
				{
					endOfStripAt.push_back(static_cast<int>(vertexIndex));
					secondStripVertex = false;
				}
			}
		}
		const float u = ShortToFloat(uvdata + byteOffset);
		const float v = ShortToFloat(uvdata + 2 + byteOffset);

		verticesUVs.push_back(DDAVector2((u + uvOffset) / uvDiviserX, (v + uvOffset) / uvDiviserY * 1));
	}
	if (fileType == DDAGameFileType::MAP)
	{
		for (size_t vertexIndex = 0; vertexIndex < stripVertexCount; vertexIndex++)
		{
			const size_t byteOffset = vertexIndex * (sizeof(uint8_t) * 3);
			const uint8_t r = *(uint8_t*)(colordata + byteOffset);
			const uint8_t g = *(uint8_t*)(colordata + 1 + byteOffset);
			const uint8_t b = *(uint8_t*)(colordata + 2 + byteOffset);

			verticesColors.push_back(DDAColor((r * 2.0f) / 255.0f, (g * 2.0f) / 255.0f, (b * 2.0f) / 255.0f, 1)); // * 2 to make it brighter like in game
		}
	}
	else if (fileType == DDAGameFileType::CAR)
	{
		for (size_t vertexIndex = 0; vertexIndex < stripVertexCount; vertexIndex++)
		{
			const size_t byteOffset = vertexIndex * (sizeof(uint8_t) * 3);
			const uint8_t x = *(uint8_t*)(normalData + byteOffset);
			const uint8_t y = *(uint8_t*)(normalData + 1 + byteOffset);
			const uint8_t z = *(uint8_t*)(normalData + 2 + byteOffset);

			verticesNormals.push_back(DDAVector3(x / 255.0f, y / 255.0f, z / 255.0f));
		}
	}

	size_t subMeshVertexCount = 0;
	size_t subMeshTriangleCount = 0;
	size_t lastStripEnd = 0;
	for (auto& endStrip : endOfStripAt)
	{
		subMeshTriangleCount += (endStrip - lastStripEnd) - 2;
		lastStripEnd = endStrip;
	}
	if (endOfStripAt.size() == 0)
	{
		subMeshTriangleCount = stripVertexCount - 2;
	}
	else
	{
		subMeshTriangleCount += (stripVertexCount - lastStripEnd) - 2;
	}
	subMeshVertexCount = subMeshTriangleCount * 3;

	// Create mesh data
	DDAVertexDescriptor vertexDescriptor = DDAVertexDescriptor();
	vertexDescriptor.elements |= DDAVertexElement::UV_32_BITS;
	if (fileType == DDAGameFileType::MAP)
	{
		vertexDescriptor.elements |= DDAVertexElement::COLOR_4_FLOATS;
	}
	else
	{
		vertexDescriptor.elements |= DDAVertexElement::NORMAL_32_BITS;
	}
	vertexDescriptor.elements |= DDAVertexElement::POSITION_32_BITS;
	mesh.vertexDescriptor = vertexDescriptor;

	DDASubMesh& subMesh = mesh.subMeshes.emplace_back();

	const uint32_t textureIndex = packetAndTextureEntryList[vifPacket.parentPacketIndex].textureIndex;
	subMesh.materialIndex = textureIndex;

	subMesh.verticesPositions.resize(subMeshVertexCount);
	subMesh.verticesUVs.resize(subMeshVertexCount);
	if (fileType == DDAGameFileType::CAR)
	{
		subMesh.verticesNormals.resize(subMeshVertexCount);
	}
	else if (fileType == DDAGameFileType::MAP)
	{
		subMesh.verticesColors.resize(subMeshVertexCount);
	}

	int stripCount = 0;
	int currentVertexIndex = 0;
	int currentVertexInStripIndex = 0;
	for (int endStrip : endOfStripAt)
	{
		for (int i = 0; i < endStrip - stripCount; i++)
		{
			if (i >= 2)
			{
				AddTriangleToMesh(subMesh, fileType, verticesPositions, verticesUVs, verticesNormals, verticesColors, currentVertexIndex, currentVertexInStripIndex);
				currentVertexIndex += 3;
			}
			currentVertexInStripIndex++;
		}
		stripCount = endStrip;
	}
	for (int i = 0; i < stripVertexCount - stripCount; i++)
	{
		if (i >= 2)
		{
			AddTriangleToMesh(subMesh, fileType, verticesPositions, verticesUVs, verticesNormals, verticesColors, currentVertexIndex, currentVertexInStripIndex);
			currentVertexIndex += 3;
		}
		currentVertexInStripIndex++;
	}

	return mesh;
}

/**
* @brief Create a list of all mesh data packets
*/
std::vector<DDAFileMeshDataInfo> MeshGenerator::GetMeshDataInfos(DDAGameFileType fileType, const std::unique_ptr<uint8_t[]>& fileData, const std::vector<DDAPacketAndTextureEntry>& packetAndTextureEntryList, bool enableLogging)
{
	std::vector<DDAFileMeshDataInfo> list;

	size_t abortCount = 0;
	const size_t packetAndTextureEntryListCount = packetAndTextureEntryList.size();

	// Each parent packet, contains a list of small mesh packets
	for (size_t packetIndex = 0; packetIndex < packetAndTextureEntryListCount; packetIndex++)
	{
		const uint16_t bigPacketSize = *(uint16_t*)(fileData.get() + packetAndTextureEntryList[packetIndex].vifPacketListAddr + GetHeaderOffset(fileType)) * 16; // Size in bytes
		const uint32_t packetStart = packetAndTextureEntryList[packetIndex].vifPacketListAddr + GetHeaderOffset(fileType);
		const uint32_t packetEnd = packetStart + bigPacketSize;

		for (size_t byteIndex = packetStart; byteIndex < packetEnd; byteIndex++)
		{
			//------------------------------------------------------------------- Find mesh packet
			// If an unpack with a gif tag right after is found
			if (fileData[byteIndex + 1] == 0x80 &&
				fileData[byteIndex + 2] == 0x01 &&
				fileData[byteIndex + 3] == 0x6C &&
				fileData[byteIndex + 5] == 0x80)
			{
				const uint8_t verticesCount = fileData[byteIndex + 4];
				const size_t gifTagUnpackPosition = byteIndex;
				byteIndex += 5 * sizeof(uint32_t);

				// Variable use to abort the search if the data format does not match
				bool abort = false;

				//------------------------------------------------------------------- Find first mesh bounding box data location
				size_t meshPositionStart = 0;
				while (!abort)
				{
					// If a STROW is found
					if (fileData[byteIndex + 0] == 0 &&
						fileData[byteIndex + 1] == 0 &&
						fileData[byteIndex + 2] == 0 &&
						fileData[byteIndex + 3] == 0x30)
					{
						if (byteIndex - gifTagUnpackPosition != 20)
						{
							if (enableLogging)
							{
								std::cout << std::to_string(byteIndex - gifTagUnpackPosition) << std::endl; // Can be 43 on cars, why?
							}
						}
						meshPositionStart = byteIndex;
						byteIndex += sizeof(uint32_t) + 3 * sizeof(float);
						break;
					}

					byteIndex += sizeof(uint32_t);
					if (byteIndex > gifTagUnpackPosition + 16 * sizeof(uint32_t))
					{
						abort = true;
						if (enableLogging)
						{
							std::cout << "[ERROR] Abort when searching for Mesh Position A" << std::endl;
						}
						byteIndex = gifTagUnpackPosition;
					}
				}


				//------------------------------------------------------------------- Find vertices positions data location
				size_t verticesPositionUnpackStart = 0;
				while (!abort)
				{
					// If a position unpack V3_16 is found
					if (fileData[byteIndex + 0] == 0x02 &&
						fileData[byteIndex + 1] == 0xC0 &&
						fileData[byteIndex + 2] >= 3 && // Check if vertices count is 3 or more
						fileData[byteIndex + 3] == 0x69)
					{
						verticesPositionUnpackStart = byteIndex;
						if (fileData[byteIndex + 2] != verticesCount)
						{
							if (enableLogging)
							{
								std::cout << "[ERROR] Vertices count mismatch: " + std::to_string(fileData[byteIndex + 3]) + " != " + std::to_string(verticesCount) + " at " + std::to_string(byteIndex) + " in file " << std::endl;
							}
						}

						// Move file cursor
						const uint32_t positionBytesSize = sizeof(uint32_t) + verticesCount * 3 * sizeof(uint16_t);
						const uint32_t padding = positionBytesSize % sizeof(uint32_t);
						byteIndex += positionBytesSize + padding;
						break;
					}
					else if (fileData[byteIndex + 1] == 0x80 &&
						fileData[byteIndex + 2] == 0x01 &&
						fileData[byteIndex + 3] == 0x6C &&
						fileData[byteIndex + 5] == 0x80) // If an unpack with a gif tag right after is found
					{
						abort = true;
						if (enableLogging)
						{
							std::cout << "[ERROR] Abort when searching for Vertices Position A" << std::endl;
						}
						break;
					}
					byteIndex += sizeof(uint32_t);
					// Do not search too far
					if (byteIndex > gifTagUnpackPosition + 16 * sizeof(uint32_t))
					{
						abort = true;
						if (enableLogging)
						{
							std::cout << "[ERROR] Abort when searching for Vertices Position B" << std::endl;
						}
						byteIndex = gifTagUnpackPosition + 8;
						break;
					}
				}


				//------------------------------------------------------------------- Find second mesh bounding box data location
				size_t meshBoundingBoxStart = 0;
				while (!abort)
				{
					// If an unpack V3_32 is found
					if (fileData[byteIndex + 0] == 1 &&
						fileData[byteIndex + 1] == 0x80 &&
						fileData[byteIndex + 2] == 0x01 &&
						fileData[byteIndex + 3] == 0x68)
					{
						meshBoundingBoxStart = byteIndex;
						byteIndex += sizeof(uint32_t) + 3 * sizeof(float);
						break;
					}
					byteIndex += sizeof(uint32_t);
				}


				//------------------------------------------------------------------- Find vertices UV data location
				size_t verticesUvUnpackStart = 0;
				while (!abort)
				{
					// If an unpack V2_16 is found
					if (fileData[byteIndex + 0] == 0x50 &&
						fileData[byteIndex + 1] == 0x80 &&
						fileData[byteIndex + 2] == verticesCount &&
						fileData[byteIndex + 3] == 0x65)
					{
						verticesUvUnpackStart = byteIndex;
						// Move file cursor
						const uint32_t positionBytesSize = 4 + verticesCount * 2 * sizeof(uint16_t);
						const uint32_t padding = positionBytesSize % 4;
						byteIndex += positionBytesSize + padding;

						break;
					}

					byteIndex += sizeof(uint32_t);
				}

				//------------------------------------------------------------------- Find vertices color data location
				size_t verticesColorUnpackStart = 0;
				while (!abort)
				{
					// If an unpack V3_8 is found
					if (fileData[byteIndex + 0] == 0x9E &&
						//fileData[byteIndex + 1] == 0xC0 && // Not 0xC0 on cars
						fileData[byteIndex + 2] == verticesCount &&
						fileData[byteIndex + 3] == 0x6A)
					{
						verticesColorUnpackStart = byteIndex;

						// Move file cursor
						const uint32_t positionBytesSize = 4 + verticesCount * 3 * sizeof(uint8_t);
						const uint32_t padding = positionBytesSize % 4;

						byteIndex += positionBytesSize + padding;
						break;
					}
				}

				if (!abort)
				{
					DDAFileMeshDataInfo fileMeshDataInfo;
					fileMeshDataInfo.verticesPositionLocation = verticesPositionUnpackStart + sizeof(uint32_t);
					fileMeshDataInfo.uvPositionLocation = verticesUvUnpackStart + sizeof(uint32_t);
					fileMeshDataInfo.vertexCount = verticesCount;
					fileMeshDataInfo.meshPositionB = meshBoundingBoxStart + sizeof(uint32_t);
					fileMeshDataInfo.meshPositionAndSizeA = meshPositionStart + sizeof(uint32_t);
					fileMeshDataInfo.verticesColorsLocation = verticesColorUnpackStart + sizeof(uint32_t);
					fileMeshDataInfo.parentPacketIndex = packetIndex;
					list.push_back(fileMeshDataInfo);
				}
				else
				{
					abortCount++;
				}
			}
		}
	}

	if (enableLogging)
	{
		std::cout << "Abort count: " + std::to_string(abortCount) << std::endl;
		std::cout << "Found packet count: " + std::to_string(list.size()) << std::endl;
	}

	return list;
}

void MeshGenerator::AddTriangleToMesh(DDASubMesh& subMesh, DDAGameFileType fileType, const std::vector<DDAVector3>& verticesPositions, const std::vector<DDAVector2>& verticesUVs, const std::vector<DDAVector3>& verticesNormals, const std::vector<DDAColor>& verticesColors, int currentVertexIndex, int currentVertexInStripIndex)
{
	if (fileType == DDAGameFileType::MAP)
	{
		subMesh.SetVertex(
			verticesUVs[currentVertexInStripIndex - 2].x, verticesUVs[currentVertexInStripIndex - 2].y,
			verticesColors[currentVertexInStripIndex - 2],
			verticesPositions[currentVertexInStripIndex - 2].x, verticesPositions[currentVertexInStripIndex - 2].y, verticesPositions[currentVertexInStripIndex - 2].z,
			currentVertexIndex);

		subMesh.SetVertex(
			verticesUVs[currentVertexInStripIndex - 1].x, verticesUVs[currentVertexInStripIndex - 1].y,
			verticesColors[currentVertexInStripIndex - 1],
			verticesPositions[currentVertexInStripIndex - 1].x, verticesPositions[currentVertexInStripIndex - 1].y, verticesPositions[currentVertexInStripIndex - 1].z,
			currentVertexIndex + 1);

		subMesh.SetVertex(
			verticesUVs[currentVertexInStripIndex - 0].x, verticesUVs[currentVertexInStripIndex - 0].y,
			verticesColors[currentVertexInStripIndex - 0],
			verticesPositions[currentVertexInStripIndex - 0].x, verticesPositions[currentVertexInStripIndex - 0].y, verticesPositions[currentVertexInStripIndex - 0].z,
			currentVertexIndex + 2);
	}
	else
	{
		subMesh.SetVertex(
			verticesUVs[currentVertexInStripIndex - 2].x, verticesUVs[currentVertexInStripIndex - 2].y,
			verticesNormals[currentVertexInStripIndex - 2].x, verticesNormals[currentVertexInStripIndex - 2].y, verticesNormals[currentVertexInStripIndex - 2].z,
			verticesPositions[currentVertexInStripIndex - 2].x, verticesPositions[currentVertexInStripIndex - 2].y, verticesPositions[currentVertexInStripIndex - 2].z,
			currentVertexIndex);

		subMesh.SetVertex(
			verticesUVs[currentVertexInStripIndex - 1].x, verticesUVs[currentVertexInStripIndex - 1].y,
			verticesNormals[currentVertexInStripIndex - 1].x, verticesNormals[currentVertexInStripIndex - 1].y, verticesNormals[currentVertexInStripIndex - 1].z,
			verticesPositions[currentVertexInStripIndex - 1].x, verticesPositions[currentVertexInStripIndex - 1].y, verticesPositions[currentVertexInStripIndex - 1].z,
			currentVertexIndex + 1);

		subMesh.SetVertex(
			verticesUVs[currentVertexInStripIndex - 0].x, verticesUVs[currentVertexInStripIndex - 0].y,
			verticesNormals[currentVertexInStripIndex - 0].x, verticesNormals[currentVertexInStripIndex - 0].y, verticesNormals[currentVertexInStripIndex - 0].z,
			verticesPositions[currentVertexInStripIndex - 0].x, verticesPositions[currentVertexInStripIndex - 0].y, verticesPositions[currentVertexInStripIndex - 0].z,
			currentVertexIndex + 2);
	}
}