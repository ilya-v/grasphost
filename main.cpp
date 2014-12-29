#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <ctime>

#include <iostream>
#include <fstream>
#include <iomanip>
#include <vector>
#include <array>
#include <set>
#include <map>
#include <string>
#include <algorithm>
#include <sstream>
#include <thread>

#ifdef _MSC_VER
#pragma warning( disable : 4200 )
#endif

#include "cmd_def.h"
#include "serial_port.h"
#include "state_machine.h"
#include "server.h"
#include "key_filter.h"
#include "config_monitor.h"
#include "utils.h"

//-----------------------------------------------------------------------------------------------------------------------------------------------------------//

std::array<unsigned, 10>  default_filtering_f(const uint8_t levels[5]) {
    std::array<unsigned, 10>  filtered_levels{};
    std::transform(levels, levels + 5, filtered_levels.begin(), [](const uint8_t l){ return 0x7F - l; });
    return filtered_levels;
};

std::function<decltype(default_filtering_f)> on_kbd_data_f = default_filtering_f;
//-----------------------------------------------------------------------------------------------------------------------------------------------------------//

//uint16_t swab16(const uint16_t x) { return ((x & 0x00FF) << 8) | (x >> 8);  }

struct ble_uuid_t
{
    std::vector<unsigned char>      uuid,   uuid_reversed;
    
    ble_uuid_t(const std::vector<unsigned char>  & uuid) : uuid(uuid), uuid_reversed(uuid.size())
    { std::reverse_copy(uuid.cbegin(), uuid.cend(), uuid_reversed.begin()); }

    ble_uuid_t(const unsigned char uuid_arr_reversed[], const unsigned len)
        : uuid(len), uuid_reversed(uuid_arr_reversed, uuid_arr_reversed + len)
    { std::reverse_copy(uuid_reversed.cbegin(), uuid_reversed.cend(), uuid.begin()); }

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

     size_t                 size()                              const       { return uuid.size();  }
     const unsigned char *  get_reversed_bytes()                const       { return &uuid_reversed.front(); }
     bool                   operator==(const ble_uuid_t & x)    const       { return x.uuid == uuid; }
     bool                   operator< (const ble_uuid_t & x)    const       { return to_string() < x.to_string();  }
};

ble_uuid_t grasp_service_uuid       ({ 0xA5, 0x8F, 0xCF, 0xAE, 0xDB, 0x61, 0x11, 0xE2, 0xB9, 0xB0, 0xF2, 0x3C, 0x91, 0xAE, 0xC0, 0x5A }) ;
ble_uuid_t primary_service_uuid     ({ 0x28, 0x00 });
ble_uuid_t scan_result_char_uuid    ({ 0x33, 0x22, 0xF2, 0x4A, 0xDB, 0x73, 0x11, 0xE2, 0xB9, 0xB0, 0xF2, 0x3C, 0x91, 0xAE, 0xC0, 0x53 });
ble_uuid_t client_char_config_uuid  ({ 0x29, 0x02 });
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

