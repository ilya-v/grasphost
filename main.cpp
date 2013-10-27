#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <windows.h>

#include <functional>
#include <iostream>
#include <map>
#include <vector>
#include <tuple>
#include <string>
#include <array>
#include <algorithm>

#pragma warning( disable : 4200 )

#include "cmd_def.h"
#include "serial_port.h"
#include "state_machine.h"

#define LOG(cmd) do { std::clog << "\t" << __LINE__ << ": " << #cmd << std::endl; cmd; } while(0);

#define ENSURE(cmd, err)  do { if (!(cmd)) { \
    std::cout << "ERROR in line " << __LINE__ << "\t" << (err) << std::endl; throw BaseException(); } } while (0);

struct BaseException {};
//-----------------------------------------------------------------------------------------------------------------------------------------------------------//

struct service_uuid_t
{
    std::vector<unsigned char>  uuid;
    std::vector<unsigned char>  uuid_reversed;

    service_uuid_t(const std::vector<unsigned char>  & uuid) : uuid(uuid), uuid_reversed(uuid.size())
    { std::reverse_copy(uuid.cbegin(), uuid.cend(), uuid_reversed.begin()); }

     service_uuid_t(const unsigned char uuid_arr_reversed[], const unsigned len)
        : uuid_reversed(uuid_arr_reversed, uuid_arr_reversed + len), uuid(len)
    {
        std::reverse_copy(uuid_reversed.cbegin(), uuid_reversed.cend(), uuid.begin());
    }

     std::string to_string() const
     {
         std::string uuid_str;
         for (auto q : uuid)
         {
             char b[3];
             sprintf(b, "%02X", (unsigned)q);
             uuid_str += b;
         }
         return uuid_str;
     }

     bool operator==(const service_uuid_t & x) { return x.uuid == uuid; }
};

service_uuid_t grasp_service_uuid  ({ 0xA5, 0x8F, 0xCF, 0xAE, 0xDB, 0x61, 0x11, 0xE2, 0xB9, 0xB0, 0xF2, 0x3C, 0x91, 0xAE, 0xC0, 0x5A }) ;
service_uuid_t primary_service_uuid({ 0x28, 0x00 });
//-----------------------------------------------------------------------------------------------------------------------------------------------------------//

std::string bd_addr_to_string(const bd_addr &a)
{
    std::string  addr_str;
    for (int i = 5; i >= 0; i--)
    {
        char b[4];
        sprintf(b, "%02x%s", a.addr[i], i ? ":" : "\n");
        addr_str += b;
    }
    return addr_str;
}


std::string get_device_name_from_scan_response(const ble_msg_gap_scan_response_evt_t *e)
{
    const int magic_device_name_start = 0x09;
    
    for (int i = 0; i < e->data.len;) {
        unsigned len = e->data.data[i++];
        if (!len) continue;
        if (i + len > e->data.len) break; // not enough data
        const unsigned char type = e->data.data[i++];

        if (type == magic_device_name_start)
            return std::string((char *)e->data.data + i, len - 1);

        i += len - 1;
    }
    return std::string();
}

//-----------------------------------------------------------------------------------------------------------------------------------------------------------//
enum { STATE_INIT = 0, STATE_DISCOVERING, STATE_CONNECTING, STATE_CONNECTED };

namespace
{
    bd_addr  remote_device_addr;
    uint8_t  connection_handle = 0;
    bool     grasp_uuid_found = false;

    StateMachine  sm;
};

ACTION(sm, STATE_INIT, StateMachine_StartEvent, e)                  { LOG(ble_cmd_gap_end_procedure()); /*stop prev op*/ }
ACTION(sm, STATE_INIT, ble_msg_gap_end_procedure_rsp_t, e)          { LOG(ble_cmd_gap_discover(gap_discover_observation)); }
ACTION(sm, STATE_INIT, ble_msg_gap_discover_rsp_t, e)               {
    ENSURE(e->result == 0, "Cannot start the Discover procedure");  
    sm.set_state(STATE_DISCOVERING); }


ACTION(sm, STATE_DISCOVERING, ble_msg_gap_scan_response_evt_t, e)   {
    remote_device_addr = e->sender;
    std::cout << bd_addr_to_string(remote_device_addr) << "\t" << (int)e->rssi << "\t" << get_device_name_from_scan_response(e) << std::endl;
    LOG(ble_cmd_gap_end_procedure());
}

