//
// Created by gabe on 8/21/25.
//

#ifndef CPP_ENGINE_INSPECTORRENDERER_H
#define CPP_ENGINE_INSPECTORRENDERER_H

#include "core/Entity.h"
namespace Engine {

	class InspectorRenderer {
	  public:
		void RenderInspectorWindow(Entity* m_selectedEntityP);
		bool m_openPopup = false;
	};

} // namespace Engine

#endif // CPP_ENGINE_INSPECTORRENDERER_H
