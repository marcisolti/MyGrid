#include "pch.h"
#include "GridRenderer.h"

#include "..\Common\DirectXHelper.h"

using namespace MyGrid;

GridRenderer::GridRenderer(const std::shared_ptr<DX::DeviceResources>& deviceResources) :
	m_loadingComplete(false),
	m_gridOrientation(DirectX::XM_PIDIV4),
	m_revolutionsPerMinute(15.0),
	m_totalSegmentCount(200),
	m_gridSize(20.f),
	m_deviceResources(deviceResources)
{

	assert(m_totalSegmentCount % 20 == 0 && m_totalSegmentCount > 0);
	CreateDeviceDependentResources();
	CreateWindowSizeDependentResources();

}

void GridRenderer::CreateDeviceDependentResources()
{

	auto loadVSTask = DX::ReadDataAsync(L"GridVertexShader.cso");
	auto loadGSTask = DX::ReadDataAsync(L"GridGeometryShader.cso");
	auto loadPSTask = DX::ReadDataAsync(L"GridPixelShader.cso");

	auto createVSTask = loadVSTask.then([this](const std::vector<byte>& fileData) {

		DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreateVertexShader(
			&fileData[0],
			fileData.size(),
			nullptr,
			m_vertexShader.GetAddressOf()));

		D3D11_INPUT_ELEMENT_DESC vertexDesc;
		ZeroMemory(&vertexDesc, sizeof(vertexDesc));
		vertexDesc.SemanticName = "POSITION";
		vertexDesc.SemanticIndex = 0;
		vertexDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		vertexDesc.InputSlot = 0;
		vertexDesc.AlignedByteOffset = 0;
		vertexDesc.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
		vertexDesc.InstanceDataStepRate = 0;

		DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreateInputLayout(
			&vertexDesc,
			1,
			&fileData[0],
			fileData.size(),
			m_inputLayout.GetAddressOf()));

	});

	auto createGSTask = loadGSTask.then([this](const std::vector<byte>& fileData) {

		DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreateGeometryShader(
			&fileData[0],
			fileData.size(),
			nullptr,
			m_geometryShader.GetAddressOf()));

	});

	auto createPSTask = loadPSTask.then([this](const std::vector<byte>& fileData) {

		DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreatePixelShader(
			&fileData[0],
			fileData.size(),
			nullptr,
			m_pixelShader.GetAddressOf()));

	});

	auto createVertexBufferTask = (createPSTask && createVSTask && createGSTask).then([this]() { 
		
		CreateVertexBuffer(); 
	
	});
	
	auto createRendererResourcesTask = createVertexBufferTask.then([this]() {

		D3D11_BLEND_DESC blendStateDesc;
		ZeroMemory(&blendStateDesc, sizeof(blendStateDesc));
		blendStateDesc.AlphaToCoverageEnable = false;
		blendStateDesc.IndependentBlendEnable = false;
		blendStateDesc.RenderTarget[0].BlendEnable = true;
		blendStateDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
		blendStateDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
		blendStateDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
		blendStateDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
		blendStateDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_SRC_ALPHA;
		blendStateDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
		blendStateDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

		DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreateBlendState(&blendStateDesc, m_blendState.GetAddressOf()));

		D3D11_BUFFER_DESC constantBufferDesc;
		ZeroMemory(&constantBufferDesc, sizeof(constantBufferDesc));
		constantBufferDesc.ByteWidth = sizeof(ConstantBufferData);
		constantBufferDesc.BindFlags= D3D11_BIND_CONSTANT_BUFFER;
		constantBufferDesc.Usage = D3D11_USAGE_DEFAULT;
		constantBufferDesc.CPUAccessFlags = 0;
		constantBufferDesc.MiscFlags = 0;
		constantBufferDesc.StructureByteStride = 0;
		
		DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreateBuffer(&constantBufferDesc, nullptr, m_constantBuffer.GetAddressOf()));

	});

	createRendererResourcesTask.then([this]() {


		m_loadingComplete = true;

	});

}

