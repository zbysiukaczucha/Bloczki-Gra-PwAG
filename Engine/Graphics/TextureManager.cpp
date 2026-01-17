#define STB_IMAGE_IMPLEMENTATION
#include "TextureManager.h"

#include <array>
#include <iostream>
#include <unordered_set>

#include "../Utils/ThirdParty/stb_image.h"
#include "../World/BlockData.h"
#include "../World/BlockDatabase.h"

bool TextureManager::Initialize(Microsoft::WRL::ComPtr<ID3D11Device>		device,
								Microsoft::WRL::ComPtr<ID3D11DeviceContext> context,
								const std::filesystem::path&				baseAssetPath)
{
	device_		   = device;
	context_	   = context;
	baseAssetPath_ = baseAssetPath;

#ifdef _DEBUG
	// print current working dir
	std::filesystem::path currentPath = std::filesystem::current_path();
	std::cout << "\ncurrentPath: " << currentPath << std::endl;
#endif
	return PopulateTextureArrays();
}

ID3D11Texture2D* TextureManager::GetTextureArray(TextureType usage)
{
	switch (usage)
	{
		case TextureType::Albedo:
			return albedoArray_.Get();
		case TextureType::Normal:
			return normalArray_.Get();
		case TextureType::ARM:
			return ARMArray_.Get();
		case TextureType::Heightmap:
			return heightArray_.Get();
		case TextureType::Emissive:
			return emissiveArray_.Get();
	}

	return nullptr;
}

ID3D11ShaderResourceView* TextureManager::GetTextureSRV(TextureType usage)
{
	switch (usage)
	{
		case TextureType::Albedo:
			return albedoSRV_.Get();
		case TextureType::Normal:
			return normalSRV_.Get();
		case TextureType::ARM:
			return ARMSRV_.Get();
		case TextureType::Heightmap:
			return heightSRV_.Get();
		case TextureType::Emissive:
			return emissiveSRV_.Get();
	}

	return nullptr;
}

