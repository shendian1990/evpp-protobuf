#include "query.pb.h"

#include <iostream>
#include <typeinfo>
#include <cassert>
#include <cstdio>

using std::cout;
using std::endl;

template<typename T>
void testDescriptor()
{
	std::string type_name = T::descriptor()->full_name();
	cout << type_name << endl;

	const google::protobuf::Descriptor* descriptor = google::protobuf::DescriptorPool::generated_pool()->FindMessageTypeByName(type_name);
	assert(descriptor == T::descriptor());
	cout << "FindMessageTypeByName() = " << descriptor << endl;
	cout << "T::descriptor()     =" << T::descriptor() << endl;
	cout << endl;

	const google::protobuf::Message* prototype =
		google::protobuf::MessageFactory::generated_factory()->GetPrototype(descriptor);
	assert(prototype == &T::default_instance());
	cout << "GetPrototype()        = " << prototype << endl;
	cout << "T::default_instance() = " << &T::default_instance() << endl;
	cout << endl;

	T* new_obj = dynamic_cast<T*>(prototype->New());
	assert(new_obj != nullptr);
	assert(new_obj != prototype);
	assert(typeid(*new_obj) == typeid(T::default_instance()));
	cout << "prototype->New() = " << new_obj << endl;
	cout << endl;
	delete new_obj;
}

google::protobuf::Message* createMessage(const std::string& type_name)
{
	google::protobuf::Message* message = nullptr;
	const google::protobuf::Descriptor* descriptor =
		google::protobuf::DescriptorPool::generated_pool()->FindMessageTypeByName(type_name);
	if(descriptor != nullptr)
	{
		const google::protobuf::Message* prototype =
			google::protobuf::MessageFactory::generated_factory()->GetPrototype(descriptor);
		if (prototype)
			message = prototype->New();
	}
	return message;
}
