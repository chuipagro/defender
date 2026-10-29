#pragma once

#include <SFML/Audio/SoundBuffer.hpp>
#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/Texture.hpp>

#include <string>
#include <unordered_map>

namespace defender
{

/// Asset ids are paths relative to assets/ (e.g. "sprites/tiles/ice.png").
using TextureId = std::string;
using FontId = std::string;
using SoundId = std::string;

/// Loads each asset once and keeps it alive for the whole game.
class ResourceManager
{
public:
    static ResourceManager& instance();

    ResourceManager(const ResourceManager&) = delete;
    ResourceManager& operator=(const ResourceManager&) = delete;
    ResourceManager(ResourceManager&&) = delete;
    ResourceManager& operator=(ResourceManager&&) = delete;
    ~ResourceManager() = default;

    sf::Texture& getTexture(const TextureId& id);
    sf::Font& getFont(const FontId& id);
    sf::SoundBuffer& getSoundBuffer(const SoundId& id);

private:
    ResourceManager() = default;

    std::unordered_map<TextureId, sf::Texture> textures_;
    std::unordered_map<FontId, sf::Font> fonts_;
    std::unordered_map<SoundId, sf::SoundBuffer> sounds_;
};

}  // namespace defender