void GridRenderer::CreateVertexBuffer()
{

	// This code could be better probably, but I was ok with it and i think it's pretty readable too.
	// The tricky part is that we need to order the lines from the edges inward up to the centerlines,
	// that is from [-size to 0), then from [size to 0] in both horizontal and vertical directions,
	// while keeping an order that's easy to iterate through to set the different widths.

	// 2 lines, each consist of 2 vertex + the farthermost vertices.
	m_vertexCount = 2 * (2 * m_totalSegmentCount + 4);
	const float increment = m_gridSize / m_totalSegmentCount;

	// For little bit cleaner code, these are the extremes
	const float X0 = -m_gridSize / 2.f;
	const float X1 =  m_gridSize / 2.f;
	const float Z0 = -m_gridSize / 2.f;
	const float Z1 =  m_gridSize / 2.f;

	constexpr float skinnyLineThickness = 3.f;
	constexpr float thickLineThickness  = 6.f;

	constexpr float depthBias = (float)5.0e-4;

	using namespace DirectX;
	std::vector<XMFLOAT4> vertices;
	vertices.reserve(m_vertexCount);
	// Vertical lines:
	for (int i = 0; i < m_totalSegmentCount / 2; ++i)
	{
		vertices.emplace_back(XMFLOAT4{ X0, -depthBias, Z0 + (float)i * increment, skinnyLineThickness });
		vertices.emplace_back(XMFLOAT4{ X1, -depthBias, Z0 + (float)i * increment, skinnyLineThickness });
	}
	// from the other edge to the centerline.
	for (int i = 0; i <= m_totalSegmentCount / 2; ++i)
	{
		vertices.emplace_back(XMFLOAT4{ X0, -depthBias, Z1 - (float)i * increment, skinnyLineThickness });
		vertices.emplace_back(XMFLOAT4{ X1, -depthBias, Z1 - (float)i * increment, skinnyLineThickness });
	}
	
	// Set width.
	for (int i = 0; i <= m_totalSegmentCount; i += 10)
	{
		vertices[2*i+0].w = thickLineThickness;
		vertices[2*i+1].w = thickLineThickness;
	}

	// Horizontal lines:
	// from one edge to the centerline;
	for (int i = 0; i < m_totalSegmentCount / 2; ++i)
	{
		vertices.emplace_back(XMFLOAT4{ X1 - (float)i * increment, depthBias, Z0, skinnyLineThickness });
		vertices.emplace_back(XMFLOAT4{ X1 - (float)i * increment, depthBias, Z1, skinnyLineThickness });
	}
	// from the other edge to the centerline.
	for (int i = 0; i <= m_totalSegmentCount / 2; ++i)
	{
		vertices.emplace_back(XMFLOAT4{ X0 + (float)i * increment, depthBias, Z0, skinnyLineThickness });
		vertices.emplace_back(XMFLOAT4{ X0 + (float)i * increment, depthBias, Z1, skinnyLineThickness });
	}

	// Set width again:
	// we are drawing the lines in the [-size;size] range, 
	// including the farthermost ones at ==size; that's why we need the offset.
	for (int i = m_totalSegmentCount + 1; i <= 2 * m_totalSegmentCount + 4; i += 10)
	{
		vertices[2*i+0].w = thickLineThickness;
		vertices[2*i+1].w = thickLineThickness;
	}

	// Upload to vertex buffer.
	D3D11_SUBRESOURCE_DATA vertexBufferData;
	ZeroMemory(&vertexBufferData, sizeof(vertexBufferData));
	vertexBufferData.pSysMem = &vertices[0];
	vertexBufferData.SysMemPitch = 0;
	vertexBufferData.SysMemSlicePitch = 0;

	m_stride = sizeof(XMFLOAT4);
	m_offset = 0;

	D3D11_BUFFER_DESC vertexBufferDesc;
	ZeroMemory(&vertexBufferDesc, sizeof(vertexBufferDesc));
	vertexBufferDesc.ByteWidth = m_stride * m_vertexCount;
	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	vertexBufferDesc.CPUAccessFlags = 0;
	vertexBufferDesc.MiscFlags = 0;
	vertexBufferDesc.StructureByteStride = 0;

	DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreateBuffer(
		&vertexBufferDesc,
		&vertexBufferData,
		&m_vertexBuffer));

}

