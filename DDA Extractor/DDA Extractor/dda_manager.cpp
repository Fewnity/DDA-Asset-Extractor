// SPDX-License-Identifier: MIT
//
// Copyright (c) 2025-2025 Gregory Machefer (Fewnity)
//
// This file is part of DDA Extractor.

#include "dda_manager.h"

#include <filesystem>

#include <assimp/Exporter.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "dda_file_parser.h"
#include "texture_dumper.h"
#include "mesh_generator.h"
#include <iostream>

// Top fix the map in blender, you have to follow these steps:
// A (Select all meshes)
// Tab (Use edit mode)
// Shift + D (duplicate the mesh)
// Right click (abort the move meshes action)
// Alt + N (Open the Mesh->Normals menu)
// F (Flip normals)

void DDAManager::CreateFXBMesh(const std::vector<DDAMesh>& meshes, const std::vector<DDATextureTable>& textureTableList, const std::string& exportFolder)
{
	const unsigned int meshCount = static_cast<unsigned int>(meshes.size());

	std::vector<aiMaterial*> assimpMaterials;

	for (const DDATextureTable& textureTable : textureTableList)
	{
		const std::vector<DDATextureTableEntry>& entryList = textureTable.entries;
		const size_t textureCount = entryList.size();

		for (size_t i = 0; i < textureCount; i++)
		{
			const std::string textureName = textureTable.textureNames[i];
			aiMaterial* assimpMaterial = new aiMaterial();
			const aiColor3D diffuseColor(1.0f, 1.0f, 1.0f);
			assimpMaterial->AddProperty(&diffuseColor, 1, AI_MATKEY_COLOR_DIFFUSE);

			const aiString name(textureName);
			const aiString assimpPextureName(textureName + ".png");
			assimpMaterial->AddProperty(&name, AI_MATKEY_NAME);
			assimpMaterial->AddProperty(&assimpPextureName, AI_MATKEY_TEXTURE_DIFFUSE(0));

			assimpMaterials.push_back(assimpMaterial);
		}
	}

	const size_t materialCount = assimpMaterials.size();

	aiScene* scene = new aiScene();
	scene->mRootNode = new aiNode();
	scene->mRootNode->mNumMeshes = 0;
	scene->mRootNode->mNumChildren = meshCount;
	scene->mRootNode->mChildren = new aiNode * [meshCount];

	scene->mNumMeshes = meshCount;
	scene->mMeshes = new aiMesh * [meshCount];

	uint32_t i = 0;
	for (const DDAMesh& ddaMesh : meshes)
	{
		const DDASubMesh& ddaSubMesh = ddaMesh.subMeshes[0];

		aiMesh* assimpMesh = new aiMesh();
		if (ddaSubMesh.materialIndex < materialCount)
			assimpMesh->mMaterialIndex = ddaSubMesh.materialIndex;
		else
			assimpMesh->mMaterialIndex = 0;

		const uint32_t subMeshVertexCount = static_cast<uint32_t>(ddaSubMesh.verticesPositions.size());
		const uint32_t subMeshTriangleCount = static_cast<uint32_t>(ddaSubMesh.verticesPositions.size() / 3);

		assimpMesh->mNumVertices = subMeshVertexCount;
		assimpMesh->mVertices = new aiVector3D[subMeshVertexCount];
		assimpMesh->mColors[0] = new aiColor4D[subMeshVertexCount];
		assimpMesh->mTextureCoords[0] = new aiVector3D[subMeshVertexCount];
		assimpMesh->mNumUVComponents[0] = 2;
		assimpMesh->mFaces = new aiFace[subMeshTriangleCount];

		for (size_t vertexIndex = 0; vertexIndex < subMeshVertexCount; vertexIndex++)
		{
			assimpMesh->mVertices[vertexIndex] = aiVector3D(
				ddaSubMesh.verticesPositions[vertexIndex].x,
				ddaSubMesh.verticesPositions[vertexIndex].y,
				ddaSubMesh.verticesPositions[vertexIndex].z
			);

			assimpMesh->mTextureCoords[0][vertexIndex] = aiVector3D(
				ddaSubMesh.verticesUVs[vertexIndex].x,
				ddaSubMesh.verticesUVs[vertexIndex].y,
				0
			);

			const DDAColor& color = ddaSubMesh.verticesColors[vertexIndex];
			assimpMesh->mColors[0][vertexIndex] = aiColor4D(
				color.r,
				color.g,
				color.b,
				color.a
			);

		}
		for (uint32_t triangleIndex = 0; triangleIndex < subMeshTriangleCount; triangleIndex++)
		{
			aiFace& face = assimpMesh->mFaces[assimpMesh->mNumFaces++];
			face.mNumIndices = 3;
			face.mIndices = new unsigned int[3];
			face.mIndices[0] = (triangleIndex * 3) + 0;
			face.mIndices[1] = (triangleIndex * 3) + 1;
			face.mIndices[2] = (triangleIndex * 3) + 2;
		}

		scene->mMeshes[i] = assimpMesh;
		aiNode* node = new aiNode();
		node->mName = aiString("Mesh_" + std::to_string(i));
		node->mNumMeshes = 1;
		node->mMeshes = new unsigned int[1] { (unsigned int)i };
		scene->mRootNode->mChildren[i] = node;

		i++;
	}

	scene->mNumMaterials = static_cast<uint32_t>(assimpMaterials.size());
	scene->mMaterials = new aiMaterial * [assimpMaterials.size()];
	for (unsigned int i = 0; i < assimpMaterials.size(); i++)
	{
		scene->mMaterials[i] = assimpMaterials[i];
	}

	Assimp::Exporter exporter;
	const aiReturn result = exporter.Export(scene, "fbx", exportFolder + "output.fbx", aiProcess_FlipUVs);
	//aiReturn result2 = exporter.Export(scene, "obj", "output.obj", aiProcess_FlipUVs);
}

void DDAManager::ExtractData(DDAGameFile gameFile, const std::string& exportFolder)
{
	std::cout << "Extracting: " << filesNames[(int)gameFile] << std::endl;
	DDAFileParser fileParser;
	TextureDumper textureDumper;

	const std::string filePath = m_gameFolderPath + filesNames[(int)gameFile];

	const std::string finalExportFolder = exportFolder + filesNames[(int)gameFile].substr(0, filesNames[(int)gameFile].find_last_of(".")) + "\\";

	const bool extractFolderExists = std::filesystem::exists(finalExportFolder);
	if (!extractFolderExists)
	{
		std::filesystem::create_directories(finalExportFolder);
	}

	const DDAExtractedData data = fileParser.LoadFile(filePath, gameFile, finalExportFolder);
	if(!extractFolderExists)
	{
		for (const DDATextureCopyParams& textureCopyParams: data.textureCopyParamsList)
		{
			textureDumper.DumpTexture(textureCopyParams, finalExportFolder);
		}
	}
	if (!data.meshes.empty())
	{
		CreateFXBMesh(data.meshes, data.textureTables, finalExportFolder);
	}
}