bool TextureManager::PopulateTextureArrays()
{
	namespace fs = std::filesystem;

	// Ensure the texture buffers are clear
	albedoRawTextures_.clear();
	normalRawTextures_.clear();
	ARMRawTextures_.clear();
	heightRawTextures_.clear();
	emissiveRawTextures_.clear();

	albedoPathToIndex_.clear();
	normalPathToIndex_.clear();
	ARMPathToIndex_.clear();
	heightPathToIndex_.clear();
	emissivePathToIndex_.clear();

	// Create and add the default textures
	albedoRawTextures_.push_back(GenerateDefaultAlbedo());
	normalRawTextures_.push_back(GenerateDefaultNormal());
	ARMRawTextures_.push_back(GenerateDefaultARM());
	heightRawTextures_.push_back(GenerateDefaultHeightmap());
	emissiveRawTextures_.push_back(GenerateDefaultEmissive());

	static constexpr auto default_albedo	= "DEFAULT_ALBEDO";
	static constexpr auto default_normal	= "DEFAULT_NORMAL";
	static constexpr auto default_ARM		= "DEFAULT_ARM";
	static constexpr auto default_heightmap = "DEFAULT_HEIGHTMAP";
	static constexpr auto default_emissive	= "DEFAULT_EMISSIVE";

	// clang-format off
	albedoPathToIndex_[default_albedo]	  = 0;
	normalPathToIndex_[default_normal]	  = 0;
	ARMPathToIndex_[default_ARM]		      = 0;
	heightPathToIndex_[default_heightmap]  = 0;
	emissivePathToIndex_[default_emissive] = 0;
	// clang-format on

	auto ResolvePath = [&](const std::string& blockName,
						   const std::string& suffix,
						   const std::string& faceInfo,
						   const fs::path&	  fallback) -> fs::path
	{
		// Trying specific path first
		fs::path specific = baseAssetPath_ / (blockName + "_" + faceInfo + "_" + suffix + ".png");
		if (fs::exists(specific))
		{
			return specific;
		}

		// Try generic side
		if (faceInfo == "north" || faceInfo == "south" || faceInfo == "west" || faceInfo == "east")
		{
			fs::path genericSide = baseAssetPath_ / (blockName + "_side_" + suffix + ".png");
			if (fs::exists(genericSide))
			{
				return genericSide;
			}
		}

		// Try generic
		fs::path generic = baseAssetPath_ / (blockName + "_" + suffix + ".png");
		if (fs::exists(generic))
		{
			return generic;
		}

		// If all else fails, return fallback (most likely a "missing texture" kind of texture
		return fallback;
	};

	std::array faceNames	 = {"north", "south", "west", "east", "top", "bottom"};
	auto&	   blockDatabase = BlockDatabase::GetDatabase();

	for (std::size_t idx = static_cast<std::size_t>(BlockType::Dirt);
		 idx < static_cast<std::size_t>(BlockType::MAX_BLOCKS_);
		 ++idx)
	{
		BlockType  blockType = static_cast<BlockType>(idx);
		BlockData* blockData = blockDatabase.GetMutableBlockData(blockType);
		assert(blockData != nullptr);

		for (std::uint8_t faceIdx = 0; faceIdx < 6; ++faceIdx)
		{
			fs::path albedoPath = ResolvePath(blockData->blockName, "basecolor", faceNames[faceIdx], default_albedo);

			fs::path normalPath = ResolvePath(blockData->blockName, "normal", faceNames[faceIdx], default_normal);

			fs::path ARMPath = ResolvePath(blockData->blockName, "ARM", faceNames[faceIdx], default_ARM);

			fs::path heightPath = ResolvePath(blockData->blockName, "heightmap", faceNames[faceIdx], default_heightmap);
			fs::path emissivePath = ResolvePath(blockData->blockName, "emissive", faceNames[faceIdx], default_emissive);

			std::size_t albedoIndex	   = GetOrLoadTexture(albedoPath, TextureType::Albedo);
			std::size_t normalIndex	   = GetOrLoadTexture(normalPath, TextureType::Normal);
			std::size_t ARMIndex	   = GetOrLoadTexture(ARMPath, TextureType::ARM);
			std::size_t heightmapIndex = GetOrLoadTexture(heightPath, TextureType::Heightmap);
			std::size_t emissiveIndex  = GetOrLoadTexture(emissivePath, TextureType::Emissive);

			std::string matKey = std::to_string(albedoIndex)
							   + "|"
							   + std::to_string(normalIndex)
							   + "|"
							   + std::to_string(ARMIndex)
							   + "|"
							   + std::to_string(heightmapIndex)
							   + "|"
							   + std::to_string(emissiveIndex);

			std::size_t matID = 0;
			if (materialCombinationCache_.contains(matKey))
			{
				// material with such specification already exists
				matID = materialCombinationCache_[matKey];
			}
			else
			{
				matID = materialCacheEntries_.size();
				materialCacheEntries_.emplace_back(static_cast<std::uint32_t>(albedoIndex),
												   static_cast<std::uint32_t>(normalIndex),
												   static_cast<std::uint32_t>(ARMIndex),
												   static_cast<std::uint32_t>(heightmapIndex),
												   static_cast<std::uint32_t>(emissiveIndex));

				materialCombinationCache_[matKey] = matID;
			}

			blockData->textureIndices[faceIdx] = matID;
		}
	}

	// now fill the buffers
	bool didInitSucceed = CreateTextureArray(albedoRawTextures_,
											 DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
											 albedoArray_,
											 albedoSRV_);
	if (didInitSucceed == false)
	{
		return false;
	}

	didInitSucceed = CreateTextureArray(normalRawTextures_, DXGI_FORMAT_R8G8B8A8_UNORM, normalArray_, normalSRV_);
	if (didInitSucceed == false)
	{
		return false;
	}

	didInitSucceed = CreateTextureArray(ARMRawTextures_, DXGI_FORMAT_R8G8B8A8_UNORM, ARMArray_, ARMSRV_);
	if (didInitSucceed == false)
	{
		return false;
	}

	didInitSucceed = CreateTextureArray(heightRawTextures_, DXGI_FORMAT_R8G8B8A8_UNORM, heightArray_, heightSRV_);
	if (didInitSucceed == false)
	{
		return false;
	}

	didInitSucceed = CreateTextureArray(emissiveRawTextures_,
										DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
										emissiveArray_,
										emissiveSRV_);
	if (didInitSucceed == false)
	{
		return false;
	}

	didInitSucceed = CreateMaterialBuffer(materialCacheEntries_);
	if (didInitSucceed == false)
	{
		return false;
	}
	return true;
}

