#pragma once

#include <cstddef>
#include <functional>
#include <typeinfo>
#include <typeindex>
#include <unordered_map>
#include <utility>
#include <vector>

namespace defender
{

/// Type-based publish / subscribe, to decouple systems
/// (e.g. Enemy emits EnemyKilled, the HUD listens to it).
class EventBus
{
public:
    using HandlerId = std::size_t;

    static EventBus& instance()
    {
        static EventBus bus;
        return bus;
    }

    EventBus(const EventBus&) = delete;
    EventBus& operator=(const EventBus&) = delete;
    EventBus(EventBus&&) = delete;
    EventBus& operator=(EventBus&&) = delete;
    ~EventBus() = default;

    template <typename Event>
    HandlerId subscribe(std::function<void(const Event&)> handler)
    {
        const HandlerId id = nextId_++;
        handlers_[std::type_index(typeid(Event))].emplace_back(
            id, [handler = std::move(handler)](const void* event)
            { handler(*static_cast<const Event*>(event)); });
        return id;
    }

    void unsubscribe(HandlerId id)
    {
        for (auto& [type, list] : handlers_)
        {
            std::erase_if(list, [id](const auto& entry) { return entry.first == id; });
        }
    }

    template <typename Event>
    void emit(const Event& event)
    {
        const auto it = handlers_.find(std::type_index(typeid(Event)));
        if (it == handlers_.end())
        {
            return;
        }
        // Copy: a handler may subscribe or unsubscribe while we iterate.
        const auto list = it->second;
        for (const auto& [id, callback] : list)
        {
            callback(&event);
        }
    }

private:
    EventBus() = default;

    using Callback = std::function<void(const void*)>;
    std::unordered_map<std::type_index, std::vector<std::pair<HandlerId, Callback>>> handlers_;
    HandlerId nextId_ = 0;
};

}  // namespace defender
