#include "shaders.h"

#include "render.h"
#include "am/types.h"
#include "am/system/asserts.h"
#include "am/system/datamgr.h"

#include <d3dcompiler.h>

ID3D11Buffer* rageam::graphics::CreateCB(size_t size)
{
	size = ALIGN_16(size);

	D3D11_BUFFER_DESC desc;
	desc.ByteWidth = size;
	desc.Usage = D3D11_USAGE_DYNAMIC;
	desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	desc.MiscFlags = 0;
	desc.StructureByteStride = size;

	ID3D11Buffer* buffer;
	AM_ASSERT_STATUS(graphics::RenderGetDevice()->CreateBuffer(&desc, NULL, &buffer));
	return buffer;
}

void rageam::graphics::CreateShaders(
	ID3D11VertexShader** vs, ID3D11PixelShader** ps, ID3DBlob** vsBlob,
	ConstWString vsFileName, ConstWString psFileName)
{
	ID3D11Device* device = graphics::RenderGetDevice();

	file::WPath shaders = DataManager::GetDataFolder() / L"shaders";
	file::WPath vsPath = shaders / vsFileName;
	file::WPath psPath = shaders / psFileName;

	// VS
	{
		ID3DBlob* blob;
		ID3DBlob* errors;
		AM_ASSERT_STATUS(D3DCompileFromFile(vsPath, NULL, NULL, "main", "vs_4_0", 0, 0, &blob, &errors));
		AM_ASSERT_STATUS(device->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), NULL, vs));
		SAFE_RELEASE(errors);
		*vsBlob = blob;
	}

	// PS
	{
		ID3DBlob* blob;
		ID3DBlob* errors;
		AM_ASSERT_STATUS(D3DCompileFromFile(psPath, NULL, NULL, "main", "ps_4_0", 0, 0, &blob, &errors));
		AM_ASSERT_STATUS(device->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), NULL, ps));
		SAFE_RELEASE(errors);
		SAFE_RELEASE(blob);
	}
}

ID3D11Buffer* rageam::graphics::CreateBuffer(size_t elementSize, size_t elementCount, bool vertexBuffer)
{
	D3D11_BUFFER_DESC desc;
	desc.BindFlags = vertexBuffer ? D3D11_BIND_VERTEX_BUFFER : D3D11_BIND_INDEX_BUFFER;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	desc.Usage = D3D11_USAGE_DYNAMIC;
	desc.ByteWidth = elementSize * elementCount;
	desc.StructureByteStride = elementSize;
	desc.MiscFlags = 0;

	ID3D11Buffer* object;
	AM_ASSERT_STATUS(graphics::RenderGetDevice()->CreateBuffer(&desc, NULL, &object));
	return object;
}

void rageam::graphics::UploadBuffer(const amComPtr<ID3D11Buffer>& buffer, pConstVoid data, u32 dataSize)
{
	ID3D11DeviceContext* context = RenderGetContext();
	D3D11_MAPPED_SUBRESOURCE mapped;
	AM_ASSERT_STATUS(context->Map(buffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped));
	memcpy(mapped.pData, data, dataSize);
	context->Unmap(buffer.Get(), 0);
}

void rageam::graphics::Shader1::Create()
{
	ID3D11Device* device = graphics::RenderGetDevice();

	amComPtr<ID3DBlob> vsBlob;
	CreateShaders(&m_VS, &m_PS, &vsBlob, m_VsName, m_PsName);

	D3D11_INPUT_ELEMENT_DESC inputDesc[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 28, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	AM_ASSERT_STATUS(device->CreateInputLayout(inputDesc, 3, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &m_VSLayout));
}

rageam::graphics::Shader1::Shader1(ConstWString vsName, ConstWString psName)
{
	m_VsName = vsName;
	m_PsName = psName;
}

void rageam::graphics::Shader1::Bind()
{
	ID3D11DeviceContext* context = graphics::RenderGetContext();
	context->IASetInputLayout(m_VSLayout.Get());
	context->VSSetShader(m_VS.Get(), NULL, 0);
	context->PSSetShader(m_PS.Get(), NULL, 0);
}

void rageam::graphics::DefaultShader::Create()
{
	Shader1::Create();

	m_LocalsCB = amComPtr<ID3D11Buffer>(CreateCB(sizeof Locals));
}

void rageam::graphics::DefaultShader::Bind()
{
	Shader1::Bind();

	ID3D11DeviceContext* context = graphics::RenderGetContext();

	UploadBuffer(m_LocalsCB, &Locals, sizeof Locals);
	ID3D11Buffer* cb = m_LocalsCB.Get();
	context->VSSetConstantBuffers(2, 1, &cb);
}

void rageam::graphics::ImageBlitShader::Create()
{
	Shader1::Create();

	Vec3S screenVerts[] =
	{
		Vec3S(-1, -1, 0), // Bottom Left
		Vec3S( 1,  1, 0), // Top Right
		Vec3S(-1,  1, 0), // Top Left
		Vec3S( 1, -1, 0), // Bottom Right
		Vec3S( 1,  1, 0), // Top Right
		Vec3S(-1, -1, 0), // Bottom Left
	};

	D3D11_BUFFER_DESC bufferDesc;
	bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bufferDesc.CPUAccessFlags = 0;
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth = sizeof screenVerts;
	bufferDesc.StructureByteStride = sizeof Vec3S;
	bufferDesc.MiscFlags = 0;

	D3D11_SUBRESOURCE_DATA bufferData;
	bufferData.SysMemPitch = 0;
	bufferData.SysMemSlicePitch = 0;
	bufferData.pSysMem = screenVerts;

	AM_ASSERT_STATUS(graphics::RenderGetDevice()->CreateBuffer(&bufferDesc, &bufferData, &m_ScreenVerts));
}

void rageam::graphics::ImageBlitShader::Blit(ID3D11ShaderResourceView* view) const
{
	ID3D11DeviceContext* context = graphics::RenderGetContext();

	// Map texture
	ID3D11ShaderResourceView* views = view;
	context->PSSetShaderResources(0, 1, &views);

	// Blit texture on screen
	ID3D11Buffer* vb = m_ScreenVerts.Get();
	u32           stride = sizeof Vec3S;
	u32           offset = 0;
	context->IASetVertexBuffers(0, 1, &vb, &stride, &offset);
	context->Draw(6, 0);
}
