#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <windows.h>

#include <functional>
#include <iostream>
#include <map>
#include <cmath>
#include <string>

#pragma warning( disable : 4200 )

#include "cmd_def.h"
#include "serial_port.h"

#define CURLINE() __FUNCTION__ << ":" << __LINE__ << ":\t"

#define ENSURE(cmd, err)  do { if (!(cmd)) { \
    std::cout << "ERROR in " << CURLINE() << (err) << std::endl; throw BaseException(); } } while (0);

#define STATE_EVENT_NOT_IMPLEMENTED() do { std::cout << "ERROR in " << CURLINE() <<  " This event is not implemented in state " \
<< typeid(this).name() << std::endl; throw BaseException(); } while (0);

#define PRN(str) do { std::cout << CURLINE() << "\n\t" << #str << std::endl;  } while(0);
#define LOG(str) do { std::cout << CURLINE() << "\n\t" << #str << std::endl; str; } while(0);
#define LEV(op)  do { std::cout << CURLINE() << std::endl \
 << "\tState " << typeid(this).name() << "\n\tEvent " << typeid(e).name() << "\n\tAction "<< (#op)  \
    << std::endl << std::endl; \
op; } while(0);
#define TYPE(t)  do { std::cout << CURLINE() << "\n\t" << typeid(t).name() << std::endl; } while(0);
#define LOG_STATE(t)  do { std::cout << CURLINE() << "\n\t" << typeid(t).name() << std::endl << std::endl; } while(0);


struct BaseException {};


//-----------------------------------------------------------------------------------------------------------------------------------------------------------//

