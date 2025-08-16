// SPDX-License-Identifier: MIT
//
// Copyright (c) 2025-2025 Gregory Machefer (Fewnity)
//
// This file is part of DDA Extractor.

#pragma once

#include <string>

#include "dda_structures.h"

class TextureDumper
{
public:
	void DumpTexture(const DDATextureCopyParams& textureCopyParams, const std::string& destinationFolder);
	void CopyTextureData(const DDATextureCopyParams& params);

private:
};

