#include "Material.h"

namespace Engine {
	Material::Material()
	    : m_diffuseColor(0.8f, 0.8f, 0.8f) // Default gray
	      ,
	      m_specularColor(0.5f, 0.5f, 0.5f) // Default gray
	      ,
	      m_ambientColor(0.2f, 0.2f, 0.2f) // Default dark gray
	      ,
	      m_emissiveColor(0.0f, 0.0f, 0.0f) // Default black
	      ,
	      m_shininess(32.0f) // Default shininess
	      ,
	      m_name("DefaultMaterial")
	{
	}

	void Material::SetDiffuseTexture(const std::shared_ptr<Texture>& texture)
	{
		m_diffuseTexture = texture;
	}

	void Material::SetSpecularTexture(const std::shared_ptr<Texture>& texture)
	{
		m_specularTexture = texture;
	}

	void Material::SetNormalTexture(const std::shared_ptr<Texture>& texture)
	{
		m_normalTexture = texture;
	}

	void Material::SetHeightTexture(const std::shared_ptr<Texture>& texture)
	{
		m_heightTexture = texture;
	}

	void Material::SetDiffuseColor(const glm::vec3& color)
	{
		m_diffuseColor = color;
	}

	void Material::SetSpecularColor(const glm::vec3& color)
	{
		m_specularColor = color;
	}

	void Material::SetAmbientColor(const glm::vec3& color)
	{
		m_ambientColor = color;
	}

	void Material::SetEmissiveColor(const glm::vec3& color)
	{
		m_emissiveColor = color;
	}

	void Material::SetShininess(float shininess)
	{
		m_shininess = shininess;
	}
} // namespace Engine