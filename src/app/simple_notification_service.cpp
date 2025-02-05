//
// Created by Gianni on 25/01/2025.
//

#include "simple_notification_service.hpp"

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
