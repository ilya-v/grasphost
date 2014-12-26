#ifndef _server__h__
#define _server__h__

#include <memory>

namespace DataServerImpl { struct DataServer;  };

struct  DataServer {

    void SendData(const std::string &data);
    bool IsGood() const;   
    DataServer();
    ~DataServer(); 

    std::unique_ptr<DataServerImpl::DataServer>  data_server; 
};

#endif
