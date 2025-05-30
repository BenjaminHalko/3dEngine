#include "Precompiled.h"
#include "MeshBuffer.h"
#include "GraphicsSystem.h"

using namespace Engine;
using namespace Engine::Graphics;

void MeshBuffer::Initialize(const void* vertices, uint32_t vertexSize, uint32_t vertexCount)
{
	CreateVertexBuffer(vertices, vertexSize, vertexCount);
}

void MeshBuffer::Initialize(const void* vertices, uint32_t vertexSize, uint32_t vertexCount, const void* indices, uint32_t indexCount)
{
	CreateVertexBuffer(vertices, vertexSize, vertexCount);
	CreateIndexBuffer(indices, indexCount);
}

void MeshBuffer::Terminate()
{
	SafeRelease(mVertexBuffer);
}

void MeshBuffer::SetTopology(Topology topology)
{
	switch (topology)
	{
	case Topology::Points:      mTopology = D3D11_PRIMITIVE_TOPOLOGY_POINTLIST; break;
	case Topology::Lines:       mTopology = D3D11_PRIMITIVE_TOPOLOGY_LINELIST; break;
	case Topology::Triangles:   mTopology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST; break;
	default:
		break;
	}
}

void MeshBuffer::Update(const void* vertices, uint32_t vertexCount)
{
	mVertexCount = vertexCount;
	auto context = GraphicsSystem::Get()->GetContext();

	D3D11_MAPPED_SUBRESOURCE resource;
	context->Map(mVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &resource);
	memcpy(resource.pData, vertices, (mVertexSize * vertexCount));
	context->Unmap(mVertexBuffer, 0);
}

void MeshBuffer::Render() const
{
	auto context = GraphicsSystem::Get()->GetContext();

	context->IASetPrimitiveTopology(mTopology);
	UINT offset = 0;
	context->IASetVertexBuffers(0, 1, &mVertexBuffer, &mVertexSize, &offset);

	if (mIndexBuffer != nullptr)
	{
		context->IASetIndexBuffer(mIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
		context->DrawIndexed((UINT)mIndexCount, 0, 0);
	}
	else
	{
		context->Draw(static_cast<UINT>(mVertexCount), 0);
	}
}

void MeshBuffer::CreateVertexBuffer(const void* vertices, uint32_t vertexSize, uint32_t vertexCount)
{
	mVertexSize = vertexSize;
	mVertexCount = vertexCount;

	auto device = GraphicsSystem::Get()->GetDevice();

	const bool isDynamic = (vertices == nullptr);
	// Need to create a buffer to store the vertices
	// STORES DATA FOR THE OBJECT
	D3D11_BUFFER_DESC bufferDesc{};
	bufferDesc.ByteWidth = vertexSize * vertexCount;
	bufferDesc.Usage = (isDynamic)? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT;
	bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bufferDesc.MiscFlags = 0;
	bufferDesc.StructureByteStride = 0;
	bufferDesc.CPUAccessFlags = (isDynamic) ? D3D11_CPU_ACCESS_WRITE : 0;

	D3D11_SUBRESOURCE_DATA initData = {};
	initData.pSysMem = vertices;

	HRESULT hr = device->CreateBuffer(&bufferDesc, (isDynamic? nullptr : &initData), &mVertexBuffer);
	ASSERT(SUCCEEDED(hr), "Failed to create vertex buffer");
}

void Engine::Graphics::MeshBuffer::CreateIndexBuffer(const void* indices, uint32_t indexCount)
{
	if (indexCount == 0)
	{
		return;
	}

	mIndexCount = indexCount;

	auto device = GraphicsSystem::Get()->GetDevice();

	// Index Buffer
	D3D11_BUFFER_DESC bufferDesc{};
	bufferDesc.ByteWidth = static_cast<UINT>(indexCount) * sizeof(uint32_t);
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	bufferDesc.MiscFlags = 0;
	bufferDesc.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA initData = {};
	initData.pSysMem = indices;

	HRESULT hr = device->CreateBuffer(&bufferDesc, &initData, &mIndexBuffer);
	ASSERT(SUCCEEDED(hr), "Failed to create Index Buffer");

}
