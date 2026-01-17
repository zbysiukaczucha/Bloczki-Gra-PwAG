#pragma once
#include <d3d11.h>
#include <filesystem>
#include <span>
#include <unordered_map>
#include <wrl/client.h>

#include "../World/BlockType.h"

struct MaterialCacheEntry
{
	std::uint32_t albedoIndex;
	std::uint32_t normalIndex;
	std::uint32_t ARMIndex;
	std::uint32_t heightmapIndex;
	std::uint32_t emissiveIndex;
};

enum class TextureType
{
	Albedo,
	Normal,
	ARM,
	Heightmap,
	Emissive,
};

class TextureManager
{
public:
	static constexpr std::size_t  TEXTURE_RESOLUTION	 = 32;
	static constexpr std::uint8_t TEXTURE_COLOR_CHANNELS = 4;
	static constexpr std::size_t  DEFAULT_TEXTURE_INDEX	 = 0;

private:
	struct RawTexture
	{
		std::vector<std::uint8_t> data;
		int						  width	   = -1;
		int						  height   = -1;
		int						  channels = -1;
	};

public:
	TextureManager() = default;

	TextureManager(const TextureManager&)			 = delete;
	TextureManager(TextureManager&&)				 = delete;
	TextureManager& operator=(const TextureManager&) = delete;
	TextureManager& operator=(TextureManager&&)		 = delete;

	bool Initialize(Microsoft::WRL::ComPtr<ID3D11Device>		device,
					Microsoft::WRL::ComPtr<ID3D11DeviceContext> context,
					const std::filesystem::path&				baseAssetPath);

	ID3D11Texture2D*		  GetTextureArray(TextureType usage);
	ID3D11ShaderResourceView* GetTextureSRV(TextureType usage);

private:
	bool PopulateTextureArrays();

	/**
	 *
	 * @param texturePath the path to the texture
	 * @param type the type of the texture (currently supported: Albedo, Normal and ARM)
	 * @return the index of the texture in the raw textures vector (up to the programmer to determine which one that is,
	 * based on the "type" paramter provided)
	 */
	std::size_t GetOrLoadTexture(const std::filesystem::path& texturePath, TextureType type);

	bool CreateTextureArray(std::span<const RawTexture>						  textureData,
							DXGI_FORMAT										  format,
							Microsoft::WRL::ComPtr<ID3D11Texture2D>&		  outTextureArray,
							Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& outTextureSRV);

	bool CreateMaterialBuffer(std::span<const MaterialCacheEntry> materialList);

	static RawTexture GenerateDefaultTexture(TextureType type);
	static RawTexture GenerateDefaultAlbedo();
	static RawTexture GenerateDefaultNormal();
	static RawTexture GenerateDefaultARM();
	static RawTexture GenerateDefaultHeightmap();
	static RawTexture GenerateDefaultEmissive();

	Microsoft::WRL::ComPtr<ID3D11Device>		device_;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> context_;

	Microsoft::WRL::ComPtr<ID3D11Texture2D> albedoArray_;
	Microsoft::WRL::ComPtr<ID3D11Texture2D> normalArray_;
	Microsoft::WRL::ComPtr<ID3D11Texture2D> ARMArray_;
	Microsoft::WRL::ComPtr<ID3D11Texture2D> heightArray_;
	Microsoft::WRL::ComPtr<ID3D11Texture2D> emissiveArray_;

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> albedoSRV_;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> normalSRV_;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> ARMSRV_;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> heightSRV_;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> emissiveSRV_;

	Microsoft::WRL::ComPtr<ID3D11Buffer>			 materialBuffer_;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> materialBufferSRV_;

	std::vector<RawTexture> albedoRawTextures_;
	std::vector<RawTexture> normalRawTextures_;
	std::vector<RawTexture> ARMRawTextures_;
	std::vector<RawTexture> heightRawTextures_;
	std::vector<RawTexture> emissiveRawTextures_;

	std::unordered_map<std::filesystem::path, std::size_t> albedoPathToIndex_;
	std::unordered_map<std::filesystem::path, std::size_t> normalPathToIndex_;
	std::unordered_map<std::filesystem::path, std::size_t> ARMPathToIndex_;
	std::unordered_map<std::filesystem::path, std::size_t> heightPathToIndex_;
	std::unordered_map<std::filesystem::path, std::size_t> emissivePathToIndex_;

	// for debug lookup
	std::vector<MaterialCacheEntry>				 materialCacheEntries_;
	std::unordered_map<std::string, std::size_t> materialCombinationCache_;

	std::filesystem::path					   baseAssetPath_;
	std::unordered_map<BlockType, std::size_t> textureZs;

public:
	ID3D11ShaderResourceView* GetMaterialBufferSRV() const { return materialBufferSRV_.Get(); }
};
