#include "RmlUIComponent.h"
#include "core/EngineData.h"
#include "core/Entity.h"
#include "core/Window.h"
#include "scripting/ScriptManager.h"

#include "rendering/ui/GameUIManager.h"
#include "rendering/ui/InspectorUI.h"
#include <RmlUi/Lua.h>

namespace Engine::Components {

	void RmlUIComponent::OnAdded(Entity& entity) {
		
		// If a document path is already set, load it
		if (!m_documentPath.empty()) {
			LoadDocument(m_documentPath, entity);
		}
	}

	void RmlUIComponent::OnRemoved(Entity& entity) {
		UnloadDocument();
	}

	void RmlUIComponent::LoadDocument(const std::string& path, Entity& entity) {
		// Unload existing document first
		UnloadDocument();
		
		m_documentPath = path;

		
		
		// Get the RmlUI context from GameUIManager
		Rml::Context* context = GetGameUIManager().GetContext();
		if (!context) {
			GetWindow().log->error("RmlUIComponent: No RmlUI context available");
			return;
		}

		try {
				lua_State* L = GetScriptManager().lua.lua_state();
				// Note: Rml::Lua::Initialise is called once globally by GameUIManager
				
				// Set up Lua environment with gameObject reference BEFORE loading the document
				// This is crucial because inline <script> tags execute during LoadDocument()
				sol::state_view lua(L);
				lua["gameObject"] = entity;

				// Load the document
				m_document = context->LoadDocument(path);
				if (m_document) {
					if (m_isVisible) {
						m_document->Show();
					} else {
						m_document->Hide();
					}
					GetWindow().log->info("Loaded RmlUi document: {}", path);
				} else {
					GetWindow().log->warn("Failed to load RmlUi document: {}", path);
					return;
				}

			} catch (const std::exception& e) {
				GetWindow().log->error("RmlUIComponent: Failed to load document: {}", e.what());
			}



		
		
		// Show the document
		// This line is now redundant as visibility is handled within the try block
		// m_document->Show(); 
	}

	void RmlUIComponent::UnloadDocument() {
		if (m_document) {
			m_document->Close();
			m_document = nullptr;
		}
	}

	void RmlUIComponent::SetVisible(bool visible) {
		m_isVisible = visible;
		if (m_document) {
			// Validate the document is still part of the context
			// (it may have been closed by resetDocuments or externally)
			auto* context = GetGameUIManager().GetContext();
			if (context) {
				bool documentStillExists = false;
				for (int i = 0; i < context->GetNumDocuments(); ++i) {
					if (context->GetDocument(i) == m_document) {
						documentStillExists = true;
						break;
					}
				}
				
				if (documentStillExists) {
					if (m_isVisible) {
						m_document->Show();
					} else {
						m_document->Hide();
					}
				} else {
					// Document was closed externally, nullify our pointer
					m_document = nullptr;
				}
			}
		}
	}

	void RmlUIComponent::RenderInspector(Entity& entity) {
		// Allow editing the document path
		char pathBuffer[256];
		strncpy(pathBuffer, m_documentPath.c_str(), sizeof(pathBuffer) - 1);
		pathBuffer[sizeof(pathBuffer) - 1] = '\0';
		
		if (ImGui::InputText("Document Path", pathBuffer, sizeof(pathBuffer))) {
			m_documentPath = pathBuffer;
		}
		ImGui::SameLine();
		if (BrowsePathButton("rml", "rml", "resources/ui", &m_documentPath)) {
			strncpy(pathBuffer, m_documentPath.c_str(), sizeof(pathBuffer) - 1);
		}
		
		if (ImGui::Checkbox("Visible", &m_isVisible)) {
			SetVisible(m_isVisible);
		}

		if (ImGui::Button("Load Document")) {
			LoadDocument(m_documentPath,entity);
		}
		
		ImGui::SameLine();
		
		if (ImGui::Button("Unload Document")) {
			UnloadDocument();
		}
		
		// Display document status
		ImGui::Separator();
		if (m_document) {
			ImGui::TextColored(ImVec4(0, 1, 0, 1), "Document Loaded");
			ImGui::Text("ID: %s", m_document->GetId().c_str());
		} else {
			ImGui::TextColored(ImVec4(1, 0, 0, 1), "No Document");
		}
	}

	void RmlUIComponent::AddBindings() {
		auto& lua = GetScriptManager().lua;
		
		lua.new_usertype<RmlUIComponent>(
			"RmlUIComponent",
			sol::base_classes, sol::bases<Component>(),
			"documentPath", &RmlUIComponent::m_documentPath,
			"LoadDocument", &RmlUIComponent::LoadDocument,
			"UnloadDocument", &RmlUIComponent::UnloadDocument,
			"SetVisible", &RmlUIComponent::SetVisible,
			"IsVisible", &RmlUIComponent::IsVisible,
			"visible", sol::property(&RmlUIComponent::IsVisible, &RmlUIComponent::SetVisible)
		);



#ifndef GAME_BUILD
		// Editor-only debug ImGui from Lua. Game builds have no ImGui frame cycle —
		// calling Begin without NewFrame/Render corrupts font stacks and crashes.
		sol::table imgui = lua.create_named_table("imgui");

		imgui.set_function("Begin",
		                   [](const char* name, sol::optional<int> flags) {
			                   if (flags)
				                   return ImGui::Begin(name, nullptr, *flags);
			                   return ImGui::Begin(name);
		                   });

		imgui.set_function("End", []() { ImGui::End(); });

		imgui.set_function("Text", [](const char* text) { ImGui::TextUnformatted(text); });

		imgui.set_function("Button", [](const char* label) { return ImGui::Button(label); });

		imgui.set_function("SameLine", []() { ImGui::SameLine(); });

		imgui.set_function("Separator", []() { ImGui::Separator(); });

		imgui.set_function("Checkbox",
		                   [](const char* label, sol::table value) {
			                   bool v       = value["v"];
			                   bool changed = ImGui::Checkbox(label, &v);
			                   value["v"]   = v;
			                   return changed;
		                   });

		imgui.set_function("SliderFloat",
		                   [](const char* label, sol::table value, float min, float max) {
			                   float v      = value["v"];
			                   bool changed = ImGui::SliderFloat(label, &v, min, max);
			                   value["v"]   = v;
			                   return changed;
		                   });

		imgui.set_function("SliderInt",
		                   [](const char* label, sol::table value, int min, int max) {
			                   int  v       = value["v"];
			                   bool changed = ImGui::SliderInt(label, &v, min, max);
			                   value["v"]   = v;
			                   return changed;
		                   });

		imgui["WindowFlags"] = lua.create_table_with(
		    "None", ImGuiWindowFlags_None,
		    "NoTitleBar", ImGuiWindowFlags_NoTitleBar,
		    "NoResize", ImGuiWindowFlags_NoResize,
		    "NoMove", ImGuiWindowFlags_NoMove,
		    "NoCollapse", ImGuiWindowFlags_NoCollapse);

		imgui[sol::meta_function::new_index] =
		    [](sol::table, sol::object key, sol::object) {
			    throw std::runtime_error(
			        "Attempt to modify read-only imgui binding: " + key.as<std::string>());
		    };
#endif

	}

} // namespace Engine::Components