std::size_t TextureManager::GetOrLoadTexture(const std::filesystem::path& texturePath, TextureType type)
{
	namespace fs = std::filesystem;

	// Quick lambda to make sure the stbi_load image is freed
	auto StbiLoadWrapper = [](const std::filesystem::path& path) -> RawTexture
	{
		int		   width, height, channels;
		const auto data = stbi_load(path.string().c_str(), &width, &height, &channels, TEXTURE_COLOR_CHANNELS);
		if (!data)
		{
			std::cerr << "Couldn't load texture from file: " << path << "\n" << "Reason: " << stbi_failure_reason();

			return RawTexture{};
		}

		if (width != TEXTURE_RESOLUTION || height != TEXTURE_RESOLUTION)
		{
			std::cerr << "Texture " << path << " does not meet the requirements\n" << "Reason: ";
			if (width != TEXTURE_RESOLUTION || height != TEXTURE_RESOLUTION)
			{
				std::cerr << " Invalid texture resolution";
			}
			std::cerr << std::endl;
			return RawTexture{};
		}

		std::vector<uint8_t> textureData(data, data + width * height * 4);
		stbi_image_free(data);

		return {std::move(textureData), width, height, channels};
	};

	// check if the texture even exists as a file
	if (fs::exists(texturePath) == false)
	{
		return DEFAULT_TEXTURE_INDEX; // returns the default texture
	}


	std::vector<RawTexture>*				   rawTextureBuffer = nullptr;
	std::unordered_map<fs::path, std::size_t>* pathToTextureMap = nullptr;

	switch (type)
	{
		case TextureType::Albedo:
			rawTextureBuffer = &albedoRawTextures_;
			pathToTextureMap = &albedoPathToIndex_;
			break;
		case TextureType::Normal:
			rawTextureBuffer = &normalRawTextures_;
			pathToTextureMap = &normalPathToIndex_;
			break;
		case TextureType::ARM:
			rawTextureBuffer = &ARMRawTextures_;
			pathToTextureMap = &ARMPathToIndex_;
			break;
		case TextureType::Heightmap:
			rawTextureBuffer = &heightRawTextures_;
			pathToTextureMap = &heightPathToIndex_;
			break;
		case TextureType::Emissive:
			rawTextureBuffer = &emissiveRawTextures_;
			pathToTextureMap = &emissivePathToIndex_;
			break;

		default:
			assert(false); // shouldn't ever happen
	}


	// check if the texture is already loaded
	if (pathToTextureMap->contains(texturePath))
	{
		return pathToTextureMap->at(texturePath);
	}

	// Add new texture since it's not been loaded yet
	RawTexture texture = StbiLoadWrapper(texturePath);

	// If the texture failed to load for some reason/is invalid, return the index of the default texture
	// Reasons will be printed in the console
	if (texture.width == -1 || texture.height == -1 || texture.channels == -1 || texture.data.empty())
	{
		return DEFAULT_TEXTURE_INDEX;
	}

	std::size_t textureIndex = rawTextureBuffer->size();
	rawTextureBuffer->push_back(std::move(texture));
	pathToTextureMap->emplace(texturePath, textureIndex);

	return textureIndex;
}

