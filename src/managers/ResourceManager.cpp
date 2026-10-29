#include "defender/managers/ResourceManager.hpp"

#include <filesystem>
#include <stdexcept>

#ifndef DEFENDER_ASSETS_DIR
#define DEFENDER_ASSETS_DIR "assets"
#endif

namespace defender
{

namespace
{

/// assets/<id> next to the working directory first, then in the source
/// tree (useful when running from an IDE build directory).
std::string resolve(const std::string& id)
{
    for (const std::filesystem::path& base : {std::filesystem::path("assets"),
                                             std::filesystem::path(DEFENDER_ASSETS_DIR)})
    {
        if (std::filesystem::exists(base / id))
        {
            return (base / id).string();
        }
    }
    throw std::runtime_error("Missing asset: assets/" + id + " (run `make sprites`?)");
}

template <typename Resource, typename Map>
Resource& loadOnce(Map& cache, const std::string& id)
{
    if (const auto it = cache.find(id); it != cache.end())
    {
        return it->second;
    }
    Resource resource;
    if (!resource.loadFromFile(resolve(id)))
    {
        throw std::runtime_error("Cannot load asset: assets/" + id);
    }
    return cache.emplace(id, std::move(resource)).first->second;
}

}  // namespace

ResourceManager& ResourceManager::instance()
{
    static ResourceManager manager;
    return manager;
}

sf::Texture& ResourceManager::getTexture(const TextureId& id)
{
    return loadOnce<sf::Texture>(textures_, id);
}

sf::Font& ResourceManager::getFont(const FontId& id)
{
    return loadOnce<sf::Font>(fonts_, id);
}

sf::SoundBuffer& ResourceManager::getSoundBuffer(const SoundId& id)
{
    return loadOnce<sf::SoundBuffer>(sounds_, id);
}

}  // namespace defender
