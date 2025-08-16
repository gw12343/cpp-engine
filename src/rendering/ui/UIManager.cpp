#include "UIManager.h"

#include "components/Components.h"
#include "components/impl/AnimationComponent.h"
#include "spdlog/spdlog.h"
#include "glm/glm.hpp"
#include "core/EngineData.h"
#include "animation/AnimationManager.h"

#include "rendering/Renderer.h"
#include "rendering/ui/IconsFontAwesome6.h"

#include "physics/PhysicsManager.h"
#include "components/impl/LuaScriptComponent.h"
#include "components/impl/EntityMetadataComponent.h"
#include "components/impl/SkeletonComponent.h"
#include "components/impl/RigidBodyComponent.h"
#include "components/impl/AudioSourceComponent.h"
#include "components/impl/ModelRendererComponent.h"
#include "components/impl/AnimationPoseComponent.h"
#include "components/impl/AnimationWorkerComponent.h"
#include "components/impl/SkinnedMeshComponent.h"
#include "components/impl/ParticleSystemComponent.h"
#include "components/impl/ShadowCasterComponent.h"
#include "imgui_internal.h"
#include "components/impl/TerrainRendererComponent.h"

namespace Engine::UI {

	void SetThemeColors(int t)
	{
		ImGuiStyle& style  = ImGui::GetStyle();
		ImVec4*     colors = style.Colors;


		switch (t) {
			case 0:
				// Any initialization code for UI manager
				// Primary background
				colors[ImGuiCol_WindowBg]  = ImVec4(0.07f, 0.07f, 0.09f, 1.00f); // #131318
				colors[ImGuiCol_MenuBarBg] = ImVec4(0.12f, 0.12f, 0.15f, 1.00f); // #131318

				colors[ImGuiCol_PopupBg] = ImVec4(0.18f, 0.18f, 0.22f, 1.00f);

				// Headers
				colors[ImGuiCol_Header]        = ImVec4(0.18f, 0.18f, 0.22f, 1.00f);
				colors[ImGuiCol_HeaderHovered] = ImVec4(0.30f, 0.30f, 0.40f, 1.00f);
				colors[ImGuiCol_HeaderActive]  = ImVec4(0.25f, 0.25f, 0.35f, 1.00f);

				// Buttons
				colors[ImGuiCol_Button]        = ImVec4(0.20f, 0.22f, 0.27f, 1.00f);
				colors[ImGuiCol_ButtonHovered] = ImVec4(0.30f, 0.32f, 0.40f, 1.00f);
				colors[ImGuiCol_ButtonActive]  = ImVec4(0.35f, 0.38f, 0.50f, 1.00f);

				// Frame BG
				colors[ImGuiCol_FrameBg]        = ImVec4(0.15f, 0.15f, 0.18f, 1.00f);
				colors[ImGuiCol_FrameBgHovered] = ImVec4(0.22f, 0.22f, 0.27f, 1.00f);
				colors[ImGuiCol_FrameBgActive]  = ImVec4(0.25f, 0.25f, 0.30f, 1.00f);

				// Tabs
				colors[ImGuiCol_Tab]                = ImVec4(0.18f, 0.18f, 0.22f, 1.00f);
				colors[ImGuiCol_TabHovered]         = ImVec4(0.35f, 0.35f, 0.50f, 1.00f);
				colors[ImGuiCol_TabActive]          = ImVec4(0.25f, 0.25f, 0.38f, 1.00f);
				colors[ImGuiCol_TabUnfocused]       = ImVec4(0.13f, 0.13f, 0.17f, 1.00f);
				colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.20f, 0.20f, 0.25f, 1.00f);

				// Title
				colors[ImGuiCol_TitleBg]          = ImVec4(0.12f, 0.12f, 0.15f, 1.00f);
				colors[ImGuiCol_TitleBgActive]    = ImVec4(0.15f, 0.15f, 0.20f, 1.00f);
				colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.10f, 0.10f, 0.12f, 1.00f);

