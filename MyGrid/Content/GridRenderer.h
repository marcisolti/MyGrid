#pragma once

#include "..\Common\DeviceResources.h"
#include "..\Common\StepTimer.h"

// You can find the docs at %(SolutionDir)..\docs\index.html

namespace MyGrid
{
	struct alignas(16) ConstantBufferData
	{
		DirectX::XMFLOAT4X4 model;
		DirectX::XMFLOAT4X4 view;
		DirectX::XMFLOAT4X4 projection;
		DirectX::XMFLOAT4   eyePos;
		DirectX::XMFLOAT2   resolution;
	};

	class GridRenderer
	{

		// Constants:
		const double	m_revolutionsPerMinute;
		const float		m_gridOrientation;
		const float		m_gridSize;
		const int		m_totalSegmentCount;

	public:

		GridRenderer(const std::shared_ptr<DX::DeviceResources>& deviceResources);
		void CreateDeviceDependentResources();
		void CreateWindowSizeDependentResources();
		void ReleaseDeviceDependentResources();
		void Update(const DX::StepTimer& timer);
		void Render();

	private:

		void CreateVertexBuffer();

		std::shared_ptr<DX::DeviceResources>			m_deviceResources;
		
		Microsoft::WRL::ComPtr<ID3D11VertexShader>		m_vertexShader;
		Microsoft::WRL::ComPtr<ID3D11GeometryShader>	m_geometryShader;
		Microsoft::WRL::ComPtr<ID3D11PixelShader>		m_pixelShader;

		Microsoft::WRL::ComPtr<ID3D11InputLayout>		m_inputLayout;
		Microsoft::WRL::ComPtr<ID3D11Buffer>			m_vertexBuffer;
		uint32_t										m_vertexCount;
		uint32_t										m_stride;
		uint32_t										m_offset;

		Microsoft::WRL::ComPtr<ID3D11Buffer>			m_constantBuffer;
		ConstantBufferData								m_constantBufferData;

		Microsoft::WRL::ComPtr<ID3D11BlendState>		m_blendState;

		bool											m_loadingComplete;

	};
}