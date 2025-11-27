//
// Created by gabe on 8/23/25.
//

#ifndef CPP_ENGINE_MATERIALEDITOR_H
#define CPP_ENGINE_MATERIALEDITOR_H

#include "rendering/Material.h"
#include "rendering/ui/MaterialPreview.h"

namespace Engine {
	class MaterialEditor {
	  public:
		void RenderMaterialEditor(AssetHandle<Material> matRef);

	  private:
		void RenderPreviewPanel(Material* material);
		
		// Preview state
		MaterialPreview m_preview;
		float m_previewYaw = 45.0f;   // Horizontal rotation angle
		float m_previewPitch = 30.0f; // Vertical rotation angle
		bool m_isDragging = false;
	};
} // namespace Engine
#endif // CPP_ENGINE_MATERIALEDITOR_H
