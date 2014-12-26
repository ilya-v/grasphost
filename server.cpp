#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif


#include <iostream>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <thread>
#include <atomic>
#include <chrono>

#include "server.h"
#include "event.h"
#include "socket.h"

namespace DataServerImpl {

    struct  DataServer {

        bool IsGood() const { return client_thread.joinable(); }

        const std::string           port = "11003";

        std::queue<std::string>     data_queue;
        std::mutex                  data_mutex;
        std::condition_variable     data_cv;
        std::thread                 client_thread;
        std::atomic_bool            is_client_connected;
        Event                       data_event;
        ServerSocket                server_socket;

        void SendData(const std::string &data)
        {           
            if (!is_client_connected || !IsGood())
                return;
            std::lock_guard<std::mutex> data_guard(data_mutex);
            data_queue.push(data);
            data_event.Signal();
        }

        DataServer() {
            is_client_connected = false;
            server_socket.Init(port.c_str());
            if (!server_socket.IsGood())  return;

            client_thread = std::thread(&DataServer::workerThread, this);
        }

        std::string  wipeQueue(bool & stop_flag)
        {
            data_event.Wait();
            std::string  data_string;
            std::lock_guard<std::mutex> data_lock(data_mutex);
            for (; !data_queue.empty(); data_queue.pop())
            {
                stop_flag = data_queue.front().empty();
                data_string += data_queue.front();
            }
            return data_string;
        }
        
        void workerThread()
        {        
            for (bool stop_flag = false; !stop_flag && server_socket.IsGood(); 
                std::this_thread::sleep_for(std::chrono::seconds(1)))
            {
                is_client_connected = false;
                for (ClientSocket client_socket = server_socket.Accept(); client_socket.IsGood() && server_socket.IsGood() && !stop_flag;)
                {   
                    is_client_connected = true;
                    const std::string  data_string = wipeQueue(stop_flag);
                    client_socket.Send(data_string.c_str(), data_string.length());
                    is_client_connected = client_socket.IsGood() && !stop_flag;
                }
            }
        }

        ~DataServer()
        {      
            if (client_thread.joinable())
            {
                SendData(std::string());
                client_thread.join();
            }
            
        }
    };
}

void DataServer::SendData(const std::string &data) { data_server->SendData(data); }
bool DataServer::IsGood() const { return data_server->IsGood(); }
DataServer::DataServer() { data_server.reset(new DataServerImpl::DataServer); }
DataServer::~DataServer() {}
