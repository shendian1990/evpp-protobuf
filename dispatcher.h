#pragma once
#include <memory>
#include <functional>
#include <map>

#include "common.h"
#include <type_traits>
#include <google/protobuf/message.h>
#include <evpp/tcp_conn.h>
#include <evpp/timestamp.h>

using MessagePtr = std::shared_ptr<google::protobuf::Message>;

class Callback:noncopyable
{
public:
	virtual ~Callback();
	virtual void onMessage(const evpp::TCPConnPtr&, const MessagePtr& message, evpp::Timestamp) const = 0;
};

template<typename T>
class CallbackT : public Callback
{
	//static_assert(std::is_base_of<google::protobuf::Message, T>::value);
public :
	using ProtobufMessageTCallback = std::function<void(const evpp::TCPConnPtr&, 
		const std::shared_ptr<T>& message,evpp::Timestamp)>;
	CallbackT(const ProtobufMessageTCallback& callback)
		:callback_(callback){}

	void onMessage(const evpp::TCPConnPtr& conn, const MessagePtr& message, evpp::Timestamp receiveTime) const override
	{
		std::shared_ptr<T> concrete = std::static_pointer_cast<T>(message);
		assert(concrete != nullptr);
		callback_(conn, concrete, receiveTime);
	}


private:
	ProtobufMessageTCallback callback_;

};

class ProtobufDispatcher
{
private:
	using CallbackMap = std::map<const google::protobuf::Descriptor*, std::shared_ptr<Callback>>;
public:
	using ProtobufMessageCallback = std::function<void(const evpp::TCPConnPtr&, const MessagePtr& message, evpp::Timestamp)>;

	explicit ProtobufDispatcher(const ProtobufMessageCallback& defaultCb)
		: defaultCallback_(defaultCb){}

	void onProtobufMessage(const evpp::TCPConnPtr& conn, const MessagePtr& message, evpp::Timestamp receiveTime) const
	{
		CallbackMap::const_iterator it = callbacks_.find(message->GetDescriptor());
		if(it!=callbacks_.end())
		{
			it->second->onMessage(conn, message, receiveTime);
		}
		else
		{
			defaultCallback_(conn, message, receiveTime);
		}
	}

	template<typename T>
	void registerMessageCallback(const typename CallbackT<T>::ProtobufMessageTCallback& callback)
	{
		auto pd = std::make_shared<CallbackT<T>>(callback);
		callbacks_[T::descriptor()] = pd;
	}

private:
	CallbackMap callbacks_;
	ProtobufMessageCallback defaultCallback_;
};