bool operator<(const bd_addr &x, const bd_addr &y)
{
    for (int i = 5; i >= 0; i--)
    {
        if (x.addr[i] < y.addr[i]) return true;
        if (x.addr[i] > y.addr[i]) return false;
    }
    return false;
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


namespace
{
    struct char_group_t     { uint16_t first_group_handle, last_group_handle; };
    std::set<bd_addr>                   checked_devices;
    std::map<ble_uuid_t, char_group_t>  services_found;
    std::map<ble_uuid_t, uint16_t>      chars_found;
    bd_addr                             remote_device_addr;
    uint8_t                             connection_handle       = 0;
    DataServer                          data_server;

    std::ofstream  flog("log.txt");

    SM_DEFINE_WITH_STATES(sm, 
        STATE_INIT, STATE_DISCOVERING, STATE_CONNECTING, STATE_CONNECTED, STATE_ATTRIB_INFO_SEARCH, STATE_MONITORING);
};

SM_ACTION(sm, STATE_INIT, StateMachine_StartEvent, e)                  { LOG(ble_cmd_gap_end_procedure()); /*stop prev op*/ 
    std::this_thread::sleep_for(std::chrono::milliseconds(300)); }
SM_ACTION(sm, STATE_INIT, ble_msg_gap_end_procedure_rsp_t, e)          { LOG(ble_cmd_gap_discover(gap_discover_observation)); }
SM_ACTION(sm, STATE_INIT, ble_msg_gap_discover_rsp_t, e)               {
    ENSURE(e->result == 0, "Cannot start the Discover procedure");  
    checked_devices.clear();
    sm.set_state(STATE_DISCOVERING); }

SM_ACTION(sm, STATE_INIT, ble_msg_attclient_attribute_value_evt_t, e)  { sm.set_state(STATE_MONITORING); }

SM_ACTION(sm, STATE_DISCOVERING, ble_msg_gap_scan_response_evt_t, e)   {
    std::cout << "*** \t" << bd_addr_to_string(e->sender) << "\t" << (int)e->rssi << "\t" << get_device_name_from_scan_response(e);
    if (checked_devices.count(e->sender))
    {
        std::cout << "\tFound once again" << std::endl;
        return;        
    }
    remote_device_addr = e->sender;
    checked_devices.insert(e->sender);
    std::cout << std::endl;
    LOG(ble_cmd_gap_end_procedure());
}

SM_ACTION(sm, STATE_DISCOVERING, ble_msg_gap_end_procedure_rsp_t, e)   {
    LOG(ble_cmd_gap_connect_direct(&remote_device_addr, gap_address_type_public, 40, 60, 100, 0));
}

SM_ACTION(sm, STATE_DISCOVERING, ble_msg_gap_connect_direct_rsp_t, e)  {
    ENSURE(e->result == 0, "Cannot establish a direct connection");
    sm.set_state(STATE_CONNECTING);  }

SM_ACTION(sm, STATE_CONNECTING,  ble_msg_connection_disconnected_evt_t, e)  { LOG(sm.start(); ); }
//SM_ACTION(sm, STATE_DISCOVERING, ble_msg_connection_disconnected_evt_t, e)  { LOG(sm.start();); }

SM_ACTION(sm, STATE_CONNECTING, ble_msg_connection_status_evt_t, e)    {
    if (!(e->flags & connection_connected))
    {
        LOG(sm.start(););
        return;
    }
    remote_device_addr = e->address;
    connection_handle = e->connection;
    std::cout << "*** \tChecking the  device " << bd_addr_to_string(remote_device_addr) << "for the primary service" << std::endl;
    std::cout << "h" << (int)connection_handle << " sz" << primary_service_uuid.size() << " bt" << (unsigned)primary_service_uuid.get_reversed_bytes()[0] << "." << (unsigned)primary_service_uuid.get_reversed_bytes()[1] << std::endl;
    LOG(ble_cmd_attclient_read_by_group_type(connection_handle, 0x0001, 0xffff, primary_service_uuid.size(), primary_service_uuid.get_reversed_bytes() ));
}

SM_ACTION(sm, STATE_CONNECTING, ble_msg_attclient_read_by_group_type_rsp_t, e) {
    std::cout << "e" << sizeof(*e) << " " << ((char*)&e->result - (char*)e) << " " << *(uint32_t*)e << std::endl;

    ENSURE(e->result == 0, "Cannot start service discovery");
    std::cout << e->result << " : "  << (int)e->connection << " : sz " << sizeof(e) << std::endl;
    sm.set_state(STATE_CONNECTED);
}

SM_ACTION(sm, STATE_CONNECTED, ble_msg_connection_disconnected_evt_t, e){ LOG(sm.start();); }

SM_ACTION(sm, STATE_CONNECTED, ble_msg_attclient_group_found_evt_t, e) {
    ble_uuid_t  found_uuid(e->uuid.data, e->uuid.len); 
    std::cout   <<   "***\tFound uuid:\t"  << found_uuid.to_string() 
                << "\n***\tGrasp  uuid:\t" << grasp_service_uuid.to_string() << std::endl;
    services_found [found_uuid] = char_group_t{ e->start, e->end };
}

SM_ACTION(sm, STATE_CONNECTED, ble_msg_attclient_procedure_completed_evt_t, e)  { 
    std::cout << "*** \tgrasp service with uuid " << grasp_service_uuid.to_string() << (services_found.count(grasp_service_uuid) ? " found" : "not found") << std::endl;
    if (services_found.count(grasp_service_uuid))
    {
        chars_found.clear();
        LOG(ble_cmd_attclient_find_information(e->connection, services_found[grasp_service_uuid].first_group_handle, services_found[grasp_service_uuid].last_group_handle));
    }
    else  sm.start();
}

SM_ACTION(sm, STATE_CONNECTED, ble_msg_attclient_find_information_rsp_t, e)  {
    ENSURE(e->result == 0, "Cannot get attribute handles info");
    sm.set_state(STATE_ATTRIB_INFO_SEARCH);
}

SM_ACTION(sm, STATE_ATTRIB_INFO_SEARCH, ble_msg_attclient_find_information_found_evt_t, e)  {
    chars_found[ ble_uuid_t(e->uuid.data, e->uuid.len) ] = e->chrhandle;
    std::cout << "*** \tCharacterisitics found: " << ble_uuid_t(e->uuid.data, e->uuid.len).to_string() << std::endl;
}

SM_ACTION(sm, STATE_ATTRIB_INFO_SEARCH, ble_msg_attclient_procedure_completed_evt_t, e)  {
    std::cout << "*** \tCharacteristics search finished" << std::endl;
    std::cout << "*** \tScan result char with uuid "    << scan_result_char_uuid   .to_string() << (chars_found.count(scan_result_char_uuid  )? " found" : " not found") << std::endl;
    std::cout << "*** \tClient config char with uuid "  << client_char_config_uuid .to_string() << (chars_found.count(client_char_config_uuid)? " found" : " not found") << std::endl;

    if (!chars_found.count(scan_result_char_uuid) || !chars_found.count(client_char_config_uuid))
    {
        sm.start();
        return;
    }
    
    uint8_t notification_config[] = { 0x01, 0x00 };
    ble_cmd_attclient_attribute_write(e->connection, chars_found[client_char_config_uuid], 2, notification_config);
}

SM_ACTION(sm, STATE_ATTRIB_INFO_SEARCH, ble_msg_attclient_attribute_write_rsp_t, e)  {
    ENSURE(e->result == 0, "Cannot write config attribute");
    sm.logging_off();
    sm.set_state(STATE_MONITORING);
}

SM_ACTION(sm, STATE_ATTRIB_INFO_SEARCH, ble_msg_attclient_find_information_rsp_t, e)  {
    std::cout << "*** \tWhy ble_msg_attclient_find_information_rsp_t in STATE_ATTRIB_INFO_SEARCH?!" << std::endl;
    ENSURE(e->result == 0, "Error message from the module; check arguments for attclient_find_info");
}

SM_ACTION(sm, STATE_ATTRIB_INFO_SEARCH, ble_msg_connection_disconnected_evt_t, e){ LOG(sm.start();); }

SM_ACTION(sm, STATE_MONITORING, ble_msg_attclient_procedure_completed_evt_t,  e)  {  ENSURE(e->result == 0, "Cannot write config attribute");    }

SM_ACTION(sm, STATE_MONITORING, ble_msg_attclient_attribute_value_evt_t, e)
{
    sm.logging_off();

    std::array<unsigned, 10> filtered_data  =  on_kbd_data_f(e->value.data);
    std::cout << "*** \t" << std::setfill('0');
    const uint8_t *d = e->value.data;
    for (auto x : std::array<unsigned, 5>({ { d[0], d[1], d[2], d[3], d[4] } }))
        std::cout << std::hex << std::setw(2) << (unsigned)x << ".";

    auto ticks = clock();
    std::cout << std::setfill(' ') << std::dec << "\t" << ticks << "\t" << CLOCKS_PER_SEC << std::endl;

    std::string data_str;
    for (auto x: filtered_data)
        data_str += to_string(x) + " ";

    data_str += to_string(ticks);
    flog << data_str << std::endl;
    data_server.SendData(data_str + "\n");
}

SM_ACTION(sm, STATE_MONITORING, ble_msg_connection_disconnected_evt_t, e){ LOG(sm.start();); }
SM_ACTION(sm, STATE_MONITORING, ble_msg_gap_end_procedure_rsp_t, e)      {}
SM_ACTION(sm, STATE_MONITORING, ble_msg_gap_discover_rsp_t, e)      {}


//---------------------------------------------------------------------------------------------------------------------------------------//
SM_EVENT(sm, ble_rsp_gap_end_procedure,                 ble_msg_gap_end_procedure_rsp_t                     );
SM_EVENT(sm, ble_evt_connection_status,                 ble_msg_connection_status_evt_t                     );
SM_EVENT(sm, ble_evt_connection_disconnected,           ble_msg_connection_disconnected_evt_t               );
SM_EVENT(sm, ble_rsp_connection_disconnect,             ble_msg_connection_disconnect_rsp_t                 );
SM_EVENT(sm, ble_evt_gap_scan_response,                 ble_msg_gap_scan_response_evt_t                     );
SM_EVENT(sm, ble_rsp_gap_discover,                      ble_msg_gap_discover_rsp_t                          );
SM_EVENT(sm, ble_rsp_gap_connect_direct,                ble_msg_gap_connect_direct_rsp_t                    );
SM_EVENT(sm, ble_rsp_attclient_read_by_group_type,      ble_msg_attclient_read_by_group_type_rsp_t          );
SM_EVENT(sm, ble_evt_attclient_group_found,             ble_msg_attclient_group_found_evt_t                 );
SM_EVENT(sm, ble_evt_attclient_procedure_completed,     ble_msg_attclient_procedure_completed_evt_t         );
SM_EVENT(sm, ble_rsp_attclient_find_information,        ble_msg_attclient_find_information_rsp_t            );
SM_EVENT(sm, ble_evt_attclient_find_information_found,  ble_msg_attclient_find_information_found_evt_t      );
SM_EVENT(sm, ble_evt_attclient_attribute_value,         ble_msg_attclient_attribute_value_evt_t             );
SM_EVENT(sm, ble_rsp_attclient_attribute_write,         ble_msg_attclient_attribute_write_rsp_t             );



int main(int argc, char *argv[] )
{    
    std::string port_name;
    
    if (argc < 2) {
        auto serial_port_names = ScanSerialPorts();
        std::cout << "Available COM ports:" << std::endl;
        for (auto &n : serial_port_names)
             std::cout << "\t" << n << std::endl;
        std::cout << "." << std::endl;
        if (serial_port_names.empty())
        {
            std::cout << "No available COM ports found. Press Enter." << std::endl;
            getc(stdin);
            return 1;
        }
        if (serial_port_names.size() == 1)
            port_name = serial_port_names.front();
    }
    else {
        port_name = std::string(argv[1]);
    }

    std::cout << "Using serial port " << port_name << std::endl;

    static SerialPort  serial_port;

    bglib_output = [](uint8_t len1, uint8_t* data1, uint16_t len2, uint8_t* data2)  {
       if (len1)
            serial_port.Write(data1, len1);
       if (len2)
            serial_port.Write(data2, len2);
    };

    serial_port.RestartInit(port_name.c_str(), []() { /*ble_cmd_system_reset(0); Sleep(5000);*/ });

    std::string config_file_name = std::string(argv[0]);
    {
        auto idx_extension = config_file_name.rfind(".exe");
        if (idx_extension != std::string::npos)
            config_file_name.erase(idx_extension);
        config_file_name += ".ini";
    }

    sm.start();

    for (config_t config; ; )
    {
        const config_t new_config = get_config(config_file_name);
        if (new_config.is_config_set && new_config != config) {
            config = new_config;
            on_kbd_data_f = [=](const uint8_t levels[5]) { return process_sensor_levels(config, levels); };
            std::cout << "Using the config from " << config_file_name << std::endl;
        }

        ble_header api_header;
        serial_port.Read( (unsigned char *) &api_header, sizeof(api_header) );

        unsigned char  message_data[0xFF];
        serial_port.Read(message_data, api_header.lolen);

        const ble_msg * api_message = ble_get_msg_hdr(api_header);
        ENSURE(api_message != nullptr, "Unknown message received");
        api_message->handler(message_data);
    }
}