void GridRenderer::CreateWindowSizeDependentResources()
{

	// Cam code from sample app. Seemed ok.
	using namespace DirectX;
	Windows::Foundation::Size outputSize = m_deviceResources->GetOutputSize();
	XMStoreFloat2(&m_constantBufferData.resolution, { outputSize.Width, outputSize.Height });

	const float aspectRatio = outputSize.Width / outputSize.Height;
	float fovAngleY = 63.0f * XM_PI / 180.0f; // 35 mm focal length on a full frame camera
	if (aspectRatio < 1.0f) fovAngleY *= 2.0f;

	const XMMATRIX perspectiveMatrix = XMMatrixPerspectiveFovRH(fovAngleY, aspectRatio, 0.01f, 100.0f);
	const XMFLOAT4X4 orientation = m_deviceResources->GetOrientationTransform3D();
	const XMMATRIX orientationMatrix = XMLoadFloat4x4(&orientation);
	XMStoreFloat4x4(&m_constantBufferData.projection, XMMatrixTranspose(perspectiveMatrix * orientationMatrix));

	constexpr XMVECTOR up = { 0.0f, 1.0f, 0.0f, 0.0f };
	constexpr XMVECTORF32 at = { 0.0f, 0.0f, 0.0f, 0.0f };
	
	// Requested view
	constexpr XMVECTOR eye = { 0.0f, 2.f, -2.0f, 1.0f };

	// Pretty close to ground
	//constexpr XMVECTORF32 eye = { 0.0f, 0.8f, -3.0f, 0.0f };

	// Very close to ground
	//constexpr XMVECTORF32 eye = { 0.0f, 0.175f, -2.0f, 0.0f };

	XMStoreFloat4(&m_constantBufferData.eyePos, eye);
	XMStoreFloat4x4(&m_constantBufferData.view, XMMatrixTranspose(XMMatrixLookAtRH(eye, at, up)));

}

void GridRenderer::Update(const DX::StepTimer& timer)
{
	if (!m_loadingComplete)
		return;

	using namespace DirectX;
	const double rotation = timer.GetTotalSeconds() * (m_revolutionsPerMinute/60.0);
	XMStoreFloat4x4( &m_constantBufferData.model, XMMatrixRotationY((float)rotation + m_gridOrientation) );
	m_deviceResources->GetD3DDeviceContext()->UpdateSubresource1(m_constantBuffer.Get(), 0, NULL, &m_constantBufferData, 0, 0, 0);
}

void GridRenderer::Render()
{

	if (!m_loadingComplete)
		return;

	auto context = m_deviceResources->GetD3DDeviceContext();

	context->VSSetConstantBuffers(0, 1, m_constantBuffer.GetAddressOf());
	context->GSSetConstantBuffers(0, 1, m_constantBuffer.GetAddressOf());
	context->PSSetConstantBuffers(0, 1, m_constantBuffer.GetAddressOf());

	context->IASetVertexBuffers(0, 1, m_vertexBuffer.GetAddressOf(), &m_stride, &m_offset);
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
	context->IASetInputLayout(m_inputLayout.Get());

	context->VSSetShader(m_vertexShader.Get(), nullptr, 0);
	context->GSSetShader(m_geometryShader.Get(), nullptr, 0);
	context->PSSetShader(m_pixelShader.Get(), nullptr, 0);

	context->OMSetBlendState(m_blendState.Get(), nullptr, 0xfu);

	context->Draw(m_vertexCount, 0);

}

void GridRenderer::ReleaseDeviceDependentResources()
{

	m_loadingComplete = false;
	
	if (m_inputLayout)		m_inputLayout.Reset();

	if (m_vertexShader)		m_vertexShader.Reset();
	if (m_geometryShader)	m_geometryShader.Reset();
	if (m_pixelShader)		m_pixelShader.Reset();
	
	if (m_vertexBuffer)		m_vertexBuffer.Reset();
	
	if (m_blendState)		m_blendState.Reset();
	if (m_constantBuffer)	m_constantBuffer.Reset();

}