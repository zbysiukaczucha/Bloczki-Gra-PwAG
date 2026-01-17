#pragma once
#include <cstring>
#include <d3d11.h>
#include <iostream>
#include <ostream>
#include <wrl/client.h>

#include "ResourceBindType.h"

template <typename T>
class CBuffer
{
public:
	CBuffer()						   = default;
	CBuffer(const CBuffer&)			   = delete;
	CBuffer& operator=(const CBuffer&) = delete;
	CBuffer(CBuffer&&)				   = delete;
	CBuffer& operator=(CBuffer&&)	   = delete;
	~CBuffer()						   = default;

	bool Create(ID3D11Device* device);

	// If custom description needed
	bool Create(ID3D11Device* device, const D3D11_BUFFER_DESC& desc);
	bool Update(ID3D11DeviceContext* context, const T& data, D3D11_MAP mapType = D3D11_MAP_WRITE_DISCARD);
	void Bind(ID3D11DeviceContext* context, UINT slot, std::uint8_t bindTarget);

private:
	Microsoft::WRL::ComPtr<ID3D11Buffer> buffer_;
};

template <typename T>
bool CBuffer<T>::Create(ID3D11Device* device)
{
	D3D11_BUFFER_DESC desc{};

	desc.Usage				 = D3D11_USAGE_DYNAMIC;
	desc.ByteWidth			 = (static_cast<UINT>(sizeof(T) + 15) / 16) * 16; // round to nearest multiple of 16
	desc.BindFlags			 = D3D11_BIND_CONSTANT_BUFFER;
	desc.CPUAccessFlags		 = D3D11_CPU_ACCESS_WRITE;
	desc.MiscFlags			 = 0;
	desc.StructureByteStride = 0;

	HRESULT result = device->CreateBuffer(&desc, nullptr, &buffer_);
	if (FAILED(result))
	{
		std::cerr << "CBuffer<T>::Create(): Failed to create buffer." << std::endl;
		return false;
	}

	return true;
}

template <typename T>
bool CBuffer<T>::Create(ID3D11Device* device, const D3D11_BUFFER_DESC& desc)
{
	HRESULT result = device->CreateBuffer(&desc, nullptr, &buffer_);
	if (FAILED(result))
	{
		std::cerr << "CBuffer<T>::Create(): Failed to create buffer." << std::endl;
		return false;
	}

	return true;
}

template <typename T>
bool CBuffer<T>::Update(ID3D11DeviceContext* context, const T& data, D3D11_MAP mapType)
{
	D3D11_MAPPED_SUBRESOURCE mappedResource;

	HRESULT result = context->Map(buffer_.Get(), 0, mapType, 0, &mappedResource);
	if (FAILED(result))
	{
		std::cerr << "CBuffer<T>::Update(): Failed to map buffer." << std::endl;
		return false;
	}

	std::memcpy(mappedResource.pData, &data, sizeof(T));

	context->Unmap(buffer_.Get(), 0);
	return true;
}

template <typename T>
void CBuffer<T>::Bind(ID3D11DeviceContext* context, UINT slot, std::uint8_t bindTarget)
{
	if (bindTarget & BindTarget::VertexShader)
	{
		context->VSSetConstantBuffers(slot, 1, buffer_.GetAddressOf());
	}
	if (bindTarget & BindTarget::PixelShader)
	{
		context->PSSetConstantBuffers(slot, 1, buffer_.GetAddressOf());
	}
}
