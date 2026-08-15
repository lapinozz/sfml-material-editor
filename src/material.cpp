#include "mls/material.hpp"

#include "mls/base64.hpp"
#include "mls/serializer.hpp"

#include <fstream>

void serialize(Serializer& s, MaterialTemplate& m)
{
    s.serialize("parameters", m.parameters);
    s.serialize("vertexShader", m.vertexSrc);
    s.serialize("fragmentShader", m.fragmentSrc);
    s.serialize("parameterToTextureReference", m.parameterToTextureReference);
}

void MaterialRepo::merge(MaterialRepo&& other)
{
    ownedTextures.reserve(ownedTextures.size() + other.ownedTextures.size());
    for (auto& texturePtr : other.ownedTextures)
    {
        ownedTextures.push_back(std::move(texturePtr));
    }
    other.ownedTextures.clear();

    for (auto& [id, materialTemplate] : other.templates)
    {
        const auto it = templates.find(id);
        if (it != templates.end())
        {
            it->second.merge(std::move(materialTemplate));
        }
        else
        {
            templates.emplace(id, std::move(materialTemplate));
        }
    }
}

void MaterialRepo::update()
{
    update(clock.restart());
}

void MaterialRepo::update(sf::Time deltaTime)
{
    update(deltaTime, deltaTime);
}

void MaterialRepo::update(sf::Time deltaTime, sf::Time realDeltaTime)
{
    time += deltaTime;
    realTime += realDeltaTime;

    for (auto& [_, materialTemplate] : templates)
    {
        materialTemplate.update(time, realTime);
    }
}

std::optional<MaterialRepo> MaterialRepo::loadFromFile(const std::string& path,
                                                       const TextureLoadingCallback& textureLoadingCallback)
{
    std::ifstream inputFile(path, std::ios_base::binary);
    if (!inputFile)
    {
        return std::nullopt;
    }

    std::string fileContent{(std::istreambuf_iterator<char>(inputFile)), std::istreambuf_iterator<char>()};

    try
    {
        auto j = json::parse(fileContent);
        Serializer s(false, j);

        MaterialRepo repo;
        repo.serialize(s);

        std::unordered_map<std::string, TextureReference> textureReferences;
        s.serialize("textureReferences", textureReferences);

        std::unordered_map<std::string, const sf::Texture*> loadedTextures;

        for (const auto& [textureId, textureRef] : textureReferences)
        {
            if (textureLoadingCallback)
            {
                loadedTextures[textureId] = textureLoadingCallback(textureRef);
            }
            else
            {
                repo.ownedTextures.emplace_back(std::make_unique<sf::Texture>(defaultTextureLoader(textureRef)));
                loadedTextures[textureId] = repo.ownedTextures.back().get();
            }
        }

        for (auto& [_, material] : repo.templates)
        {
            for (const auto& [paramId, textureId] : material.parameterToTextureReference)
            {
                if (auto it = loadedTextures.find(textureId); it != loadedTextures.end())
                {
                    material.setParameterDefault(paramId, it->second);
                }
            }

            material.parameterToTextureReference.clear();
        }

        return std::move(repo);

    } catch (...)
    {
    }

    return std::nullopt;
}

void MaterialRepo::serialize(Serializer& s)
{
    assert(!s.isSaving);

    s.serialize("materials", templates);
}

MaterialTemplate::MaterialTemplate(MaterialTemplate&& other) :
    parameters{std::move(other.parameters)},
    instances{std::move(other.instances)},
    vertexSrc{std::move(other.vertexSrc)},
    fragmentSrc{std::move(other.fragmentSrc)}
{
    for (auto instance : instances)
    {
        instance->materialTemplate = this;
    }

    rebuildInstances();
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

Material MaterialTemplate::makeInstance()
{
    return {*this};
}

void MaterialTemplate::setParameterDefault(const std::string& name, ParameterValue param)
{
    parameters[name].defaultValue = param;
    for (auto* material : instances)
    {
        material->onDefaultChange(name, param);
    }
}

void MaterialTemplate::update(sf::Time currentTime, sf::Time currentRealTime)
{
    time = currentTime;
    realTime = currentRealTime;

    setParameterDefault("time", time.asSeconds());
    setParameterDefault("realTime", realTime.asSeconds());
}

void MaterialTemplate::merge(MaterialTemplate&& other)
{
    parameters = std::move(other.parameters);

    for (auto instance : other.instances)
    {
        instance->materialTemplate = this;
    }

    instances.insert(instances.end(), other.instances.begin(), other.instances.end());
    other.instances.clear();

    vertexSrc = std::move(other.vertexSrc);
    fragmentSrc = std::move(other.fragmentSrc);

    rebuildInstances();
}

Material::operator const sf::Shader*() const
{
    return &shader;
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

void Material::update(sf::Time currentTime)
{
    update(currentTime, currentTime);
}

void Material::update(sf::Time currentTime, sf::Time currentRealTime)
{
    setUniform("time", currentTime.asSeconds());
    setUniform("realTime", currentRealTime.asSeconds());
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
