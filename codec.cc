#include "codec.h"
#include "google-inl.h"
#include "portable-endian.h"
#include <google/protobuf/descriptor.h>



#include <zlib.h>

void ProtobufCodec::fillEmptyBuffer(evpp::Buffer* buf, const google::protobuf::Message& message)
{
	//assert(buf->ReadByte() == 0);

	const auto& typeName = message.GetTypeName();
	int32_t nameLen = static_cast<int32_t>(typeName.size() + 1);
	buf->AppendInt32(nameLen);
	buf->Append(typeName.c_str(), nameLen);

	GOOGLE_CHECK(message.IsInitialized()) << InitializationErrorMessage("serialize", message);

	int byte_size = message.ByteSize();
	buf->EnsureWritableBytes(byte_size);

	uint8_t* start = reinterpret_cast<uint8_t*>(buf->WriteBegin());
	uint8_t* end = message.SerializeWithCachedSizesToArray(start);
	
	if(end - start != byte_size)
	{
		ByteSizeConsistencyError(byte_size, message.ByteSize(), static_cast<int>(end - start));
	}
	buf->WriteBytes(byte_size);
	auto checkSum = buf->length()+4;
	buf->AppendInt32(checkSum);
	assert(buf->length() == sizeof nameLen + nameLen + byte_size + sizeof checkSum);
	int32_t len = htobe32(static_cast<int32_t>(buf->length()));
	buf->Prepend(&len, sizeof len);
}

namespace
{
	const std::string kNoErrorStr = "NoError";
	const std::string kInvalidLengthStr = "InvalidLength";
	const std::string kCheckSumErrorStr = "CheckSumError";
	const std::string kInvalidNameLenStr = "InvalidNameLen";
	const std::string kUnknownMessageTypeStr = "UnknownMessageType";
	const std::string kParseErrorStr = "ParseError";
	const std::string kUnknownErrorStr = "UnknownError";
}

const std::string& ProtobufCodec::errorCodeToString(ErrorCode errorCode)
{
	switch (errorCode)
	{
	case kNoError:
		return kNoErrorStr;
	case kInvalidLength:
		return kInvalidLengthStr;
	case kCheckSumError:
		return kCheckSumErrorStr;
	case kInvalidNameLen:
		return kInvalidNameLenStr;
	case kUnknownMessageType:
		return kUnknownMessageTypeStr;
	case kParseError:
		return kParseErrorStr;
	default:
		return kUnknownErrorStr;
	}
}

void ProtobufCodec::defaultErrorCallback(const evpp::TCPConnPtr& conn, evpp::Buffer* buf, evpp::Timestamp, ErrorCode errorCode)
{
	LOG_ERROR << "ProtobufCodec::defaultErrorCallback - " << errorCodeToString(errorCode);
	if (conn && conn->IsConnected())
		conn->Close();
}

int32_t asInt32(const char* buf)
{
	int32_t be32 = 0;
	::memcpy(&be32, buf, sizeof(be32));
	return be32toh(be32);
}

void ProtobufCodec::onMessage(const evpp::TCPConnPtr& conn, evpp::Buffer* buf, evpp::Timestamp receiveTime)
{
	while(buf->length() >= kMinMessageLen+kHeaderLen)
	{
		const int32_t len = buf->PeekInt32();
		if(len > kMaxMessageLen || len<kMinMessageLen)
		{
			errorCallback_(conn, buf, receiveTime, kInvalidLength);
			break;
		}
		else if (buf->length() >= static_cast<size_t>(len + kHeaderLen))
		{
			ErrorCode errorCode = kNoError;
			MessagePtr message = parse(buf->data()+kHeaderLen, len, &errorCode);
			if(errorCode == kNoError && message)
			{
				messageCallback_(conn, message, receiveTime);
				buf->Retrieve(kHeaderLen + len);
			}
			else
			{
				errorCallback_(conn, buf, receiveTime, errorCode);
				break;
			}
		}
		else
		{
			break;
		}
	}
}


google::protobuf::Message* ProtobufCodec::createMessage(const std::string& type_name)
{
	google::protobuf::Message* message = nullptr;
	const auto* descriptor = google::protobuf::DescriptorPool::generated_pool()->FindMessageTypeByName(type_name);
	if(descriptor != nullptr)
	{
		const auto* prototype = google::protobuf::MessageFactory::generated_factory()->GetPrototype(descriptor);
		if (prototype)
			message = prototype->New();
	}
	return message;
}

MessagePtr ProtobufCodec::parse(const char* buf, int len, ErrorCode* error)
{
	MessagePtr message;

	//checkSum
	int32_t expectedCheckSum = asInt32(buf + len - kHeaderLen);
	auto    checkSum = asInt32(buf + len - kHeaderLen);
	if(checkSum == expectedCheckSum)
	{
		int32_t nameLen = asInt32(buf);
		if(nameLen >=2 && nameLen <= len-2*kHeaderLen)
		{
			std::string typeName(buf + kHeaderLen, buf + kHeaderLen + nameLen - 1);
			//create message object
			message.reset(createMessage(typeName));
			if(message)
			{
				const char* data = buf + kHeaderLen + nameLen;
				int32_t dataLen = len - nameLen - 2 * kHeaderLen;
				if(message->ParseFromArray(data,dataLen))
				{
					*error = kNoError;
				}
				else
				{
					*error = kParseError;
				}
			}
			else
			{
				*error = kInvalidNameLen;
			}
		}
		else
		{
			*error = kCheckSumError;
		}
		return message;
	}
}
