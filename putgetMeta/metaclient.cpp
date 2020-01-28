
/*
 *
 * Copyright 2015 gRPC authors.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include <iostream>
#include <memory>
#include <string>

#include <grpcpp/grpcpp.h>

#include <iostream>
#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <unistd.h>
#include <arpa/inet.h>

#include <fstream>

#include <sys/stat.h>
#include <map>
#include <dirent.h>
#include <system_error>
#include <cstring>
#include <set>


#include "metaserver.grpc.pb.h"
#include "metaclient.h"

const std::string metaserverDir = "./Metaserver";

std::vector<std::string> loadAddrInDir(std::string Dir)
{
    // only init the client in clusterDir
    std::string dir = Dir;
    DIR *dp;
    std::vector<std::string> AddrList;
    struct dirent *entry;
    struct stat statbuf;
    try
    {
        if ((dp = opendir(dir.data())) == NULL)
        {
            printf("Can`t open directory %s\n", dir.data());
            return AddrList;
        }

        while ((entry = readdir(dp)) != NULL)
        {

            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            {
                continue;
            }

            //printf("load addr name %s\n", entry->d_name);
            AddrList.push_back(std::string(entry->d_name));
        }
        //change to the upper dir
        //printf("debug id %d load finish\n", GETIPCOMPONENTID);
        closedir(dp);
        return AddrList;
    }
    catch (const std::system_error &e)
    {
        std::cout << "getip Caught system_error with code " << e.code()
                  << " meaning " << e.what() << '\n';
        exit(1);
    }
}

std::string MetaClient::Recordtime(const std::string &key)
{
    // Data we are sending to the server.
    TimeRequest request;
    request.set_key(key);

    // Container for the data we expect from the server.
    TimeReply reply;

    // Context for the client. It could be used to convey extra information to
    // the server and/or tweak certain RPC behaviors.
    ClientContext context;

    // The actual RPC.
    Status status = stub_->Recordtime(&context, request, &reply);

    // Act upon its status.
    if (status.ok())
    {
        return reply.message();
    }
    else
    {
        std::cout << status.error_code() << ": " << status.error_message()
                  << std::endl;
        return "RPC failed";
    }
}

std::string MetaClient::Recordtimestart(const std::string &key)
{
    // Data we are sending to the server.
    TimeRequest request;
    request.set_key(key);

    // Container for the data we expect from the server.
    TimeReply reply;

    // Context for the client. It could be used to convey extra information to
    // the server and/or tweak certain RPC behaviors.
    ClientContext context;

    // The actual RPC.
    Status status = stub_->Recordtimestart(&context, request, &reply);

    // Act upon its status.
    if (status.ok())
    {
        return reply.message();
    }
    else
    {
        std::cout << status.error_code() << ": " << status.error_message()
                  << std::endl;
        return "RPC failed";
    }
}

std::string MetaClient::Recordtimetick(const std::string &key)
{
    // Data we are sending to the server.
    TimeRequest request;
    request.set_key(key);

    // Container for the data we expect from the server.
    TimeReply reply;

    // Context for the client. It could be used to convey extra information to
    // the server and/or tweak certain RPC behaviors.
    ClientContext context;

    // The actual RPC.
    Status status = stub_->Recordtimetick(&context, request, &reply);

    // Act upon its status.
    if (status.ok())
    {
        return reply.message();
    }
    else
    {
        std::cout << status.error_code() << ": " << status.error_message()
                  << std::endl;
        return "RPC failed";
    }
}

std::string MetaClient::Getmeta(const std::string &key)
{
    // Data we are sending to the server.
    GetRequest request;
    request.set_key(key);

    // Container for the data we expect from the server.
    GetReply reply;

    // Context for the client. It could be used to convey extra information to
    // the server and/or tweak certain RPC behaviors.
    ClientContext context;

    // The actual RPC.
    Status status = stub_->Getmeta(&context, request, &reply);

    // Act upon its status.
    if (status.ok())
    {
        return reply.message();
    }
    else
    {
        std::cout << status.error_code() << ": " << status.error_message()
                  << std::endl;
        return "RPC failed";
    }
}

std::string MetaClient::Putmeta(const std::string &key, const std::string &value)
{
    // Data we are sending to the server.
    PutRequest request;
    request.set_key(key);
    request.set_value(value);

    // Container for the data we expect from the server.
    PutReply reply;

    // Context for the client. It could be used to convey extra information to
    // the server and/or tweak certain RPC behaviors.
    ClientContext context;

    // The actual RPC.
    Status status = stub_->Putmeta(&context, request, &reply);

    // Act upon its status.
    if (status.ok())
    {
        return reply.message();
    }
    else
    {
        std::cout << status.error_code() << ": " << status.error_message()
                  << std::endl;
        return "RPC failed";
    }
}

std::string MetaClient::Getmetaspace(const std::string &key)
{
    // Data we are sending to the server.
    GetRequest request;
    request.set_key(key);

    // Container for the data we expect from the server.
    GetReply reply;

    // Context for the client. It could be used to convey extra information to
    // the server and/or tweak certain RPC behaviors.
    ClientContext context;

    // The actual RPC.
    Status status = stub_->Getmetaspace(&context, request, &reply);

    // Act upon its status.
    if (status.ok())
    {
        return reply.message();
    }
    else
    {
        std::cout << status.error_code() << ": " << status.error_message()
                  << std::endl;
        return "RPC failed";
    }
}

std::string MetaClient::Putmetaspace(const std::string &key, const std::string &value)
{
    // Data we are sending to the server.
    PutRequest request;
    request.set_key(key);
    request.set_value(value);

    // Container for the data we expect from the server.
    PutReply reply;

    // Context for the client. It could be used to convey extra information to
    // the server and/or tweak certain RPC behaviors.
    ClientContext context;

    // The actual RPC.
    Status status = stub_->Putmetaspace(&context, request, &reply);

    // Act upon its status.
    if (status.ok())
    {
        return reply.message();
    }
    else
    {
        std::cout << status.error_code() << ": " << status.error_message()
                  << std::endl;
        return "RPC failed";
    }
}

std::string getAddr()
{

    std::vector<std::string> addrList = loadAddrInDir(metaserverDir);
    if (addrList.size() > 0)
    {
        return addrList[0];
    }

    return "";
}

void recordKey(std::string key)
{

    std::string serverAddr = getAddr();

    if (serverAddr == "")
    {
        printf("failed to get server addr\n");
        return;
    }

    MetaClient metaclient(grpc::CreateChannel(
        serverAddr, grpc::InsecureChannelCredentials()));

    std::string reply = metaclient.Recordtime(key);
    std::cout << "Timer received: " << reply << std::endl;

    return;
}

void recordKeyStart(std::string key)
{
    //if use parallel file system
    //load the addr from the file
    std::string serverAddr = getAddr();

    if (serverAddr == "")
    {
        printf("failed to get server addr\n");
        return;
    }

    MetaClient metaclient(grpc::CreateChannel(
        serverAddr, grpc::InsecureChannelCredentials()));

    std::string reply = metaclient.Recordtimestart(key);
    std::cout << "Timer received: " << reply << std::endl;

    return;
}

void recordKeyTick(std::string key)
{

     std::string serverAddr = getAddr();

    if (serverAddr == "")
    {
        printf("failed to get server addr\n");
        return;
    }

    MetaClient metaclient(grpc::CreateChannel(
        serverAddr, grpc::InsecureChannelCredentials()));

     std::string reply = metaclient.Recordtimetick(key);
    std::cout << "Timer received: " << reply << std::endl;

    return;
}

MetaClient getMetaClient()
{
     std::string serverAddr = getAddr();

    if (serverAddr == "")
    {

        throw std::runtime_error("the addr for MMserver should not be empty");
    }

    MetaClient metaclient(grpc::CreateChannel(
        serverAddr, grpc::InsecureChannelCredentials()));

    return metaclient;
}