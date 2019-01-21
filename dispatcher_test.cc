#include "dispatcher.h"
#include "query.pb.h"

#include <iostream>

using std::cout;
using std::endl;

using QueryPtr = std::shared_ptr<muduo::Query>;
using AnswerPtr = std::shared_ptr<muduo::Answer>;

void test_down_pointer_cast()
{
	std::shared_ptr<google::protobuf::Message> msg(new muduo::Query);
	std::shared_ptr<muduo::Query> query(std::static_pointer_cast<muduo::Query>(msg));
	assert(msg && query);
	if (!query)
		abort();
}

void onQuery(const evpp::TCPConnPtr&, const QueryPtr& message, evpp::Timestamp)
{
	cout << "on Query: " << message->GetTypeName() << endl;
}

void onAnswer(const evpp::TCPConnPtr&, const AnswerPtr& message, evpp::Timestamp)
{
	cout << "onAnswer: " << message->GetTypeName() << endl;
}

void onUnknownMessageType(const evpp::TCPConnPtr&, const MessagePtr& message, evpp::Timestamp)
{
	cout << "onUnknownMessageType: " << message->GetTypeName() << endl;
}

int main()
{
	GOOGLE_PROTOBUF_VERIFY_VERSION;
	test_down_pointer_cast();

	ProtobufDispatcher dispatcher(onUnknownMessageType);
	dispatcher.registerMessageCallback<muduo::Query>(onQuery);
	dispatcher.registerMessageCallback<muduo::Answer>(onAnswer);

	evpp::TCPConnPtr conn;
	evpp::Timestamp t;

	std::shared_ptr<muduo::Query> query(new muduo::Query);
	std::shared_ptr<muduo::Answer> answer(new muduo::Answer);
	std::shared_ptr<muduo::Empty> empty(new muduo::Empty);
	dispatcher.onProtobufMessage(conn, query, t);
	dispatcher.onProtobufMessage(conn, answer, t);
	dispatcher.onProtobufMessage(conn, empty, t);

	google::protobuf::ShutdownProtobufLibrary();
}