ACTION(sm, STATE_DISCOVERING, ble_msg_gap_end_procedure_rsp_t, e)   {
    LOG(ble_cmd_gap_connect_direct(&remote_device_addr, gap_address_type_public, 40, 60, 100, 0));
}

ACTION(sm, STATE_DISCOVERING, ble_msg_gap_connect_direct_rsp_t, e)  {
    ENSURE(e->result == 0, "Cannot establish a direct connection");
    sm.set_state(STATE_CONNECTING);  }

ACTION(sm, STATE_CONNECTING, ble_msg_connection_disconnected_evt_t, e)  { LOG(sm.start(); ); }

ACTION(sm, STATE_CONNECTING, ble_msg_connection_status_evt_t, e)    {
    if (! (e->flags & connection_connected) )
        LOG(sm.start(););

    remote_device_addr = e->address;
    connection_handle = e->connection;
    std::cout << bd_addr_to_string(remote_device_addr) << std::endl;
    LOG(ble_cmd_attclient_read_by_group_type(connection_handle, 0x0001, 0xffff, 2, &primary_service_uuid.uuid_reversed.front() ));
}

ACTION(sm, STATE_CONNECTING, ble_msg_attclient_read_by_group_type_rsp_t, e) {
    ENSURE(e->result == 0, "Cannot start service discovery");
    grasp_uuid_found = false;
    sm.set_state(STATE_CONNECTED);
}

ACTION(sm, STATE_CONNECTED, ble_msg_connection_disconnected_evt_t, e){ LOG(sm.start();); }

ACTION(sm, STATE_CONNECTED, ble_msg_attclient_group_found_evt_t, e) {
    service_uuid_t  found_uuid(e->uuid.data, e->uuid.len);
    std::cout   <<   "\tFround uuid:\t" << found_uuid.to_string() 
                << "\n\tGrasp  uuid:\t" << grasp_service_uuid.to_string() << std::endl;
    grasp_uuid_found = grasp_uuid_found || (found_uuid == grasp_service_uuid);
}

ACTION(sm, STATE_CONNECTED, ble_msg_attclient_procedure_completed_evt_t, e) 
{ std::cout << "grasp uuid " << (grasp_uuid_found ? "found" : "not found") << std::endl; }


//---------------------------------------------------------------------------------------------------------------------------------------//
EVENT(sm, ble_rsp_gap_end_procedure,                ble_msg_gap_end_procedure_rsp_t);
EVENT(sm, ble_evt_connection_status, ble_msg_connection_status_evt_t);
EVENT(sm, ble_evt_connection_disconnected, ble_msg_connection_disconnected_evt_t);
EVENT(sm, ble_rsp_connection_disconnect, ble_msg_connection_disconnect_rsp_t);
EVENT(sm, ble_evt_gap_scan_response, ble_msg_gap_scan_response_evt_t);
EVENT(sm, ble_rsp_gap_discover, ble_msg_gap_discover_rsp_t);
EVENT(sm, ble_rsp_gap_connect_direct, ble_msg_gap_connect_direct_rsp_t);
EVENT(sm, ble_rsp_attclient_read_by_group_type, ble_msg_attclient_read_by_group_type_rsp_t);
EVENT(sm, ble_evt_attclient_group_found, ble_msg_attclient_group_found_evt_t);
EVENT(sm, ble_evt_attclient_procedure_completed, ble_msg_attclient_procedure_completed_evt_t);


int main(int argc, char *argv[] )
{    
    if ( argc < 2 ) exit(-1);

    static SerialPort  serial_port;

    bglib_output =  [](uint8_t len1, uint8_t* data1, uint16_t len2, uint8_t* data2)  {
        serial_port.Write(data1, len1);
        serial_port.Write(data2, len2);
    };

    serial_port.RestartInit(argv[1], []() { ble_cmd_system_reset(0); });

    sm.start();

    for (; ; )
    {
        ble_header api_header;
        serial_port.Read( (unsigned char *) &api_header, sizeof(api_header) );

        unsigned char  message_data[0xFF];
        serial_port.Read(message_data, api_header.lolen);

        const ble_msg * api_message = ble_get_msg_hdr(api_header);
        ENSURE(api_message != nullptr, "Unknown message received");
        api_message->handler(message_data);
    }
    return 0;
}
