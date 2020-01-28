

#ifndef __METACLIENT__
#define __METACLIENT__


#include <iostream>
#include <memory>
#include <string>

#include <grpcpp/grpcpp.h>

#include "unistd.h"

#include "metaserver.grpc.pb.h"


using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

using metaserver::Meta;

using metaserver::PutReply;
using metaserver::PutRequest;

using metaserver::GetReply;
using metaserver::GetRequest;

using metaserver::TimeReply;
using metaserver::TimeRequest;


class MetaClient
{
public:
    MetaClient(std::shared_ptr<Channel> channel)
        : stub_(Meta::NewStub(channel)) {}

    std::string Recordtime(const std::string &key);

    std::string Recordtimestart(const std::string &key);

    std::string Recordtimetick(const std::string &key);

    std::string Getmeta(const std::string &key);

    std::string Putmeta(const std::string &key, const std::string &value);

    std::string Getmetaspace(const std::string &key);

    std::string Putmetaspace(const std::string &key, const std::string &value);

private:
    std::unique_ptr<Meta::Stub> stub_;
};


MetaClient getMetaClient();


#endif