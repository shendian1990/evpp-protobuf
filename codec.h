#pragma once

#include <evpp/buffer.h>
#include <evpp/tcp_conn.h>
#include <evpp/timestamp.h>
#include "common.h"

#include <functional>
#include <memory>
#include <google/protobuf/message.h>

using MessagePtr = std::shared_ptr<google::protobuf::Message>;

class ProtobufCodec : public noncopyable
{
public:
	enum ErrorCode
	{
		kNoError = 0,
		kInvalidLength,
		kCheckSumError,
		kInvalidNameLen,
		kUnknownMessageType,
		kParseError,
	};

	using ProtobufMessageCallback = std::function<void(const evpp::TCPConnPtr&,
		const MessagePtr&,
		evpp::Timestamp)>;

	using ErrorCallback = std::function<void(const evpp::TCPConnPtr&,
		evpp::Buffer*,
		evpp::Timestamp,
		ErrorCode)>;

	explicit  ProtobufCodec(const ProtobufMessageCallback& messageCb)
		:messageCallback_(messageCb),errorCallback_(defaultErrorCallback)
	{}

	ProtobufCodec(const ProtobufMessageCallback& messageCb, const ErrorCallback& errorCb)
		:messageCallback_(messageCb),errorCallback_(errorCb)
	{}

	void onMessage(const evpp::TCPConnPtr& conn, evpp::Buffer* buf, evpp::Timestamp receiveTime);

	void send(const evpp::TCPConnPtr& conn, const google::protobuf::Message& message)
	{
		evpp::Buffer buf;
		fillEmptyBuffer(&buf, message);
		conn->Send(&buf);
	}

	static const std::string& errorCodeToString(ErrorCode errorCode);
	static void fillEmptyBuffer(evpp::Buffer* buf, const google::protobuf::Message& message);
	static google::protobuf::Message* createMessage(const std::string& type_name);
	static MessagePtr parse(const char* buf, int len, ErrorCode* errorCode);


private:
	static void defaultErrorCallback(const evpp::TCPConnPtr&,
		evpp::Buffer*, evpp::Timestamp, ErrorCode);

	ProtobufMessageCallback messageCallback_;
	ErrorCallback			errorCallback_;

	const static int kHeaderLen = sizeof(int32_t);
	const static int kMinMessageLen = 2 * kHeaderLen + 2;
	const static int kMaxMessageLen = 64 * 1024 * 1024;
};




