#pragma once

#include "Common\StepTimer.h"
#include "Common\DeviceResources.h"
#include "Content\GridRenderer.h"

// Renders Direct2D and 3D content on the screen.
namespace MyGrid
{
	class MyGridMain : public DX::IDeviceNotify
	{
	public:
		MyGridMain(const std::shared_ptr<DX::DeviceResources>& deviceResources);
		~MyGridMain();
		void CreateWindowSizeDependentResources();
		void Update();
		bool Render();

		// IDeviceNotify
		virtual void OnDeviceLost();
		virtual void OnDeviceRestored();

	private:
		// Cached pointer to device resources.
		std::shared_ptr<DX::DeviceResources> m_deviceResources;

		// TODO: Replace with your own content renderers.
		std::unique_ptr<GridRenderer> m_gridRenderer;

		// Rendering loop timer.
		DX::StepTimer m_timer;
	};
}