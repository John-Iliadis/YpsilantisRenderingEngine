//
// Created by Gianni on 25/01/2025.
//

#ifndef VULKANRENDERINGENGINE_SIMPLE_NOTIFICATION_SERVICE_HPP
#define VULKANRENDERINGENGINE_SIMPLE_NOTIFICATION_SERVICE_HPP

#include <glm/glm.hpp>
#include "types.hpp"

class Model;
class SubscriberSNS;

class Message
{
public:
    struct ModelDeleted
    {
        uuid32_t modelID;
    };

    struct MeshInstanceUpdate
    {
        uuid32_t meshID;
        uuid32_t objectID;
        glm::mat4 transformation;
    };

    struct RemoveMeshInstance
    {
        uuid32_t meshID;
        uuid32_t objectID;
    };

    struct LoadModel
    {
        std::filesystem::path path;
        // options
    };

    struct ModelLoaded
    {
        std::shared_ptr<Model> model;
        // status
    };

    std::variant<ModelDeleted,
        MeshInstanceUpdate,
        RemoveMeshInstance,
        LoadModel,
        ModelLoaded> message;

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

    static Message modelDeleted(uuid32_t modelID);
    static Message meshInstanceUpdate(uuid32_t meshID, uuid32_t objectID, glm::mat4 transformation);
    static Message removeMeshInstance(uuid32_t meshID, uuid32_t objectID);
    static Message loadModel(const std::filesystem::path& path);
    static Message modelLoaded(std::shared_ptr<Model> model);
};

class Topic
{
public:
    enum Type
    {
        None = 0,
        Editor,
        SceneGraph,
        Renderer,
        Resource,
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