bool TextureManager::CreateTextureArray(std::span<const RawTexture>						  textureData,
										DXGI_FORMAT										  format,
										Microsoft::WRL::ComPtr<ID3D11Texture2D>&		  outTextureArray,
										Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& outTextureSRV)
{
	D3D11_TEXTURE2D_DESC desc = {};
	desc.Width				  = TEXTURE_RESOLUTION;
	desc.Height				  = TEXTURE_RESOLUTION;
	desc.MipLevels			  = 0;
	desc.ArraySize			  = textureData.size();
	desc.Format				  = format;
	desc.SampleDesc.Count	  = 1;
	desc.SampleDesc.Quality	  = 0;
	desc.Usage				  = D3D11_USAGE_DEFAULT;
	desc.BindFlags			  = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
	desc.CPUAccessFlags		  = 0;
	desc.MiscFlags			  = D3D11_RESOURCE_MISC_GENERATE_MIPS;

	HRESULT result = device_->CreateTexture2D(&desc, nullptr, &outTextureArray);
	if (FAILED(result))
	{
		return false;
	}

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format							= desc.Format;
	srvDesc.ViewDimension					= D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
	srvDesc.Texture2DArray.MostDetailedMip	= 0;
	srvDesc.Texture2DArray.MipLevels		= -1;
	srvDesc.Texture2DArray.FirstArraySlice	= 0;
	srvDesc.Texture2DArray.ArraySize		= desc.ArraySize;

	result = device_->CreateShaderResourceView(outTextureArray.Get(), &srvDesc, &outTextureSRV);
	if (FAILED(result))
	{
		return false;
	}


	// Check how many mips D3D is actually creating
	outTextureArray->GetDesc(&desc);
	const UINT mipsPerSlice = desc.MipLevels;

	const UINT rowPitch = TEXTURE_COLOR_CHANNELS * TEXTURE_RESOLUTION;
	for (std::size_t textureIdx = 0; textureIdx < textureData.size(); textureIdx++)
	{
		UINT subresourceIdx = D3D11CalcSubresource(0, textureIdx, mipsPerSlice);

		context_->UpdateSubresource(outTextureArray.Get(),
									subresourceIdx,
									nullptr,
									textureData[textureIdx].data.data(),
									rowPitch,
									0);
	}

	context_->GenerateMips(outTextureSRV.Get());

	return true;
}

bool TextureManager::CreateMaterialBuffer(std::span<const MaterialCacheEntry> materialList)
{
	D3D11_BUFFER_DESC desc	 = {};
	desc.Usage				 = D3D11_USAGE_IMMUTABLE;
	desc.ByteWidth			 = static_cast<UINT>(sizeof(MaterialCacheEntry) * materialList.size());
	desc.BindFlags			 = D3D11_BIND_SHADER_RESOURCE;
	desc.CPUAccessFlags		 = 0;
	desc.MiscFlags			 = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	desc.StructureByteStride = sizeof(MaterialCacheEntry);

	D3D11_SUBRESOURCE_DATA initData = {};
	initData.pSysMem				= materialList.data();

	HRESULT result = device_->CreateBuffer(&desc, &initData, &materialBuffer_);
	if (ERROR(result))
	{
		return false;
	}

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format							= DXGI_FORMAT_UNKNOWN;
	srvDesc.ViewDimension					= D3D11_SRV_DIMENSION_BUFFER;
	srvDesc.Buffer.FirstElement				= 0;
	srvDesc.Buffer.NumElements				= static_cast<UINT>(materialList.size());

	result = device_->CreateShaderResourceView(materialBuffer_.Get(), &srvDesc, &materialBufferSRV_);
	if (ERROR(result))
	{
		return false;
	}

	return true;
}

TextureManager::RawTexture TextureManager::GenerateDefaultTexture(const TextureType type)
{
	switch (type)
	{
		case TextureType::Albedo:
			return GenerateDefaultAlbedo();
		case TextureType::Normal:
			return GenerateDefaultNormal();
		case TextureType::ARM:
			return GenerateDefaultARM();
		case TextureType::Heightmap:
			return GenerateDefaultHeightmap();
		case TextureType::Emissive:
			return GenerateDefaultEmissive();
	}

	// this will never happen but clang doesn't understand this for some reason
	return {};
}