void print_bd_addr(const bd_addr &a)
{
    for (int i = 5; i >= 0; i--)
        printf("%02x%s", a.addr[i], i?":":"\n");
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

struct StartEvent {} start_event;

struct StateBase {
    static StateBase * cur_state;
    enum { STATE_BASE, STATE_INIT, STATE_DISCOVERING, STATE_CONNECTING, STATE_CONNECTED };
    static std::map<int, StateBase *> map_states;

    static void        set_state(const int e_state)                             { cur_state = map_states[e_state];  LOG_STATE(*cur_state); }
    static StateBase * get_state()                                              { return cur_state;    }
    template <typename TEvent>  static void process(const TEvent * event)       { TYPE(event); get_state()-> handle_event(event);  }
    static void start()                                                         { process(&start_event);  }

    virtual void handle_event(const StartEvent *)                               { STATE_EVENT_NOT_IMPLEMENTED(); }
    virtual void handle_event(const ble_msg_connection_disconnect_rsp_t *)      { STATE_EVENT_NOT_IMPLEMENTED(); }
    virtual void handle_event(const ble_msg_connection_disconnected_evt_t *)    { STATE_EVENT_NOT_IMPLEMENTED(); }
    virtual void handle_event(const ble_msg_connection_status_evt_t *)          { STATE_EVENT_NOT_IMPLEMENTED(); }
    virtual void handle_event(const ble_msg_gap_end_procedure_rsp_t *)          { STATE_EVENT_NOT_IMPLEMENTED(); }
    virtual void handle_event(const ble_msg_gap_discover_rsp_t      *)          { STATE_EVENT_NOT_IMPLEMENTED(); }
    virtual void handle_event(const ble_msg_gap_scan_response_evt_t *)          { STATE_EVENT_NOT_IMPLEMENTED(); }
    virtual void handle_event(const ble_msg_gap_connect_direct_rsp_t *)         { STATE_EVENT_NOT_IMPLEMENTED(); }
    virtual void handle_event(const ble_msg_attclient_read_by_group_type_rsp_t *)   { STATE_EVENT_NOT_IMPLEMENTED(); }
    virtual void handle_event(const ble_msg_attclient_group_found_evt_t *)      { STATE_EVENT_NOT_IMPLEMENTED(); }
    virtual void handle_event(const ble_msg_attclient_procedure_completed_evt_t *)  { STATE_EVENT_NOT_IMPLEMENTED(); }
protected:
    void new_state(const int e_state, StateBase * p_state)                      { map_states[e_state] = p_state; }
};

std::map<int, StateBase *> StateBase::map_states;


struct StateInit : public StateBase  {
    StateInit() { new_state(STATE_INIT, this); }

    void handle_event(const StartEvent                          *e) { LEV(ble_cmd_gap_end_procedure()); /*stop prev op*/ }
    void handle_event(const ble_msg_gap_end_procedure_rsp_t     *e) { LEV(ble_cmd_gap_discover(gap_discover_observation)); }   
    void handle_event(const ble_msg_gap_discover_rsp_t          *e) { 
        ENSURE(e->result == 0, "Cannot start the Discover procedure");
        set_state(STATE_DISCOVERING);
    }
} state_init;

struct StateDiscovering : public StateBase  {
    StateDiscovering() { new_state(STATE_DISCOVERING, this); }

    bd_addr  remote_device_addr;

    void handle_event(const ble_msg_gap_scan_response_evt_t     *e) {
        remote_device_addr = e->sender;
        print_bd_addr(remote_device_addr);
        std::cout << (int)e->rssi <<"\t"<< get_device_name_from_scan_response(e) << std::endl;
        LEV(ble_cmd_gap_end_procedure());
    }

    void handle_event(const ble_msg_gap_end_procedure_rsp_t     *e) {
        LEV(ble_cmd_gap_connect_direct(&remote_device_addr, gap_address_type_public, 40, 60, 100, 0));
    }

    void handle_event(const ble_msg_gap_connect_direct_rsp_t    *e) {
        LEV(ENSURE(e->result == 0, "Cannot establish a direct connection")); 
        set_state(STATE_CONNECTING);
    }

}  state_discovering;


struct StateConnecting : public StateBase  {
    StateConnecting() { new_state(STATE_CONNECTING, this); }

    uint8_t  connection_handle = 0;
    bd_addr  remote_device_addr;

    static uint8_t primary_service_uuid[];

    void handle_event(const ble_msg_connection_disconnected_evt_t *e) { LEV(ble_cmd_gap_discover(gap_discover_observation)); }

    void handle_event(const ble_msg_connection_status_evt_t     *e) { 
        if (e->flags & connection_connected)    {
            remote_device_addr = e->address;
            connection_handle = e->connection;
            print_bd_addr(remote_device_addr);
            LEV(ble_cmd_attclient_read_by_group_type(remote_device_addr, 0x0001, 0xffff, 2, primary_service_uuid));
        }
        else
            LEV(ble_cmd_gap_discover(gap_discover_observation));       
    }

    void handle_event(const ble_msg_gap_discover_rsp_t          *e) {
        ENSURE(e->result == 0, "Cannot start the Discover procedure");
        set_state(STATE_DISCOVERING);
    }

    void handle_event(const ble_msg_attclient_read_by_group_type_rsp_t *e) {
        ENSURE(e->result == 0, "Cannot start service discovery");
        set_state(STATE_CONNECTED);
    }
} state_connecting;


uint8_t StateConnecting :: primary_service_uuid[] = { 0x00, 0x28 };



struct StateConnected : public StateBase  {
    void handle_event(const ble_msg_connection_status_evt_t *) {  }

    void handle_event(const ble_msg_connection_disconnected_evt_t *e) {  LEV(set_state(STATE_INIT); process (&start_event));  }
    void handle_event(const ble_msg_attclient_group_found_evt_t *e)      {}
    void handle_event(const ble_msg_attclient_procedure_completed_evt_t *e)  { STATE_EVENT_NOT_IMPLEMENTED(); }

} state_connected;





StateBase* StateBase::cur_state = &state_init;


//---------------------------------------------------------------------------------------------------------------------------------------//
#define EVENT(callback, event_struct)  void callback (const struct event_struct *msg) { \
    PRN( event_struct );\
    StateBase::process(msg);  }

EVENT(ble_rsp_gap_end_procedure,                ble_msg_gap_end_procedure_rsp_t);
EVENT(ble_evt_connection_status,                ble_msg_connection_status_evt_t);
EVENT(ble_evt_connection_disconnected,          ble_msg_connection_disconnected_evt_t);
EVENT(ble_rsp_connection_disconnect,            ble_msg_connection_disconnect_rsp_t);
EVENT(ble_evt_gap_scan_response,                ble_msg_gap_scan_response_evt_t);
EVENT(ble_rsp_gap_discover,                     ble_msg_gap_discover_rsp_t);
EVENT(ble_rsp_gap_connect_direct,               ble_msg_gap_connect_direct_rsp_t);
EVENT(ble_rsp_attclient_read_by_group_type,     ble_msg_attclient_read_by_group_type_rsp_t);
EVENT(ble_evt_attclient_group_found,            ble_msg_attclient_group_found_evt_t);
EVENT(ble_evt_attclient_procedure_completed,    ble_msg_attclient_procedure_completed_evt_t);


int main(int argc, char *argv[] )
{    
    if ( argc < 2 ) exit(-1);

    static SerialPort  serial_port;

    bglib_output =  [](uint8_t len1, uint8_t* data1, uint16_t len2, uint8_t* data2)  {
        serial_port.Write(data1, len1);
        serial_port.Write(data2, len2);
    };

    serial_port.RestartInit(argv[1], []() { ble_cmd_system_reset(0); });

    StateBase::start();

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



