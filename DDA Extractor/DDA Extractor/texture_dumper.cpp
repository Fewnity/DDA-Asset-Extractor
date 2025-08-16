// SPDX-License-Identifier: MIT
//
// Copyright (c) 2025-2025 Gregory Machefer (Fewnity)
//
// This file is part of DDA Extractor.

#include "texture_dumper.h"

#include <filesystem>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

/**
* @brief Copy raw texture using palette to a buffer
*/
void TextureDumper::CopyTextureData(const DDATextureCopyParams& params)
{
	for (size_t y = 0; y < params.outputHeight; y++)
	{
		for (size_t x = 0; x < params.outputWidth; x++)
		{
			const size_t inputPixelIndex = (x + params.xOffset) + y * params.inputWidth;
			const size_t outputPixelIndex = x + y * params.outputWidth;
			// There are two pixels per byte
			const uint8_t colorId = *(params.inputTextureData + inputPixelIndex);
			if (params.clutType == DDAClutType::CLUT_256)
			{
				// On PS2 the alpha is between 0 and 127, so we multiply by 2
				params.outputTextureData[outputPixelIndex * 4] = params.palette[colorId * 4 + 0];
				params.outputTextureData[outputPixelIndex * 4 + 1] = params.palette[colorId * 4 + 1];
				params.outputTextureData[outputPixelIndex * 4 + 2] = params.palette[colorId * 4 + 2];
				params.outputTextureData[outputPixelIndex * 4 + 3] = std::clamp(params.palette[colorId * 4 + 3] * 2, 0, 0xff);
			}
			else if (params.clutType == DDAClutType::CLUT_16)
			{
				const uint8_t firstColorId = colorId & 0x0F;
				const uint8_t secondColorId = colorId >> 4;

				// On PS2 the alpha is between 0 and 127, so we multiply by 2
				params.outputTextureData[outputPixelIndex * 8 + 0] = params.palette[firstColorId * 4 + 0];
				params.outputTextureData[outputPixelIndex * 8 + 1] = params.palette[firstColorId * 4 + 1];
				params.outputTextureData[outputPixelIndex * 8 + 2] = params.palette[firstColorId * 4 + 2];
				params.outputTextureData[outputPixelIndex * 8 + 3] = std::clamp(params.palette[firstColorId * 4 + 3] * 2, 0, 0xff);

				params.outputTextureData[outputPixelIndex * 8 + 4] = params.palette[secondColorId * 4 + 0];
				params.outputTextureData[outputPixelIndex * 8 + 5] = params.palette[secondColorId * 4 + 1];
				params.outputTextureData[outputPixelIndex * 8 + 6] = params.palette[secondColorId * 4 + 2];
				params.outputTextureData[outputPixelIndex * 8 + 7] = std::clamp(params.palette[secondColorId * 4 + 3] * 2, 0, 0xff);
			}
		}
	}
}

/**
* @brief Dump all textures from the texture table to PNG files
*/
void TextureDumper::DumpTexture(const DDATextureCopyParams& textureCopyParams, const std::string& destinationFolder)
{
	if (destinationFolder.empty())
	{
		return;
	}

	if (textureCopyParams.clutType != DDAClutType::CLUT_NONE)
	{
		CopyTextureData(textureCopyParams);
	}

	std::string path = destinationFolder + textureCopyParams.textureName + ".png";
	size_t fileNum = 0;
	while (std::filesystem::exists(path))
	{
		fileNum++;
		path = destinationFolder + textureCopyParams.textureName + " (" + std::to_string(fileNum) + ").png";
	}

	stbi_write_png(path.c_str(), static_cast<int>(textureCopyParams.exportWidth), static_cast<int>(textureCopyParams.exportHeight), 4, textureCopyParams.outputTextureData.get(), 0);
}