TextureManager::RawTexture TextureManager::GenerateDefaultAlbedo()
{
	std::vector<uint8_t> data(TEXTURE_COLOR_CHANNELS * TEXTURE_RESOLUTION * TEXTURE_RESOLUTION);
	for (std::size_t idx = 0; idx < data.size(); idx += 4)
	{
		std::size_t pixelIdx = idx / TEXTURE_COLOR_CHANNELS;
		std::size_t x		 = pixelIdx % TEXTURE_RESOLUTION;
		std::size_t y		 = pixelIdx / TEXTURE_RESOLUTION;

		// Checkerboard logic

		const std::size_t denominator = 2;

		bool isBlack = ((x / (TEXTURE_RESOLUTION / denominator)) + (y / (TEXTURE_RESOLUTION / denominator))) % 2 == 0;

		if (isBlack)
		{
			data[idx]	  = 0;
			data[idx + 1] = 0;
			data[idx + 2] = 0;
			data[idx + 3] = 255; // alpha always max
		}
		else
		{
			// Magenta
			data[idx]	  = 255;
			data[idx + 1] = 0;
			data[idx + 2] = 255;
			data[idx + 3] = 255;
		}
	}

	return {.data	  = std::move(data),
			.width	  = TEXTURE_RESOLUTION,
			.height	  = TEXTURE_RESOLUTION,
			.channels = TEXTURE_COLOR_CHANNELS};
}

TextureManager::RawTexture TextureManager::GenerateDefaultNormal()
{
	std::vector<uint8_t> data(TEXTURE_COLOR_CHANNELS * TEXTURE_RESOLUTION * TEXTURE_RESOLUTION);
	for (std::size_t idx = 0; idx < data.size(); idx += 4)
	{
		// flat normal map
		data[idx]	  = 128;
		data[idx + 1] = 128;
		data[idx + 2] = 255;
		data[idx + 3] = 255;
	}

	return {.data	  = std::move(data),
			.width	  = TEXTURE_RESOLUTION,
			.height	  = TEXTURE_RESOLUTION,
			.channels = TEXTURE_COLOR_CHANNELS};
}

TextureManager::RawTexture TextureManager::GenerateDefaultARM()
{
	std::vector<uint8_t> data(TEXTURE_COLOR_CHANNELS * TEXTURE_RESOLUTION * TEXTURE_RESOLUTION);
	for (std::size_t idx = 0; idx < data.size(); idx += 4)
	{
		data[idx]	  = 255; // No AO
		data[idx + 1] = 255; // Rough
		data[idx + 2] = 0;	 // Non-metallic
		data[idx + 3] = 255; // for now alpha is not really used
	}

	return {.data	  = std::move(data),
			.width	  = TEXTURE_RESOLUTION,
			.height	  = TEXTURE_RESOLUTION,
			.channels = TEXTURE_COLOR_CHANNELS};
}

TextureManager::RawTexture TextureManager::GenerateDefaultHeightmap()
{
	std::vector<uint8_t> data(TEXTURE_COLOR_CHANNELS * TEXTURE_RESOLUTION * TEXTURE_RESOLUTION);
	std::ranges::fill(data, 255);

	return {.data	  = std::move(data),
			.width	  = TEXTURE_RESOLUTION,
			.height	  = TEXTURE_RESOLUTION,
			.channels = TEXTURE_COLOR_CHANNELS};
}
TextureManager::RawTexture TextureManager::GenerateDefaultEmissive()
{
	std::vector<uint8_t> data(TEXTURE_COLOR_CHANNELS * TEXTURE_RESOLUTION * TEXTURE_RESOLUTION);
	for (std::size_t idx = 0; idx < data.size(); idx += 4)
	{
		// default: non-emissive
		data[idx]	  = 0;
		data[idx + 1] = 0;
		data[idx + 2] = 0;
		data[idx + 3] = 255;
	}

	return {.data	  = std::move(data),
			.width	  = TEXTURE_RESOLUTION,
			.height	  = TEXTURE_RESOLUTION,
			.channels = TEXTURE_COLOR_CHANNELS};
}
