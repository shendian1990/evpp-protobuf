#include "codec.h"
#include "portable-endian.h"
#include "query.pb.h"
#include "winmain-inl.h"

#include <cstdio>
#include <zlib.h>

void print(const evpp::Buffer& buf)
{
	printf("encode to %zd bytes\n", buf.length());
	for (size_t i=0; i<buf.length();++i)
	{
		auto ch = static_cast<unsigned char>(buf.data()[i]);

		printf("%2zd:  0x%02x  %c\n", i, ch, isgraph(ch) ? ch : ' ');
	}
}

void testQuery()
{
	muduo::Query query;
	query.set_id(1);
	query.set_questioner("Chen Shuo");
	query.add_question("Running?");

	evpp::Buffer buf;
	ProtobufCodec::fillEmptyBuffer(&buf, query);
	print(buf);

	const int32_t len = buf.ReadInt32();
	assert(len == static_cast<int32_t>(buf.length()));

	ProtobufCodec::ErrorCode errorCode = ProtobufCodec::kNoError;
	MessagePtr message = ProtobufCodec::parse(buf.data(), len, &errorCode);
	assert(errorCode == ProtobufCodec::kNoError);
	assert(message != NULL);
	message->PrintDebugString();
	assert(message->DebugString() == query.DebugString());

	std::shared_ptr<muduo::Query> newQuery = std::static_pointer_cast<muduo::Query>(message);
	assert(newQuery != NULL);
}
/*
int main()
{
	GOOGLE_PROTOBUF_VERIFY_VERSION;
	testQuery();
	google::protobuf::ShutdownProtobufLibrary();
}
*/