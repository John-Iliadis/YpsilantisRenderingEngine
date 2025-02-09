//
// Created by Gianni on 25/01/2025.
//

#include "simple_notification_service.hpp"

Message Message::modelDeleted(uuid32_t modelID)
{
    return Message(std::in_place_type_t<Message::ModelDeleted>(), modelID);
}

Message Message::meshInstanceUpdate(uuid32_t meshID, uuid32_t objectID, uint32_t instanceID, glm::mat4 transformation)
{
    return Message(std::in_place_type_t<Message::MeshInstanceUpdate>(), meshID, objectID, instanceID, transformation);
}

Message Message::removeMeshInstance(uuid32_t meshID, uint32_t instanceID)
{
    return Message(std::in_place_type_t<Message::RemoveMeshInstance>(), meshID, instanceID);
}

void Topic::addSubscriber(SubscriberSNS *subscriber)
{
    mSubscribers.insert(subscriber);
}

void Topic::removeSubscriber(SubscriberSNS *subscriber)
{
    mSubscribers.erase(subscriber);
}

void Topic::publish(const Message &message)
{
    for (auto subscriber : mSubscribers)
        subscriber->notify(message);
}

SubscriberSNS::SubscriberSNS(std::initializer_list<Topic::Type> topics)
    : mSubscriptionList(topics)
{
    for (auto topic : mSubscriptionList)
        SNS::subscribe(topic, this);
}

SubscriberSNS::~SubscriberSNS()
{
    for (auto topic : mSubscriptionList)
        SNS::unsubscribe(topic, this);
}

void SubscriberSNS::subscribe(Topic::Type topicType)
{
    if (!mSubscriptionList.contains(topicType))
    {
        mSubscriptionList.insert(topicType);
        SNS::subscribe(topicType, this);
    }
}

void SubscriberSNS::unsubscribe(Topic::Type topicType)
{
    if (mSubscriptionList.contains(topicType))
    {
        mSubscriptionList.erase(topicType);
        SNS::unsubscribe(topicType, this);
    }
}

void SNS::subscribe(Topic::Type topicType, SubscriberSNS *subscriber)
{
    mTopics.at(topicType).addSubscriber(subscriber);
}

void SNS::unsubscribe(Topic::Type topicType, SubscriberSNS *subscriber)
{
    mTopics.at(topicType).removeSubscriber(subscriber);
}

void SNS::publishMessage(Topic::Type topicType, const Message& message)
{
    mTopics.at(topicType).publish(message);
}