				// Borders
				colors[ImGuiCol_Border]       = ImVec4(0.20f, 0.20f, 0.25f, 0.50f);
				colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);

				// Text
				colors[ImGuiCol_Text]         = ImVec4(0.90f, 0.90f, 0.95f, 1.00f);
				colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.55f, 1.00f);

				// Highlights
				colors[ImGuiCol_CheckMark]         = ImVec4(0.50f, 0.70f, 1.00f, 1.00f);
				colors[ImGuiCol_SliderGrab]        = ImVec4(0.50f, 0.70f, 1.00f, 1.00f);
				colors[ImGuiCol_SliderGrabActive]  = ImVec4(0.60f, 0.80f, 1.00f, 1.00f);
				colors[ImGuiCol_ResizeGrip]        = ImVec4(0.50f, 0.70f, 1.00f, 0.50f);
				colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.60f, 0.80f, 1.00f, 0.75f);
				colors[ImGuiCol_ResizeGripActive]  = ImVec4(0.70f, 0.90f, 1.00f, 1.00f);

				// Scrollbar
				colors[ImGuiCol_ScrollbarBg]          = ImVec4(0.10f, 0.10f, 0.12f, 1.00f);
				colors[ImGuiCol_ScrollbarGrab]        = ImVec4(0.30f, 0.30f, 0.35f, 1.00f);
				colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.40f, 0.40f, 0.50f, 1.00f);
				colors[ImGuiCol_ScrollbarGrabActive]  = ImVec4(0.45f, 0.45f, 0.55f, 1.00f);

				// Style tweaks
				style.WindowRounding    = 5.0f;
				style.FrameRounding     = 5.0f;
				style.GrabRounding      = 5.0f;
				style.TabRounding       = 5.0f;
				style.PopupRounding     = 5.0f;
				style.ScrollbarRounding = 5.0f;
				style.WindowPadding     = ImVec2(10, 10);
				style.FramePadding      = ImVec2(6, 4);
				style.ItemSpacing       = ImVec2(8, 6);
				style.PopupBorderSize   = 0.f;
				break;
			case 1:
				// Base colors inspired by Catppuccin Mocha
				colors[ImGuiCol_Text]                  = ImVec4(0.90f, 0.89f, 0.88f, 1.00f); // Latte
				colors[ImGuiCol_TextDisabled]          = ImVec4(0.60f, 0.56f, 0.52f, 1.00f); // Surface2
				colors[ImGuiCol_WindowBg]              = ImVec4(0.17f, 0.14f, 0.20f, 1.00f); // Base
				colors[ImGuiCol_ChildBg]               = ImVec4(0.18f, 0.16f, 0.22f, 1.00f); // Mantle
				colors[ImGuiCol_PopupBg]               = ImVec4(0.17f, 0.14f, 0.20f, 1.00f); // Base
				colors[ImGuiCol_Border]                = ImVec4(0.27f, 0.23f, 0.29f, 1.00f); // Overlay0
				colors[ImGuiCol_BorderShadow]          = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
				colors[ImGuiCol_FrameBg]               = ImVec4(0.21f, 0.18f, 0.25f, 1.00f); // Crust
				colors[ImGuiCol_FrameBgHovered]        = ImVec4(0.24f, 0.20f, 0.29f, 1.00f); // Overlay1
				colors[ImGuiCol_FrameBgActive]         = ImVec4(0.26f, 0.22f, 0.31f, 1.00f); // Overlay2
				colors[ImGuiCol_TitleBg]               = ImVec4(0.14f, 0.12f, 0.18f, 1.00f); // Mantle
				colors[ImGuiCol_TitleBgActive]         = ImVec4(0.17f, 0.15f, 0.21f, 1.00f); // Mantle
				colors[ImGuiCol_TitleBgCollapsed]      = ImVec4(0.14f, 0.12f, 0.18f, 1.00f); // Mantle
				colors[ImGuiCol_MenuBarBg]             = ImVec4(0.17f, 0.15f, 0.22f, 1.00f); // Base
				colors[ImGuiCol_ScrollbarBg]           = ImVec4(0.17f, 0.14f, 0.20f, 1.00f); // Base
				colors[ImGuiCol_ScrollbarGrab]         = ImVec4(0.21f, 0.18f, 0.25f, 1.00f); // Crust
				colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(0.24f, 0.20f, 0.29f, 1.00f); // Overlay1
				colors[ImGuiCol_ScrollbarGrabActive]   = ImVec4(0.26f, 0.22f, 0.31f, 1.00f); // Overlay2
				colors[ImGuiCol_CheckMark]             = ImVec4(0.95f, 0.66f, 0.47f, 1.00f); // Peach
				colors[ImGuiCol_SliderGrab]            = ImVec4(0.82f, 0.61f, 0.85f, 1.00f); // Lavender
				colors[ImGuiCol_SliderGrabActive]      = ImVec4(0.89f, 0.54f, 0.79f, 1.00f); // Pink
				colors[ImGuiCol_Button]                = ImVec4(0.65f, 0.34f, 0.46f, 1.00f); // Maroon
				colors[ImGuiCol_ButtonHovered]         = ImVec4(0.71f, 0.40f, 0.52f, 1.00f); // Red
				colors[ImGuiCol_ButtonActive]          = ImVec4(0.76f, 0.46f, 0.58f, 1.00f); // Pink
				colors[ImGuiCol_Header]                = ImVec4(0.65f, 0.34f, 0.46f, 1.00f); // Maroon
				colors[ImGuiCol_HeaderHovered]         = ImVec4(0.71f, 0.40f, 0.52f, 1.00f); // Red
				colors[ImGuiCol_HeaderActive]          = ImVec4(0.76f, 0.46f, 0.58f, 1.00f); // Pink
				colors[ImGuiCol_Separator]             = ImVec4(0.27f, 0.23f, 0.29f, 1.00f); // Overlay0
				colors[ImGuiCol_SeparatorHovered]      = ImVec4(0.95f, 0.66f, 0.47f, 1.00f); // Peach
				colors[ImGuiCol_SeparatorActive]       = ImVec4(0.95f, 0.66f, 0.47f, 1.00f); // Peach
				colors[ImGuiCol_ResizeGrip]            = ImVec4(0.82f, 0.61f, 0.85f, 1.00f); // Lavender
				colors[ImGuiCol_ResizeGripHovered]     = ImVec4(0.89f, 0.54f, 0.79f, 1.00f); // Pink
				colors[ImGuiCol_ResizeGripActive]      = ImVec4(0.92f, 0.61f, 0.85f, 1.00f); // Mauve
				colors[ImGuiCol_Tab]                   = ImVec4(0.21f, 0.18f, 0.25f, 1.00f); // Crust
				colors[ImGuiCol_TabHovered]            = ImVec4(0.82f, 0.61f, 0.85f, 1.00f); // Lavender
				colors[ImGuiCol_TabActive]             = ImVec4(0.76f, 0.46f, 0.58f, 1.00f); // Pink
				colors[ImGuiCol_TabUnfocused]          = ImVec4(0.18f, 0.16f, 0.22f, 1.00f); // Mantle
				colors[ImGuiCol_TabUnfocusedActive]    = ImVec4(0.21f, 0.18f, 0.25f, 1.00f); // Crust
				colors[ImGuiCol_DockingPreview]        = ImVec4(0.95f, 0.66f, 0.47f, 0.70f); // Peach
				colors[ImGuiCol_DockingEmptyBg]        = ImVec4(0.12f, 0.12f, 0.12f, 1.00f); // Base
				colors[ImGuiCol_PlotLines]             = ImVec4(0.82f, 0.61f, 0.85f, 1.00f); // Lavender
				colors[ImGuiCol_PlotLinesHovered]      = ImVec4(0.89f, 0.54f, 0.79f, 1.00f); // Pink
				colors[ImGuiCol_PlotHistogram]         = ImVec4(0.82f, 0.61f, 0.85f, 1.00f); // Lavender
				colors[ImGuiCol_PlotHistogramHovered]  = ImVec4(0.89f, 0.54f, 0.79f, 1.00f); // Pink
				colors[ImGuiCol_TableHeaderBg]         = ImVec4(0.19f, 0.19f, 0.20f, 1.00f); // Mantle
				colors[ImGuiCol_TableBorderStrong]     = ImVec4(0.27f, 0.23f, 0.29f, 1.00f); // Overlay0
				colors[ImGuiCol_TableBorderLight]      = ImVec4(0.23f, 0.23f, 0.25f, 1.00f); // Surface2
				colors[ImGuiCol_TableRowBg]            = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
				colors[ImGuiCol_TableRowBgAlt]         = ImVec4(1.00f, 1.00f, 1.00f, 0.06f); // Surface0
				colors[ImGuiCol_TextSelectedBg]        = ImVec4(0.82f, 0.61f, 0.85f, 0.35f); // Lavender
				colors[ImGuiCol_DragDropTarget]        = ImVec4(0.95f, 0.66f, 0.47f, 0.90f); // Peach
				colors[ImGuiCol_NavHighlight]          = ImVec4(0.82f, 0.61f, 0.85f, 1.00f); // Lavender
				colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
				colors[ImGuiCol_NavWindowingDimBg]     = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
				colors[ImGuiCol_ModalWindowDimBg]      = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);

				// Style adjustments
				style.WindowRounding    = 6.0f;
				style.FrameRounding     = 4.0f;
				style.ScrollbarRounding = 4.0f;
				style.GrabRounding      = 3.0f;
				style.ChildRounding     = 4.0f;

				style.WindowTitleAlign = ImVec2(0.50f, 0.50f);
				style.WindowPadding    = ImVec2(8.0f, 8.0f);
				style.FramePadding     = ImVec2(5.0f, 4.0f);
				style.ItemSpacing      = ImVec2(6.0f, 6.0f);
				style.ItemInnerSpacing = ImVec2(6.0f, 6.0f);
				style.IndentSpacing    = 22.0f;

				style.ScrollbarSize = 14.0f;
				style.GrabMinSize   = 10.0f;

				style.AntiAliasedLines = true;
				style.AntiAliasedFill  = true;
				break;
			case 2:
				// Base color scheme
				colors[ImGuiCol_Text]                  = ImVec4(0.92f, 0.92f, 0.92f, 1.00f);
				colors[ImGuiCol_TextDisabled]          = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
				colors[ImGuiCol_WindowBg]              = ImVec4(0.13f, 0.14f, 0.15f, 1.00f);
				colors[ImGuiCol_ChildBg]               = ImVec4(0.13f, 0.14f, 0.15f, 1.00f);
				colors[ImGuiCol_PopupBg]               = ImVec4(0.10f, 0.10f, 0.11f, 0.94f);
				colors[ImGuiCol_Border]                = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
				colors[ImGuiCol_BorderShadow]          = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
				colors[ImGuiCol_FrameBg]               = ImVec4(0.20f, 0.21f, 0.22f, 1.00f);
				colors[ImGuiCol_FrameBgHovered]        = ImVec4(0.25f, 0.26f, 0.27f, 1.00f);
				colors[ImGuiCol_FrameBgActive]         = ImVec4(0.18f, 0.19f, 0.20f, 1.00f);
				colors[ImGuiCol_TitleBg]               = ImVec4(0.15f, 0.15f, 0.16f, 1.00f);
				colors[ImGuiCol_TitleBgActive]         = ImVec4(0.15f, 0.15f, 0.16f, 1.00f);
				colors[ImGuiCol_TitleBgCollapsed]      = ImVec4(0.15f, 0.15f, 0.16f, 1.00f);
				colors[ImGuiCol_MenuBarBg]             = ImVec4(0.20f, 0.20f, 0.21f, 1.00f);
				colors[ImGuiCol_ScrollbarBg]           = ImVec4(0.20f, 0.21f, 0.22f, 1.00f);
				colors[ImGuiCol_ScrollbarGrab]         = ImVec4(0.28f, 0.28f, 0.29f, 1.00f);
				colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(0.33f, 0.34f, 0.35f, 1.00f);
				colors[ImGuiCol_ScrollbarGrabActive]   = ImVec4(0.40f, 0.40f, 0.41f, 1.00f);
				colors[ImGuiCol_CheckMark]             = ImVec4(0.76f, 0.76f, 0.76f, 1.00f);
				colors[ImGuiCol_SliderGrab]            = ImVec4(0.28f, 0.56f, 1.00f, 1.00f);
				colors[ImGuiCol_SliderGrabActive]      = ImVec4(0.37f, 0.61f, 1.00f, 1.00f);
				colors[ImGuiCol_Button]                = ImVec4(0.20f, 0.25f, 0.30f, 1.00f);
				colors[ImGuiCol_ButtonHovered]         = ImVec4(0.30f, 0.35f, 0.40f, 1.00f);
				colors[ImGuiCol_ButtonActive]          = ImVec4(0.25f, 0.30f, 0.35f, 1.00f);
				colors[ImGuiCol_Header]                = ImVec4(0.25f, 0.25f, 0.25f, 0.80f);
				colors[ImGuiCol_HeaderHovered]         = ImVec4(0.30f, 0.30f, 0.30f, 0.80f);
				colors[ImGuiCol_HeaderActive]          = ImVec4(0.35f, 0.35f, 0.35f, 0.80f);
				colors[ImGuiCol_Separator]             = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
				colors[ImGuiCol_SeparatorHovered]      = ImVec4(0.33f, 0.67f, 1.00f, 1.00f);
				colors[ImGuiCol_SeparatorActive]       = ImVec4(0.33f, 0.67f, 1.00f, 1.00f);
				colors[ImGuiCol_ResizeGrip]            = ImVec4(0.28f, 0.56f, 1.00f, 1.00f);
				colors[ImGuiCol_ResizeGripHovered]     = ImVec4(0.37f, 0.61f, 1.00f, 1.00f);
				colors[ImGuiCol_ResizeGripActive]      = ImVec4(0.37f, 0.61f, 1.00f, 1.00f);
				colors[ImGuiCol_Tab]                   = ImVec4(0.15f, 0.18f, 0.22f, 1.00f);
				colors[ImGuiCol_TabHovered]            = ImVec4(0.38f, 0.48f, 0.69f, 1.00f);
				colors[ImGuiCol_TabActive]             = ImVec4(0.28f, 0.38f, 0.59f, 1.00f);
				colors[ImGuiCol_TabUnfocused]          = ImVec4(0.15f, 0.18f, 0.22f, 1.00f);
				colors[ImGuiCol_TabUnfocusedActive]    = ImVec4(0.15f, 0.18f, 0.22f, 1.00f);
				colors[ImGuiCol_DockingPreview]        = ImVec4(0.28f, 0.56f, 1.00f, 1.00f);
				colors[ImGuiCol_DockingEmptyBg]        = ImVec4(0.13f, 0.14f, 0.15f, 1.00f);
				colors[ImGuiCol_PlotLines]             = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
				colors[ImGuiCol_PlotLinesHovered]      = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
				colors[ImGuiCol_PlotHistogram]         = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
				colors[ImGuiCol_PlotHistogramHovered]  = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
				colors[ImGuiCol_TableHeaderBg]         = ImVec4(0.19f, 0.19f, 0.20f, 1.00f);
				colors[ImGuiCol_TableBorderStrong]     = ImVec4(0.31f, 0.31f, 0.35f, 1.00f);
				colors[ImGuiCol_TableBorderLight]      = ImVec4(0.23f, 0.23f, 0.25f, 1.00f);
				colors[ImGuiCol_TableRowBg]            = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
				colors[ImGuiCol_TableRowBgAlt]         = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
				colors[ImGuiCol_TextSelectedBg]        = ImVec4(0.28f, 0.56f, 1.00f, 0.35f);
				colors[ImGuiCol_DragDropTarget]        = ImVec4(0.28f, 0.56f, 1.00f, 0.90f);
				colors[ImGuiCol_NavHighlight]          = ImVec4(0.28f, 0.56f, 1.00f, 1.00f);
				colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
				colors[ImGuiCol_NavWindowingDimBg]     = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
				colors[ImGuiCol_ModalWindowDimBg]      = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);

				// Style adjustments
				style.WindowRounding    = 5.3f;
				style.FrameRounding     = 2.3f;
				style.ScrollbarRounding = 0;

				style.WindowTitleAlign = ImVec2(0.50f, 0.50f);
				style.WindowPadding    = ImVec2(8.0f, 8.0f);
				style.FramePadding     = ImVec2(5.0f, 5.0f);
				style.ItemSpacing      = ImVec2(6.0f, 6.0f);
				style.ItemInnerSpacing = ImVec2(6.0f, 6.0f);
				style.IndentSpacing    = 25.0f;
				break;
			case 3:
				colors[ImGuiCol_WindowBg] = ImVec4{0.1f, 0.105f, 0.11f, 1.0f};

				// Headers
				colors[ImGuiCol_Header]        = ImVec4{0.2f, 0.205f, 0.21f, 1.0f};
				colors[ImGuiCol_HeaderHovered] = ImVec4{0.3f, 0.305f, 0.31f, 1.0f};
				colors[ImGuiCol_HeaderActive]  = ImVec4{0.15f, 0.1505f, 0.151f, 1.0f};

				// Buttons
				colors[ImGuiCol_Button]        = ImVec4{0.2f, 0.205f, 0.21f, 1.0f};
				colors[ImGuiCol_ButtonHovered] = ImVec4{0.3f, 0.305f, 0.31f, 1.0f};
				colors[ImGuiCol_ButtonActive]  = ImVec4{0.15f, 0.1505f, 0.151f, 1.0f};

				// Frame BG
				colors[ImGuiCol_FrameBg]        = ImVec4{0.2f, 0.205f, 0.21f, 1.0f};
				colors[ImGuiCol_FrameBgHovered] = ImVec4{0.3f, 0.305f, 0.31f, 1.0f};
				colors[ImGuiCol_FrameBgActive]  = ImVec4{0.15f, 0.1505f, 0.151f, 1.0f};

				// Tabs
				colors[ImGuiCol_Tab]                = ImVec4{0.15f, 0.1505f, 0.151f, 1.0f};
				colors[ImGuiCol_TabHovered]         = ImVec4{0.38f, 0.3805f, 0.381f, 1.0f};
				colors[ImGuiCol_TabActive]          = ImVec4{0.28f, 0.2805f, 0.281f, 1.0f};
				colors[ImGuiCol_TabUnfocused]       = ImVec4{0.15f, 0.1505f, 0.151f, 1.0f};
				colors[ImGuiCol_TabUnfocusedActive] = ImVec4{0.2f, 0.205f, 0.21f, 1.0f};

				// Title
				colors[ImGuiCol_TitleBg]          = ImVec4{0.15f, 0.1505f, 0.151f, 1.0f};
				colors[ImGuiCol_TitleBgActive]    = ImVec4{0.15f, 0.1505f, 0.151f, 1.0f};
				colors[ImGuiCol_TitleBgCollapsed] = ImVec4{0.15f, 0.1505f, 0.151f, 1.0f};
				break;
			case 4:
				// General window settings
				style.WindowRounding    = 5.0f;
				style.FrameRounding     = 5.0f;
				style.ScrollbarRounding = 5.0f;
				style.GrabRounding      = 5.0f;
				style.TabRounding       = 5.0f;
				style.WindowBorderSize  = 1.0f;
				style.FrameBorderSize   = 1.0f;
				style.PopupBorderSize   = 1.0f;
				style.PopupRounding     = 5.0f;

				// Setting the colors
				colors[ImGuiCol_Text]                 = ImVec4(0.95f, 0.95f, 0.95f, 1.00f);
				colors[ImGuiCol_TextDisabled]         = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
				colors[ImGuiCol_WindowBg]             = ImVec4(0.13f, 0.13f, 0.13f, 1.00f);
				colors[ImGuiCol_ChildBg]              = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
				colors[ImGuiCol_PopupBg]              = ImVec4(0.18f, 0.18f, 0.18f, 1.f);
				colors[ImGuiCol_Border]               = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
				colors[ImGuiCol_BorderShadow]         = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
				colors[ImGuiCol_FrameBg]              = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
				colors[ImGuiCol_FrameBgHovered]       = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
				colors[ImGuiCol_FrameBgActive]        = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
				colors[ImGuiCol_TitleBg]              = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
				colors[ImGuiCol_TitleBgActive]        = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
				colors[ImGuiCol_TitleBgCollapsed]     = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
				colors[ImGuiCol_MenuBarBg]            = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
				colors[ImGuiCol_ScrollbarBg]          = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
				colors[ImGuiCol_ScrollbarGrab]        = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
				colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
				colors[ImGuiCol_ScrollbarGrabActive]  = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);

				// Accent colors changed to darker olive-green/grey shades
				colors[ImGuiCol_CheckMark]          = ImVec4(0.45f, 0.45f, 0.45f, 1.00f); // Dark gray for check marks
				colors[ImGuiCol_SliderGrab]         = ImVec4(0.45f, 0.45f, 0.45f, 1.00f); // Dark gray for sliders
				colors[ImGuiCol_SliderGrabActive]   = ImVec4(0.50f, 0.50f, 0.50f, 1.00f); // Slightly lighter gray when active
				colors[ImGuiCol_Button]             = ImVec4(0.25f, 0.25f, 0.25f, 1.00f); // Button background (dark gray)
				colors[ImGuiCol_ButtonHovered]      = ImVec4(0.30f, 0.30f, 0.30f, 1.00f); // Button hover state
				colors[ImGuiCol_ButtonActive]       = ImVec4(0.35f, 0.35f, 0.35f, 1.00f); // Button active state
				colors[ImGuiCol_Header]             = ImVec4(0.40f, 0.40f, 0.40f, 1.00f); // Dark gray for menu headers
				colors[ImGuiCol_HeaderHovered]      = ImVec4(0.45f, 0.45f, 0.45f, 1.00f); // Slightly lighter on hover
				colors[ImGuiCol_HeaderActive]       = ImVec4(0.50f, 0.50f, 0.50f, 1.00f); // Lighter gray when active
				colors[ImGuiCol_Separator]          = ImVec4(0.30f, 0.30f, 0.30f, 1.00f); // Separators in dark gray
				colors[ImGuiCol_SeparatorHovered]   = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);
				colors[ImGuiCol_SeparatorActive]    = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
				colors[ImGuiCol_ResizeGrip]         = ImVec4(0.45f, 0.45f, 0.45f, 1.00f); // Resize grips in dark gray
				colors[ImGuiCol_ResizeGripHovered]  = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
				colors[ImGuiCol_ResizeGripActive]   = ImVec4(0.55f, 0.55f, 0.55f, 1.00f);
				colors[ImGuiCol_Tab]                = ImVec4(0.18f, 0.18f, 0.18f, 1.00f); // Tabs background
				colors[ImGuiCol_TabHovered]         = ImVec4(0.40f, 0.40f, 0.40f, 1.00f); // Darker gray on hover
				colors[ImGuiCol_TabActive]          = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
				colors[ImGuiCol_TabUnfocused]       = ImVec4(0.18f, 0.18f, 0.18f, 1.00f);
				colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
				colors[ImGuiCol_DockingPreview]     = ImVec4(0.45f, 0.45f, 0.45f, 1.00f); // Docking preview in gray
				colors[ImGuiCol_DockingEmptyBg]     = ImVec4(0.18f, 0.18f, 0.18f, 1.00f); // Empty dock background
				// Additional styles
				style.FramePadding  = ImVec2(8.0f, 4.0f);
				style.ItemSpacing   = ImVec2(8.0f, 4.0f);
				style.IndentSpacing = 20.0f;
				style.ScrollbarSize = 16.0f;
				break;
		}
	}


	void UIManager::onShutdown()
	{
	}

	std::shared_ptr<Texture> audioTexture;

	void UIManager::onInit()
	{
		SetThemeColors(0);
		drawFuncs[typeid(Audio::SoundBuffer)] = [this]() { DrawSoundAssets(); };
		drawFuncs[typeid(Texture)]            = [this]() { DrawTextureAssets(); };
		drawFuncs[typeid(Rendering::Model)]   = [this]() { DrawModelAssets(); };

		audioTexture = std::make_shared<Texture>();
		audioTexture->LoadFromFile("resources/engine/speaker.png");
	}

	int selectedTheme = 0;

	void UIManager::DrawTopBar()
	{
		if (ImGui::BeginMainMenuBar()) {
			if (ImGui::BeginMenu("Themes")) {
				if (ImGui::MenuItem("Bess Dark", nullptr, selectedTheme == 0)) selectedTheme = 0;
				if (ImGui::MenuItem("Catpuccin Mocha", nullptr, selectedTheme == 1)) selectedTheme = 1;
				if (ImGui::MenuItem("Modern Dark", nullptr, selectedTheme == 2)) selectedTheme = 2;
				if (ImGui::MenuItem("Dark Theme", nullptr, selectedTheme == 3)) selectedTheme = 3;
				if (ImGui::MenuItem("Fluent UI", nullptr, selectedTheme == 4)) selectedTheme = 4;
				ImGui::EndMenu();
			}

			ImGui::EndMainMenuBar();
		}
	}


	void UIManager::BeginDockspace()
	{
		// 1. Draw top menu bar *before* dockspace
		DrawTopBar();

		// 2. Apply theme colors
		SetThemeColors(selectedTheme);

		// 3. Setup full-screen window for dockspace
		ImGuiWindowFlags windowFlags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;

		const ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(viewport->Pos);
		ImGui::SetNextWindowSize(viewport->Size);
		ImGui::SetNextWindowViewport(viewport->ID);

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

		windowFlags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

		bool dockspaceOpen = true;
		ImGui::Begin("Dockspace", &dockspaceOpen, windowFlags);
		ImGui::PopStyleVar(3); // Pop all three style vars

		// 4. Create the dockspace
		ImGuiID dockspaceID = ImGui::GetID("Dockspace");
		ImGui::DockSpace(dockspaceID, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);
	}

	void UIManager::EndDockspace()
	{
		ImGui::End();
	}

	void UIManager::onUpdate(float dt)
	{
		GetRenderer().PreRender();
		BeginDockspace();

		RenderSceneView(Engine::Window::GetFramebuffer(Window::FramebufferID::GAME_OUT)->texture);
		RenderHierarchyWindow();
		RenderInspectorWindow();
		RenderAnimationWindow();
		RenderAssetWindow();
		// RenderAudioDebugUI();

		// Display pause overlay when physics is disabled
		if (GetPhysics().isPhysicsPaused) {
			RenderPauseOverlay();
		}

		EndDockspace();
	}

	void UIManager::RenderHierarchyWindow()
	{
		ImGui::Begin("Hierarchy");

		// Get all entities with EntityMetadata component
		auto view = GetRegistry().view<Components::EntityMetadata>();

		for (auto entity : view) {
			Entity e(entity);
			auto&  metadata = e.GetComponent<Components::EntityMetadata>();

			// Create a selectable item for each entity
			bool isSelected = (m_selectedEntity == e);

			std::string title = metadata.name + "##" + std::to_string((int) entity);
			if (ImGui::Selectable(title.c_str(), isSelected)) {
				m_selectedEntity = e;
			}
		}
		if (ImGui::BeginPopupContextWindow()) {
			if (ImGui::MenuItem("Create Entity")) {
				// Action for menu item
				Entity entity    = Entity::Create("New Entity");
				m_selectedEntity = entity;
			}

			ImGui::EndPopup();
		}


		ImGui::End();
	}

	void UIManager::RenderInspectorWindow()
	{
		ImGui::Begin("Inspector");

		if (m_selectedEntity) {
			// Display entity name at the top
			auto& metadata = m_selectedEntity.GetComponent<Components::EntityMetadata>();
			ImGui::Text("Selected: %s", metadata.name.c_str());
			ImGui::Separator();

			// Render each component in the inspector
			if (m_selectedEntity.HasComponent<Components::EntityMetadata>()) {
				if (ImGui::CollapsingHeader(ICON_FA_ID_CARD " Entity Metadata", ImGuiTreeNodeFlags_DefaultOpen)) {
					m_selectedEntity.GetComponent<Components::EntityMetadata>().RenderInspector(m_selectedEntity);
				}
			}

			if (m_selectedEntity.HasComponent<Components::Transform>()) {
				if (ImGui::CollapsingHeader(ICON_FA_MAXIMIZE " Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
					m_selectedEntity.GetComponent<Components::Transform>().RenderInspector(m_selectedEntity);
				}
			}

			if (m_selectedEntity.HasComponent<Components::ModelRenderer>()) {
				if (ImGui::CollapsingHeader(ICON_FA_CUBE " Model Renderer")) {
					m_selectedEntity.GetComponent<Components::ModelRenderer>().RenderInspector(m_selectedEntity);
				}
			}

			if (m_selectedEntity.HasComponent<Components::RigidBodyComponent>()) {
				if (ImGui::CollapsingHeader(ICON_FA_CUBES_STACKED " Rigid Body")) {
					m_selectedEntity.GetComponent<Components::RigidBodyComponent>().RenderInspector(m_selectedEntity);
				}
			}

			if (m_selectedEntity.HasComponent<Components::AudioSource>()) {
				if (ImGui::CollapsingHeader(ICON_FA_VOLUME_HIGH " Audio Source")) {
					auto& audioSource = m_selectedEntity.GetComponent<Components::AudioSource>();
					audioSource.RenderInspector(m_selectedEntity);

					// Handle the Play button functionality here since we have access to the sound manager
					//					if (!audioSource.isPlaying && ImGui::Button("Play")) {
					//						if (audioSource.source && !audioSource.soundName.empty()) {
					//							audioSource.Play(GetSoundManager());
					//							spdlog::error("Playing sound");
					//						}
					//					}
				}
			}

			if (m_selectedEntity.HasComponent<Components::SkeletonComponent>()) {
				if (ImGui::CollapsingHeader("Skeleton")) {
					m_selectedEntity.GetComponent<Components::SkeletonComponent>().RenderInspector(m_selectedEntity);
				}
			}

			if (m_selectedEntity.HasComponent<Components::AnimationComponent>()) {
				if (ImGui::CollapsingHeader("Animation")) {
					m_selectedEntity.GetComponent<Components::AnimationComponent>().RenderInspector(m_selectedEntity);
				}
			}

			if (m_selectedEntity.HasComponent<Components::AnimationPoseComponent>()) {
				if (ImGui::CollapsingHeader("Animation Pose")) {
					m_selectedEntity.GetComponent<Components::AnimationPoseComponent>().RenderInspector(m_selectedEntity);
				}
			}

			if (m_selectedEntity.HasComponent<Components::AnimationWorkerComponent>()) {
				if (ImGui::CollapsingHeader("Animation Context")) {
					m_selectedEntity.GetComponent<Components::AnimationWorkerComponent>().RenderInspector(m_selectedEntity);
				}
			}

			if (m_selectedEntity.HasComponent<Components::SkinnedMeshComponent>()) {
				if (ImGui::CollapsingHeader("Skinned Mesh")) {
					m_selectedEntity.GetComponent<Components::SkinnedMeshComponent>().RenderInspector(m_selectedEntity);
				}
			}
			if (m_selectedEntity.HasComponent<Components::ParticleSystem>()) {
				if (ImGui::CollapsingHeader(ICON_FA_STAR_HALF_STROKE "Particle System")) {
					m_selectedEntity.GetComponent<Components::ParticleSystem>().RenderInspector(m_selectedEntity);
				}
			}
			if (m_selectedEntity.HasComponent<Components::ShadowCaster>()) {
				if (ImGui::CollapsingHeader(ICON_FA_MOON " Shadow Caster")) {
					m_selectedEntity.GetComponent<Components::ShadowCaster>().RenderInspector(m_selectedEntity);
				}
			}
			if (m_selectedEntity.HasComponent<Components::LuaScript>()) {
				if (ImGui::CollapsingHeader(ICON_FA_SCROLL " Script")) {
					m_selectedEntity.GetComponent<Components::LuaScript>().RenderInspector(m_selectedEntity);
				}
			}
			if (m_selectedEntity.HasComponent<Components::TerrainRenderer>()) {
				if (ImGui::CollapsingHeader(ICON_FA_MAP " Terrain Renderer")) {
					m_selectedEntity.GetComponent<Components::TerrainRenderer>().RenderInspector(m_selectedEntity);
				}
			}
		}
		else {
			ImGui::Text("No entity selected");
		}

		ImGui::End();
	}

	void UIManager::RenderAnimationWindow()
	{
		ImGui::Begin("Animation");

		if (ImGui::CollapsingHeader("Animation")) {
			//						auto& animManager = GetAnimationManager();
			//
			//
			//						auto& skeleton = animManager.GetSkeleton();
			//						auto& meshes   = animManager.GetMeshes();
			//
			//						ImGui::LabelText("Animated Joints", "%d animated joints", skeleton.num_joints());
			//
			//						int influences = 0;
			//						for (const auto& mesh : meshes) {
			//							influences = ozz::math::Max(influences, mesh.max_influences_count());
			//						}
			//						ImGui::LabelText("Influences", "%d influences (max)", influences);
			//
			//						int vertices = 0;
			//						for (const auto& mesh : meshes) {
			//							vertices += mesh.vertex_count();
			//						}
			//
			//						ImGui::LabelText("Vertices", "%.1fK vertices", vertices / 1000.f);
			//
			//						int indices = 0;
			//						for (const auto& mesh : meshes) {
			//							indices += mesh.triangle_index_count();
			//						}
			//
			//						ImGui::LabelText("Triangles", "%.1fK triangles", indices / 3000.f);
		}

		if (ImGui::CollapsingHeader("Rendering Options")) {
			auto& animManager    = GetAnimationManager();
			auto& draw_skeleton  = animManager.GetDrawSkeleton();
			auto& draw_mesh      = animManager.GetDrawMesh();
			auto& render_options = animManager.GetRenderOptions();

			ImGui::Checkbox("Show Skeleton", &draw_skeleton);
			ImGui::Checkbox("Show Mesh", &draw_mesh);
			ImGui::Separator();

			ImGui::Checkbox("Show triangles", &render_options.triangles);
			ImGui::Checkbox("Show texture", &render_options.texture);
			ImGui::Checkbox("Show vertices", &render_options.vertices);
			ImGui::Checkbox("Show normals", &render_options.normals);
			ImGui::Checkbox("Show tangents", &render_options.tangents);
			ImGui::Checkbox("Show binormals", &render_options.binormals);
			ImGui::Checkbox("Show colors", &render_options.colors);
			ImGui::Checkbox("Wireframe", &render_options.wireframe);
			ImGui::Checkbox("Skip skinning", &render_options.skip_skinning);
		}

		ImGui::SliderFloat("fov", &GetCamera().m_fov, 45, 120);

		ImGui::End();
	}

	void UIManager::RenderAudioDebugUI()
	{
		ImGui::Begin("Audio Debug");
		auto      audioView = GetRegistry().view<Components::EntityMetadata, Components::Transform, Components::AudioSource>();
		glm::vec3 cameraPos = GetCamera().GetPosition();

		for (auto [entity, metadata, transform, audio] : audioView.each()) {
			if (audio.source) {
				float distance = glm::distance(cameraPos, transform.position);
				ImGui::Text("Entity: %s", metadata.name.c_str());
				ImGui::Text("Distance: %.2f units", distance);

				// Add sliders to adjust audio parameters
				bool paramsChanged = false;

				paramsChanged |= ImGui::SliderFloat("Volume", &audio.volume, 0.0f, 1.0f);
				if (paramsChanged) {
					audio.source->SetGain(audio.volume);
				}

				paramsChanged = false;
				paramsChanged |= ImGui::SliderFloat("Reference Distance", &audio.referenceDistance, 0.1f, 20.0f);
				paramsChanged |= ImGui::SliderFloat("Max Distance", &audio.maxDistance, 10.0f, 100.0f);
				paramsChanged |= ImGui::SliderFloat("Rolloff Factor", &audio.rolloffFactor, 0.1f, 5.0f);

				if (paramsChanged) {
					audio.source->ConfigureAttenuation(audio.referenceDistance, audio.maxDistance, audio.rolloffFactor);
				}

				// Calculate linear attenuation for display (matching OpenAL's AL_LINEAR_DISTANCE_CLAMPED)
				float attenuation = 1.0f;

				if (distance <= audio.referenceDistance) {
					// Within reference distance - full volume
					attenuation = 1.0f;
				}
				else if (distance >= audio.maxDistance) {
					// Beyond max distance - silent
					attenuation = 0.0f;
				}
				else {
					// Linear interpolation between reference and max distance
					attenuation = 1.0f - ((distance - audio.referenceDistance) / (audio.maxDistance - audio.referenceDistance));

					// Apply rolloff factor
					attenuation = pow(attenuation, audio.rolloffFactor);
				}

				ImGui::Text("Estimated Attenuation: %.3f", attenuation);
				ImGui::Text("Estimated Volume: %.3f", audio.volume * attenuation);

				// Add a visual representation of the attenuation
				ImGui::Text("Attenuation:");
				ImGui::SameLine();
				ImGui::ProgressBar(attenuation, ImVec2(100, 10));

				ImGui::Separator();
			}
		}
		ImGui::End();
	}

	void UIManager::RenderPauseOverlay()
	{
		ImGuiIO& io = ImGui::GetIO();
		// ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x - 10, 10), ImGuiCond_Always, ImVec2(1.0f, 0.0f));

		ImGui::SetNextWindowPos(ImVec2(GetWindow().targetX + GetWindow().targetWidth - 10, GetWindow().targetY + 10), ImGuiCond_Always, ImVec2(1.0f, 0.0f));
		ImGui::SetNextWindowBgAlpha(0.35f);
		ImGui::Begin("PauseOverlay", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove);
		ImGui::Text("PHYSICS PAUSED");
		ImGui::Text("Press P to resume");
		ImGui::End();
	}
	void UIManager::RenderSceneView(GLuint texId)
	{
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

		bool open = true;
		ImGui::Begin("Viewport", &open);

		ImVec2 sizeOut = ImGui::GetContentRegionAvail();
		bool   focus   = ImGui::IsWindowFocused();

		float aspect = (float) sizeOut.y / (float) sizeOut.x;

		float width, height;
		float offsetX = 0.0f, offsetY = 0.0f;

		if (sizeOut.y / sizeOut.x > aspect) {
			width   = sizeOut.x;
			height  = width * aspect;
			offsetY = (sizeOut.y - height) / 2.0f;
			ImGui::SetCursorPosY(ImGui::GetCursorPosY() + offsetY);
		}
		else {
			height  = sizeOut.y;
			width   = height / aspect;
			offsetX = (sizeOut.x - width) / 2.0f;
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offsetX);
		}

		ImVec2 topLeft = ImGui::GetCursorScreenPos(); // Now it's correct

		GLuint tex = texId; // Engine::Window::GetFramebuffer(Window::FramebufferID::GAME_OUT)->texture;
		ImGui::Image((ImTextureID) tex, ImVec2(width, height), ImVec2(0, 1), ImVec2(1, 0));
		// Now the topLeft is the actual top-left of the displayed image
		GetWindow().UpdateViewportSize(width, height, topLeft.x, topLeft.y);

		ImGui::PopStyleVar();
		ImGui::End();
	}

	float m_iconSize = 128.0f;

	void UIManager::RenderAssetWindow()
	{
		ImGui::Begin("Assets");

		ImGui::SliderFloat("Icon Size", &m_iconSize, 16.0f, 256.0f, "%.0f", ImGuiSliderFlags_AlwaysClamp);

		ImGuiWindow* window = ImGui::GetCurrentWindow();
		if (window && ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup)) {
			if (ImGui::GetIO().KeyCtrl) {
				float scroll = ImGui::GetIO().MouseWheel;
				if (scroll != 0.0f) {
					m_iconSize += scroll * 8.0f;
					m_iconSize = std::clamp(m_iconSize, 16.0f, 256.0f);
				}
			}
		}


		if (ImGui::BeginTabBar("AssetTabs")) {
			for (auto& [type, drawFn] : drawFuncs) {
				const char* label = type.name(); // fallback label
				if (type == typeid(Texture))
					label = "Textures";
				else if (type == typeid(Rendering::Model))
					label = "Models";
				else if (type == typeid(Audio::SoundBuffer))
					label = "Sounds";
				// Add custom labels per type

				if (ImGui::BeginTabItem(label)) {
					ImGui::BeginChild("AssetScroll", ImVec2(0, 0), false, ImGuiWindowFlags_AlwaysVerticalScrollbar);
					drawFn();
					ImGui::EndChild();
					ImGui::EndTabItem();
				}
			}
			ImGui::EndTabBar();
		}

		ImGui::End();
	}

	void UIManager::DrawSoundAssets()
	{
		auto& storage = GetAssetManager().GetStorage<Audio::SoundBuffer>();

		float padding     = 8.0f;
		int   columnCount = static_cast<int>(ImGui::GetContentRegionAvail().x / (m_iconSize + padding));
		if (columnCount < 1) columnCount = 1;

		ImGui::Columns(columnCount, nullptr, false);

		for (auto& [id, soundPtr] : storage.guidToAsset) {
			if (!soundPtr) continue;
			ImGui::PushID(("tex" + id).c_str());


			ImGui::Image(audioTexture->GetID(), ImVec2(m_iconSize, m_iconSize));
			ImGui::TextWrapped("Sound Buffer %s", id.c_str());

			ImGui::NextColumn();
			ImGui::PopID();
		}

		ImGui::Columns(1);
	}

	void UIManager::DrawTextureAssets()
	{
		auto& storage = GetAssetManager().GetStorage<Texture>();

		float padding     = 8.0f;
		int   columnCount = static_cast<int>(ImGui::GetContentRegionAvail().x / (m_iconSize + padding));
		if (columnCount < 1) columnCount = 1;

		ImGui::Columns(columnCount, nullptr, false);

		for (auto& [id, texPtr] : storage.guidToAsset) {
			if (!texPtr) continue;
			ImGui::PushID(("tex" + id).c_str());

			ImGui::Image(reinterpret_cast<void*>(static_cast<intptr_t>(texPtr->GetID())), ImVec2(m_iconSize, m_iconSize));
			ImGui::TextWrapped("%s", texPtr->GetName().c_str());

			ImGui::NextColumn();
			ImGui::PopID();
		}

		ImGui::Columns(1);
	}


	void UIManager::DrawModelAssets()
	{
		auto& storage = GetAssetManager().GetStorage<Rendering::Model>();

		float padding     = 8.0f;
		int   columnCount = static_cast<int>(ImGui::GetContentRegionAvail().x / (m_iconSize + padding));
		if (columnCount < 1) columnCount = 1;

		ImGui::Columns(columnCount, nullptr, false);

		for (auto& [id, model] : storage.guidToAsset) {
			if (!model) continue;
			ImGui::PushID(("model" + id).c_str());

			auto& preview  = m_modelPreviews[id];
			preview.width  = static_cast<int>(MODEL_PREVIEW_SIZE);
			preview.height = static_cast<int>(MODEL_PREVIEW_SIZE);
			preview.Render(model.get(), GetRenderer().GetModelPreviewShader());

			ImGui::Image(reinterpret_cast<void*>(static_cast<intptr_t>(preview.texture)),
			             ImVec2(m_iconSize, m_iconSize),
			             ImVec2(0, 1), // top-left
			             ImVec2(1, 0)  // bottom-right
			);
			ImGui::TextWrapped("Model #%s", id.c_str());

			ImGui::NextColumn();
			ImGui::PopID();
		}

		ImGui::Columns(1);
	}


} // namespace Engine::UI
