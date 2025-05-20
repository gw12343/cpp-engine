#pragma once

#include "Texture.h"

#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <unordered_map>

namespace Engine {
	class Material {
	  public:
		Material();
		~Material() = default;

		// Texture setters
		void SetDiffuseTexture(const std::shared_ptr<Texture>& texture);
		void SetSpecularTexture(const std::shared_ptr<Texture>& texture);
		void SetNormalTexture(const std::shared_ptr<Texture>& texture);
		void SetHeightTexture(const std::shared_ptr<Texture>& texture);

		// Color setters
		void SetDiffuseColor(const glm::vec3& color);
		void SetSpecularColor(const glm::vec3& color);
		void SetAmbientColor(const glm::vec3& color);
		void SetEmissiveColor(const glm::vec3& color);
		void SetShininess(float shininess);

		// Getters
		std::shared_ptr<Texture> GetDiffuseTexture() const { return m_diffuseTexture; }
		std::shared_ptr<Texture> GetSpecularTexture() const { return m_specularTexture; }
		std::shared_ptr<Texture> GetNormalTexture() const { return m_normalTexture; }
		std::shared_ptr<Texture> GetHeightTexture() const { return m_heightTexture; }

		glm::vec3 GetDiffuseColor() const { return m_diffuseColor; }
		glm::vec3 GetSpecularColor() const { return m_specularColor; }
		glm::vec3 GetAmbientColor() const { return m_ambientColor; }
		glm::vec3 GetEmissiveColor() const { return m_emissiveColor; }
		float     GetShininess() const { return m_shininess; }

		// Material name
		void               SetName(const std::string& name) { m_name = name; }
		const std::string& GetName() const { return m_name; }

	  private:
		// Textures
		std::shared_ptr<Texture> m_diffuseTexture;
		std::shared_ptr<Texture> m_specularTexture;
		std::shared_ptr<Texture> m_normalTexture;
		std::shared_ptr<Texture> m_heightTexture;

		// Colors
		glm::vec3 m_diffuseColor;
		glm::vec3 m_specularColor;
		glm::vec3 m_ambientColor;
		glm::vec3 m_emissiveColor;
		float     m_shininess;

		// Material name
		std::string m_name;
	};
} // namespace Engine