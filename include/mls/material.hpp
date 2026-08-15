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

class Material;
class MaterialRepo;
    
struct MLS_EXPORT MaterialTemplate
{
    MaterialTemplate() = default;
    MaterialTemplate(MaterialTemplate&& other);

    void rebuildInstances();
    void merge(MaterialTemplate&& other);

    void setSource(std::string vertex, std::string fragment);

    Material makeInstance();

    void setParameterDefault(const std::string& name, ParameterValue param);

    void update(sf::Time currentTime, sf::Time currentRealTime);

private:
    std::unordered_map<std::string, Parameter> parameters;
    std::unordered_map<std::string, std::string> parameterToTextureReference;

    std::string vertexSrc;
    std::string fragmentSrc;

    std::vector<Material*> instances;

    sf::Time time;
    sf::Time realTime;

    friend Material;
    friend MaterialRepo;
    friend class MaterialTab;
    friend void serialize(Serializer& s, MaterialTemplate& m);
};

class MLS_EXPORT Material
{
public:
    static constexpr std::string_view uniformPrefix = "P_";
    static constexpr std::string_view textureUniformSizeSuffix = "_texSize";

    Material(MaterialTemplate& matTemplate) : materialTemplate{&matTemplate}
    {
        materialTemplate->instances.push_back(this);
        rebuild();
    }

    Material(Material&& other)
    {
        *this = std::move(other);
    }

    Material(const Material& other)
    {
        *this = other;
    }

    Material& operator=(Material&& other)
    {
        if (other.materialTemplate)
        {
            materialTemplate = other.materialTemplate;
            values = std::move(other.values);
            shader = std::move(other.shader);
        }

        return *this;
    }

    Material& operator=(const Material& other)
    {
        if (other.materialTemplate)
        {
            materialTemplate = other.materialTemplate;
            values = other.values;
            rebuild();
        }

        return *this;
    }

    ~Material()
    {
        materialTemplate->instances.erase(
            std::remove(materialTemplate->instances.begin(), materialTemplate->instances.end(), this));
    }

    void rebuild();

    operator const sf::Shader*() const;
    const sf::Shader& getShader() const;

    void setValue(const std::string& name, ParameterValue param);

    void update(sf::Time currentTime);
    void update(sf::Time currentTime, sf::Time currentRealTime);

private:
    MaterialTemplate* materialTemplate{};
    std::unordered_map<std::string, ParameterValue> values;
    sf::Shader shader;

    Material() = delete;

    void setUniform(const std::string& name, ParameterValue param);

    void onDefaultChange(const std::string& name, ParameterValue param);

    void updateParameters();

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

    Type type;
    std::string data;
};

MLS_EXPORT sf::Texture defaultTextureLoader(const TextureReference& textureReference);
using TextureLoadingCallback = std::function<const sf::Texture*(const TextureReference&)>;

class MLS_EXPORT MaterialRepo
{
public:
    Material makeInstance(const std::string& templateId)
    {
        return templates[templateId].makeInstance();
    }

    [[nodiscard]] static std::optional<MaterialRepo> loadFromFile(const std::string& path,
                                                                  const TextureLoadingCallback& textureLoadingCallback = {});

    void merge(MaterialRepo&& other);

    void update();
    void update(sf::Time deltaTime);
    void update(sf::Time deltaTime, sf::Time realDeltaTime);

private:
    void serialize(Serializer& s);

    std::vector<std::unique_ptr<sf::Texture>> ownedTextures;
    std::unordered_map<std::string, MaterialTemplate> templates;

    sf::Time time;
    sf::Time realTime;

    sf::Clock clock;
};

inline MLS_EXPORT void serialize(Serializer& s, Parameter& p)
{
    s.serialize("defaultValue", p.defaultValue);
}

inline MLS_EXPORT void serialize(Serializer& s, TextureReference& tr)
{
    s.serialize("type", tr.type);
    s.serialize("data", tr.data);
}

// dirty hack
inline MLS_EXPORT void serialize(Serializer& s, const sf::Texture* ptr)
{
    static_assert(sizeof(ptr) == sizeof(std::uint64_t));
    s.serialize((std::uint64_t&)ptr);
}