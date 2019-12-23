// Copyright 2016 Proyectos y Sistemas de Mantenimiento SL (eProsima).
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/**
 * @file HelloWorldSubscriber.cpp
 *
 */

#include "HelloWorldSubscriber.h"
#include <fastrtps/attributes/ParticipantAttributes.h>
#include <fastrtps/attributes/SubscriberAttributes.h>
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/subscriber/Subscriber.hpp>
#include <fastdds/dds/topic/DataReader.hpp>
#include <fastdds/dds/topic/qos/DataReaderQos.hpp>
#include <fastdds/dds/topic/Topic.hpp>
#include <fastrtps/attributes/TopicAttributes.h>

#include <fastdds/dds/core/conditions/WaitSet.hpp>
#include <fastdds/dds/core/conditions/StatusCondition.hpp>

using namespace eprosima::fastdds::dds;
using namespace ::dds::core::status;

HelloWorldSubscriber::HelloWorldSubscriber()
    : participant_(nullptr)
    , subscriber_(nullptr)
    , type_(new HelloWorldPubSubType())
{
}

bool HelloWorldSubscriber::init(
        int domain_id)
{
    DomainParticipantQos part_qos = PARTICIPANT_QOS_DEFAULT;
    part_qos.part_attr.rtps.builtin.domainId = domain_id;
    part_qos.part_attr.rtps.setName("Participant_sub");
    participant_ = DomainParticipantFactory::get_instance()->create_participant(
        part_qos.part_attr.rtps.builtin.domainId, part_qos);

    if (participant_ == nullptr)
    {
        return false;
    }

    //REGISTER THE TYPE
    type_.register_type(participant_, type_.get_type_name());

    //CREATE THE SUBSCRIBER
    SubscriberQos sub_qos = SUBSCRIBER_QOS_DEFAULT;
    subscriber_ = participant_->create_subscriber(sub_qos, nullptr);

    if (subscriber_ == nullptr)
    {
        return false;
    }

    //Set the handler associated to the subscriber status condition
    subscriber_->get_statuscondition()->set_handler([this]() -> void {
        std::vector<DataReader*> readers;
        subscriber_->get_datareaders(readers);
        for (auto reader : readers)
        {
            data_available_handler(reader);
        }
        subscriber_->get_statuscondition()->set_status_as_read(StatusMask::data_on_readers());
    });

    waitset_.attach_condition(subscriber_->get_statuscondition());

    // CREATE THE READER
    DataReaderQos rqos;
    rqos.reliability.kind = RELIABLE_RELIABILITY_QOS;
    rqos.reliability.max_blocking_time = 500; //It is neccesary in order to wait until the Publisher topic is created
    //Topic topic(participant_, "HelloWorldTopic", "HelloWorld", TopicQos()); //PSM
    TopicDescription topic_desc(participant_, "HelloWorldTopic", "HelloWorld"); //PIM
    reader_ = subscriber_->create_datareader(topic_desc, rqos, nullptr);

    if (reader_ == nullptr)
    {
        return false;
    }

    //Set the handler associated to the reader status condition
    reader_->get_statuscondition()->set_handler([this]() -> void {
        StatusMask triggered_status = reader_->get_status_changes();
        if (triggered_status.is_active(StatusMask::subscription_matched()))
        {
            subscription_matched_handler();
        }
        if (triggered_status.is_active(StatusMask::requested_incompatible_qos()))
        {
            requested_incompatible_qos_handler();
        }
        if (triggered_status.is_active(StatusMask::data_available()))
        {
            data_available_handler(reader_);
        }
        if (triggered_status.is_active(StatusMask::sample_rejected()))
        {
            sample_rejected_handler();
        }
        if (triggered_status.is_active(StatusMask::liveliness_changed()))
        {
            liveliness_changed_handler();
        }
        if (triggered_status.is_active(StatusMask::requested_deadline_missed()))
        {
            requested_deadline_missed_handler();
        }

    });

    waitset_.attach_condition(reader_->get_statuscondition());

    return true;
}

HelloWorldSubscriber::~HelloWorldSubscriber()
{
    DomainParticipantFactory::get_instance()->delete_participant(participant_);
}

void HelloWorldSubscriber::run()
{
    std::cout << "Subscriber running." << std::endl;
    //Creation of waitset and attachment of the conditions
    ConditionSeq active_conditions;
    while (true)
    {
        if (waitset_.wait(active_conditions, Duration_t(0, 0)) != ReturnCode_t::RETCODE_TIMEOUT)
        {
            for (auto cond: active_conditions)
            {
                cond->call_handler();
            }
        }
        //waitset_.dispatch(Duration_t(0, 0));
    }
}

void HelloWorldSubscriber::data_available_handler(
        eprosima::fastdds::dds::DataReader* reader)
{
    while (reader->take_next_sample(&hello_, &info_) == ReturnCode_t::RETCODE_OK)
    {
        if (info_.instance_state == ::dds::sub::status::InstanceState::alive())
        {
            samples_++;
            // Print your structure data here.
            std::cout << "Message " << hello_.message() << " " << hello_.index() << " RECEIVED" <<
                std::endl;
        }
    }
    reader->get_statuscondition()->set_status_as_read(StatusMask::data_available());
}

void HelloWorldSubscriber::liveliness_changed_handler()
{
    eprosima::fastdds::dds::LivelinessChangedStatus status;
    reader_->get_liveliness_changed_status(status);
    if (status.alive_count_change == 1)
    {
        std::cout << "Publisher " << status.last_publication_handle << " recovered liveliness" << std::endl;
    }
    else if (status.not_alive_count_change == 1)
    {
        std::cout << "Publisher " << status.last_publication_handle << " lost liveliness" << std::endl;
    }
}

void HelloWorldSubscriber::requested_deadline_missed_handler()
{
    eprosima::fastdds::dds::RequestedDeadlineMissedStatus status;
    reader_->get_requested_deadline_missed_status(status);
    std::cout << "Deadline missed for instance: " << status.last_instance_handle << std::endl;
}

void HelloWorldSubscriber::requested_incompatible_qos_handler()
{
    eprosima::fastdds::dds::RequestedIncompatibleQosStatus status;
    reader_->get_requested_incompatible_qos_status(status);
    DataReaderQos qos;
    std::cout << "The Requested Qos is incompatible with the Offered one." << std::endl;
    std::cout << "The Qos causing this incompatibility is " << qos.search_qos_by_id(
        status.last_policy_id) << "." << std::endl;
}

void HelloWorldSubscriber::subscription_matched_handler()
{
    eprosima::fastdds::dds::SubscriptionMatchedStatus info;
    reader_->get_subscription_matched_status(info);
    if (info.current_count_change == 1)
    {
        matched_ = info.total_count;
        std::cout << "Subscriber matched." << std::endl;
    }
    else if (info.current_count_change == -1)
    {
        matched_ = info.total_count;
        std::cout << "Subscriber unmatched." << std::endl;
    }
    else
    {
        std::cout << info.current_count_change
                  << " is not a valid value for SubscriptionMatchedStatus current count change" << std::endl;
    }
}

void HelloWorldSubscriber::sample_rejected_handler()
{
    eprosima::fastdds::dds::SampleRejectedStatus status;
    reader_->get_sample_rejected_status(status);
    std::cout << "The sample " << status.last_seq_num << " is " << status.reason_to_string() <<
        std::endl;
}