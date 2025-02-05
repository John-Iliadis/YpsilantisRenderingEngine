//
// Created by Gianni on 25/01/2025.
//

#ifndef VULKANRENDERINGENGINE_SIMPLE_NOTIFICATION_SERVICE_HPP
#define VULKANRENDERINGENGINE_SIMPLE_NOTIFICATION_SERVICE_HPP

#include <glm/glm.hpp>
#include "types.hpp"

class Message
{
public:
    struct ModelDeleted
    {
        uuid64_t modelID;
        std::unordered_set<uuid64_t> meshIDs;
    };

    struct MaterialDeleted
    {
        uuid64_t materialID;
        index_t removeIndex;
        std::optional<index_t> transferIndex;
    };

    struct TextureDeleted
    {
        uuid64_t textureID;
        index_t removedIndex;
        std::optional<index_t> transferIndex;
    };

    struct MeshInstanceUpdate
    {
        uuid64_t meshID;
        uuid64_t objectID;
        uint32_t instanceID;
        index_t matIndex;
        glm::mat4 transformation;
    };

    struct RemoveMeshInstance
    {
        uuid64_t meshID;
        uint32_t instanceID;
    };

    struct MaterialRemap
    {
        uint32_t newMatIndex;
        std::string matName;
    };

    std::variant<ModelDeleted,
        MaterialDeleted,
        TextureDeleted,
        MeshInstanceUpdate,
        RemoveMeshInstance,
        MaterialRemap> message;

    template<typename T>
    Message(const T& message) : message(message) {}

    template <typename T, typename... Args>
    Message(std::in_place_type_t<T>, Args&&... args)
        : message(std::in_place_type<T>, std::forward<Args>(args)...)
    {
    }

    template<typename T>
    T* getIf() { return std::get_if<T>(&message); }

    template<typename T>
    const T* getIf() const { return std::get_if<T>(&message); }

    template<typename T, typename... Args>
    static Message create(Args&&... args)
    {
        return Message(std::in_place_type<T>, std::forward<Args>(args)...);
    }
};

class SubscriberSNS;
class Topic
{
public:
    enum Type
    {
        None = 0,
        Editor,
        SceneGraph,
        Resources,
        Count
    };

public:
    Topic() = default;

    void addSubscriber(SubscriberSNS* subscriber);
    void removeSubscriber(SubscriberSNS* subscriber);
    void publish(const Message& message);

private:
    std::unordered_set<SubscriberSNS*> mSubscribers;
};

class SubscriberSNS
{
public:
    SubscriberSNS() = default;
    SubscriberSNS(std::initializer_list<Topic::Type> topics);

    virtual ~SubscriberSNS();
    virtual void notify(const Message& message) {};

    void subscribe(Topic::Type topicType);
    void unsubscribe(Topic::Type topicType);

private:
    std::unordered_set<Topic::Type> mSubscriptionList;
};

// SimpleNotificationService
class SNS
{
public:
    static void subscribe(Topic::Type topicType, SubscriberSNS* subscriber);
    static void unsubscribe(Topic::Type topicType, SubscriberSNS* subscriber);
    static void publishMessage(Topic::Type topicType, const Message& message);

private:
    inline static std::array<Topic, Topic::Type::Count> mTopics;
};

#endif //VULKANRENDERINGENGINE_SIMPLE_NOTIFICATION_SERVICE_HPP
