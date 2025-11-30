#pragma once

#include "components/Components.h"
#include "cereal/cereal.hpp"
#include <RmlUi/Core.h>
#include <string>



namespace Engine::Components {

	class RmlUIComponent : public Component {
	public:
		RmlUIComponent() = default;
		~RmlUIComponent() override = default;


		template <class Archive>
		void serialize(Archive& ar)
		{
			ar(cereal::make_nvp("documentPath", m_documentPath));
			ar(cereal::make_nvp("isVisible", m_isVisible));
		}

		void OnAdded(Entity& entity) override;
		void OnRemoved(Entity& entity) override;
		void RenderInspector(Entity& entity) override;

		// Load an RML document
		void LoadDocument(const std::string& path, Entity& entity);
		
		// Unload the current document
		void UnloadDocument();
		
		// Visibility control
		void SetVisible(bool visible);
		bool IsVisible() const { return m_isVisible; }

		// Get the loaded document
		Rml::ElementDocument* GetDocument() const { return m_document; }
		
		// Get the document path
		const std::string& GetDocumentPath() const { return m_documentPath; }

		static void AddBindings();

	private:
		std::string m_documentPath;
		bool m_isVisible = true;
		Rml::ElementDocument* m_document = nullptr;
	};

} // namespace Engine::Components
