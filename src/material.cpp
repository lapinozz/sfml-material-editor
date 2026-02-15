#include "mls/material.hpp"

#include "mls/base64.hpp"

std::optional<MaterialRepo> MaterialRepo::loadFromFile(std::string_view path, const TextureLoadingCallback& textureLoadingCallback)
{
    return std::optional<MaterialRepo>();
}

void MaterialTemplate::rebuildInstances()
{
    for (auto* material : instances)
    {
        material->rebuild();
    }
}

void MaterialTemplate::setSource(std::string vertex, std::string fragment)
{
    vertexSrc = vertex;
    fragmentSrc = fragment;

    rebuildInstances();
}

std::unique_ptr<Material> MaterialTemplate::makeInstance()
{
    return std::make_unique<Material>(*this);
}

void MaterialTemplate::setParameterDefault(const std::string& name, ParameterValue param)
{
    parameters[name].defaultValue = param;
    for (auto* material : instances)
    {
        material->onDefaultChange(name, param);
    }
}

void Material::setUniform(const std::string& name, ParameterValue param)
{
    const auto uniformId = std::format("{}{}", uniformPrefix, name);
    if (const sf::Texture** texture = std::get_if<const sf::Texture*>(&param))
    {
        if (*texture)
        {
            const auto sizeUniform = std::format("{}{}", uniformId, textureUniformSizeSuffix);
            shader.setUniform(uniformId, **texture);
            shader.setUniform(sizeUniform, sf::Vector2f((*texture)->getSize()));
        }
    }
    else
    {
        std::visit([&](auto value) -> void { shader.setUniform(uniformId, value); }, param);
    }
}

void Material::onDefaultChange(const std::string& name, ParameterValue param)
{
    if (!values.contains(name))
    {
        setUniform(name, param);
    }
}

void Material::updateParameters()
{
    if (materialTemplate)
    {
        for (auto& pair : materialTemplate->parameters)
        {
            const auto& name = pair.first;
            const auto& param = pair.second;

            if (!values.contains(name))
            {
                setUniform(name, param.defaultValue);
            }
        }
    }

    for (auto& pair : values)
    {
        const auto& name = pair.first;
        const auto& param = pair.second;
        setUniform(name, param);
    }
}

void Material::rebuild()
{
    if (!materialTemplate)
    {
        return;
    }

    shader.loadFromMemory(materialTemplate->vertexSrc, materialTemplate->fragmentSrc);

    updateParameters();
}

const sf::Shader& Material::getShader() const
{
    return shader;
}

void Material::setValue(const std::string& name, ParameterValue param)
{
    values[name] = param;
    setUniform(name, param);
}

sf::Texture defaultTextureLoader(const TextureReference& textureReference)
{
    sf::Texture texture;

    if (textureReference.type == TextureReference::Type::Embedded)
    {
        const std::string textureData = base64::from_base64(textureReference.data);
        texture.loadFromMemory(textureData.data(), textureData.size());
    }
    else if (textureReference.type == TextureReference::Type::Path)
    {
    }

    return texture;
}
