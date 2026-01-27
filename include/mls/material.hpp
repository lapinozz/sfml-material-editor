#pragma once

#include "serializer.hpp"
#include "sfml-serialization.hpp"

#include <SFML/Graphics.hpp>
#include <SFML/Graphics/Glsl.hpp>

#include <array>
#include <functional>
#include <memory>
#include <variant>
#include <vector>

using Vector4f = sf::Glsl::Vec4; //std::array<float, 4>;
using ParameterValue = std::variant<float, sf::Vector2f, sf::Vector3f, Vector4f, const sf::Texture*>;

enum class ParamterType
{
    Float,
    Vec2,
    Vec3,
    Vec4,
    Texture
};

struct Parameter
{
    ParameterValue defaultValue;
};

struct MaterialTemplate
{
    std::unordered_map<std::string, Parameter> parameters;

    std::string vertexSrc;
    std::string fragmentSrc;

    std::vector<class Material*> instances;

    void rebuildInstances();

    void setSource(std::string vertex, std::string fragment);

    std::unique_ptr<Material> makeInstance();

    void setParameterDefault(const std::string& name, ParameterValue param);
};

struct MaterialGraph : public MaterialTemplate
{
};

struct Project
{
    std::unordered_map<std::string, MaterialTemplate> materialDefinitions;
};

class Material
{
private:
    MaterialTemplate* materialTemplate{};
    std::unordered_map<std::string, ParameterValue> values;
    sf::Shader shader;

    Material() = delete;
    Material(const Material&) = delete;
    Material(Material&&) = delete;
    Material& operator=(const Material&) = delete;
    Material& operator=(Material&&) = delete;

    void setUniform(const std::string& name, ParameterValue param);

    void onDefaultChange(const std::string& name, ParameterValue param);

    void updateParameters();

public:
    using Ptr = std::unique_ptr<Material>;

    Material(MaterialTemplate& matTemplate) : materialTemplate{&matTemplate}
    {
        materialTemplate->instances.push_back(this);
    }

    ~Material()
    {
        materialTemplate->instances.erase(
            std::remove(materialTemplate->instances.begin(), materialTemplate->instances.end(), this));
    }

    void rebuild();

    const sf::Shader& getShader() const;

    void setValue(const std::string& name, ParameterValue param);

    friend MaterialTemplate;
};

struct TextureReference
{
    enum class Type
    {
        Id,
        Path,
        Embedded
    };

    std::string id;

    Type type;
    std::string data;
};
using TextureReferences = std::vector<TextureReference>;

sf::Texture defaultTextureLoader(const TextureReference& textureReference);
using TextureLoadingCallback = std::function<const sf::Texture*(const TextureReference&)>;

class MaterialRepo
{
public:
    std::vector<sf::Texture> ownedTextures;
    std::vector<const sf::Texture*> referencedTextures;
    std::unordered_map<std::string, MaterialTemplate> templates;

    std::unique_ptr<Material> makeInstance(const std::string& templateId)
    {
        return templates[templateId].makeInstance();
    }

    static std::optional<MaterialRepo> loadFromFile(std::string_view path,
                                                    const TextureLoadingCallback& textureLoadingCallback = {});
};

inline void serialize(Serializer& s, Parameter& p)
{
    s.serialize("defaultValue", p.defaultValue);
}

inline void serialize(Serializer& s, TextureReference& tr)
{
    s.serialize("id", tr.id);
    s.serialize("type", tr.type);
    s.serialize("data", tr.data);
}

// dirty hack
inline void serialize(Serializer& s, const sf::Texture* ptr)
{
    static_assert(sizeof(ptr) == sizeof(std::uint64_t));
    s.serialize((std::uint64_t&)ptr);
}