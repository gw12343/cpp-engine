#pragma once

#include "assets/AssetHandle.h"
#include "core/Entity.h"
#include "core/EntityHandle.h"
#include "core/module/Module.h"

#include <memory>
#include <string>
#include <unordered_set>

namespace Engine {
	class GEngine;
	class Texture;
	class GBuffer;
	class InspectorRenderer;
	class AssetUIRenderer;
	class MaterialEditor;

	namespace UI {

		class UIManager : public Module {
		  public:
			UIManager();
			~UIManager() override;

			void                      onInit() override;
			void                      onUpdate(float dt) override;
			void                      onGameStart() override {}
			void                      onShutdown() override;
			[[nodiscard]] std::string name() const override { return "UIModule"; };
			void                      setLuaBindings() override;

			void BeginDockspace(float ht);
			void EndDockspace();

			std::shared_ptr<Texture> m_audioIconTexture;
			std::shared_ptr<Texture> m_terrainIconTexture;
			std::shared_ptr<Texture> m_animationIconTexture;
			std::shared_ptr<Texture> m_skeletonIconTexture;
			std::shared_ptr<Texture> m_folderIconTexture;
			std::shared_ptr<Texture> m_fileIconTexture;
			std::shared_ptr<Texture> m_modelIconTexture;
			std::shared_ptr<Texture> m_shaderIconTexture;
			std::shared_ptr<Texture> m_particleIconTexture;
			std::shared_ptr<Texture> m_materialIconTexture;

			MaterialHandle                        m_selectedMaterial;
			AssetHandle<Engine::Rendering::Model> m_selectedModel;

			Entity m_selectedEntity = Entity();

			Entity DuplicateEntity(Entity source);
			void   RevealInHierarchy(Entity entity);

			std::unique_ptr<InspectorRenderer> m_inspectorRenderer;
			bool                               isOverSceneView() const;
			[[nodiscard]] const Engine::Entity& getSelectedEntity() const { return m_selectedEntity; }

		  private:
			void  RenderHierarchyWindow();
			float RenderTopBar(float top);
			float RenderMainMenuBar();
			void  RenderPauseOverlay();
			void  RenderGBufferDebug(std::shared_ptr<GBuffer> gbuffer);
			void  RenderModelDebug(AssetHandle<Engine::Rendering::Model> handle);
			void  FlushHierarchyCommands();

			bool                             m_overSceneView = false;
			std::unique_ptr<AssetUIRenderer> m_uiAssetRenderer;
			std::unique_ptr<MaterialEditor>  m_materialEditor;
			int                              m_selectedTheme = 0;
			void                             RenderEntityTreeNode(Entity entity);
			void                             DrawAddEntityMenu();

			enum class HierarchyCommand { None, Delete, Duplicate, CreateChild, SavePrefab, InstantiatePrefab, Reparent, DropPrefab };
			HierarchyCommand                m_hierarchyCommand = HierarchyCommand::None;
			Entity                          m_hierarchyCommandEntity;
			EntityHandle                    m_hierarchyCommandParent;
			std::string                     m_hierarchyDropPrefabId;
			std::string                     m_renamingGuid;
			char                            m_renameBuffer[256]     = {};
			bool                            m_renameFocusRequested  = false;
			std::unordered_set<std::string> m_hierarchyRevealGuids;
			bool                            m_hierarchyScrollToSelection = false;
		};
	} // namespace UI
} // namespace Engine
