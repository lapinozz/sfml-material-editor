#pragma once

#include "mls_export.h"
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

enum class MLS_EXPORT ParamterType
{
    Float,
    Vec2,
    Vec3,
    Vec4,
    Texture
};

struct MLS_EXPORT Parameter
{
    ParameterValue defaultValue;
};

struct MLS_EXPORT MaterialTemplate
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

struct MLS_EXPORT MaterialGraph : public MaterialTemplate
{
};

struct MLS_EXPORT Project
{
    std::unordered_map<std::string, MaterialTemplate> materialDefinitions;
};

class MLS_EXPORT Material
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
    static constexpr std::string_view uniformPrefix = "P_";
    static constexpr std::string_view textureUniformSizeSuffix = "_texSize";

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

struct MLS_EXPORT TextureReference
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

MLS_EXPORT sf::Texture defaultTextureLoader(const TextureReference& textureReference);
using TextureLoadingCallback = std::function<const sf::Texture*(const TextureReference&)>;

class MLS_EXPORT MaterialRepo
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

inline MLS_EXPORT void serialize(Serializer& s, Parameter& p)
{
    s.serialize("defaultValue", p.defaultValue);
}

inline MLS_EXPORT void serialize(Serializer& s, TextureReference& tr)
{
    s.serialize("id", tr.id);
    s.serialize("type", tr.type);
    s.serialize("data", tr.data);
}

// dirty hack
inline MLS_EXPORT void serialize(Serializer& s, const sf::Texture* ptr)
{
    static_assert(sizeof(ptr) == sizeof(std::uint64_t));
    s.serialize((std::uint64_t&)ptr);
}