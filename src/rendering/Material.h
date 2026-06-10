#pragma once

#include "Texture.h"





#include <unordered_map>

namespace Engine {
	class Material {
	  public:
		Material();
		~Material() = default;

		// Texture setters
		void SetDiffuseTexture(TextureHandle texture);
		void SetSpecularTexture(TextureHandle texture);
		void SetNormalTexture(TextureHandle texture);
		void SetHeightTexture(TextureHandle texture);

		// Color setters
		void SetDiffuseColor(const glm::vec3& color);
		void SetSpecularColor(const glm::vec3& color);
		void SetAmbientColor(const glm::vec3& color);
		void SetEmissiveColor(const glm::vec3& color);
		void SetShininess(float shininess);

		// Getters
		[[nodiscard]] TextureHandle                  GetDiffuseTexture() const { return m_diffuseTexture; }
		[[nodiscard]] TextureHandle                  GetSpecularTexture() const { return m_specularTexture; }
		[[nodiscard]] TextureHandle                  GetNormalTexture() const { return m_normalTexture; }
		[[maybe_unused]] [[nodiscard]] TextureHandle GetHeightTexture() const { return m_heightTexture; }

		[[nodiscard]] glm::vec3 GetDiffuseColor() const { return m_diffuseColor; }
		[[nodiscard]] glm::vec3 GetSpecularColor() const { return m_specularColor; }
		[[nodiscard]] glm::vec3 GetAmbientColor() const { return m_ambientColor; }
		[[nodiscard]] glm::vec3 GetEmissiveColor() const { return m_emissiveColor; }
		[[nodiscard]] float     GetShininess() const { return m_shininess; }

		[[nodiscard]] const glm::vec2& GetTextureScale() const;
		void                           SetTextureScale(const glm::vec2& textureScale);

		// Material name
		void                             SetName(const std::string& name) { m_name = name; }
		[[nodiscard]] const std::string& GetName() const { return m_name; }

		std::string m_path;

	  private:
		friend class MaterialEditor;

		// Textures
		TextureHandle m_diffuseTexture;
		TextureHandle m_specularTexture;
		TextureHandle m_normalTexture;
		TextureHandle m_heightTexture;

		// Colors
		glm::vec3 m_diffuseColor;
		glm::vec3 m_specularColor;
		glm::vec3 m_ambientColor;
		glm::vec3 m_emissiveColor;
		float     m_shininess;

		glm::vec2 m_textureScale = {1.0f, 1.0f};

		// Material name
		std::string m_name;
	};
} // namespace Engine