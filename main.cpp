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
 << "\tState " << typeid(*this).name() << "\n\tEvent " << typeid(*e).name() << "\n\tAction "<< (#op)  \
    << std::endl << std::endl; \
op; } while(0);
#define TYPE(t)  do { std::cout << CURLINE() << "\n\t" << typeid(t).name() << std::endl; } while(0);

//-----------------------------------------------------------------------------------------------------------------------------------------------------------//

struct BaseException {};

//-----------------------------------------------------------------------------------------------------------------------------------------------------------//

const unsigned char grasp_service_uuid[] { 0xA5, 0x8F, 0xCF, 0xAE, 0xDB, 0x61, 0x11, 0xE2, 0xB9, 0xB0, 0xF2, 0x3C, 0x91, 0xAE, 0xC0, 0x5A };

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

std::string get_uuid_str(const uint8array &uuid)
{
    std::string uuid_str;
    if (uuid.len)
        for (const unsigned char *p = uuid.data + uuid.len - 1; p >= uuid.data; p--)
        {
            char b[3];
            sprintf(b, "%02X", (unsigned)*p);
            uuid_str += b;
        }
    return uuid_str;    
}

bool are_uuids_equal(const uint8array &u, const uint8array &v)
{
    return u.len == v.len && 0 == memcmp(u.data, v.data, u.len);
}

//-----------------------------------------------------------------------------------------------------------------------------------------------------------//

struct StartEvent {} start_event;

struct BaseState {
    static BaseState * cur_state;
    enum { STATE_BASE, STATE_INIT, STATE_DISCOVERING, STATE_CONNECTING, STATE_CONNECTED };
    static std::map<int, BaseState *> map_states;

    static void        set_state(const int e_state)                             { cur_state = map_states[e_state]; 
                                                                                  std::cout << CURLINE() << "New state: " << typeid(*cur_state).name() << std::endl << std::endl; }
    static BaseState * get_state()                                              { return cur_state;    }
    template <typename TEvent>  static void process(const TEvent * event)       { std::cout << CURLINE() << "State " << typeid(*cur_state).name() << "\tEvent " << typeid(*event).name() << std::endl;
                                                                                  get_state()->handle_event(event); }
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
    void new_state(const int e_state, BaseState * p_state)                      { map_states[e_state] = p_state; }
};

std::map<int, BaseState *> BaseState::map_states;


struct InitState : public BaseState  {
    InitState() { new_state(STATE_INIT, this); }

    void handle_event(const StartEvent                          *e)     { LEV(ble_cmd_gap_end_procedure()); /*stop prev op*/ }
    void handle_event(const ble_msg_gap_end_procedure_rsp_t     *e)     { LEV(ble_cmd_gap_discover(gap_discover_observation)); }   
    void handle_event(const ble_msg_gap_discover_rsp_t          *e)     { ENSURE(e->result == 0, "Cannot start the Discover procedure");   set_state(STATE_DISCOVERING);    }
} state_init;

struct DiscoveringState : public BaseState  {
    DiscoveringState() { new_state(STATE_DISCOVERING, this); }

    bd_addr  remote_device_addr;

    void handle_event(const ble_msg_gap_scan_response_evt_t     *e)     {
        remote_device_addr = e->sender;
        print_bd_addr(remote_device_addr);
        std::cout << (int)e->rssi <<"\t"<< get_device_name_from_scan_response(e) << std::endl;
        LEV(ble_cmd_gap_end_procedure());
    }

    void handle_event(const ble_msg_gap_end_procedure_rsp_t     *e)     { LEV(ble_cmd_gap_connect_direct(&remote_device_addr, gap_address_type_public, 40, 60, 100, 0));  }
    void handle_event(const ble_msg_gap_connect_direct_rsp_t    *e)     { LEV(ENSURE(e->result == 0, "Cannot establish a direct connection")); set_state(STATE_CONNECTING); }
}  state_discovering;


struct ConnectingState : public BaseState  {
    ConnectingState() { new_state(STATE_CONNECTING, this); }

    uint8_t  connection_handle = 0;
    bd_addr  remote_device_addr;

    static uint8_t primary_service_uuid[];

    void handle_event(const ble_msg_connection_disconnected_evt_t *e)   { LEV(ble_cmd_gap_discover(gap_discover_observation)); }

    void handle_event(const ble_msg_connection_status_evt_t     *e)     { 
        if (e->flags & connection_connected)    {
            remote_device_addr = e->address;
            connection_handle = e->connection;
            print_bd_addr(remote_device_addr);
            LEV(ble_cmd_attclient_read_by_group_type(connection_handle, 0x0001, 0xffff, 2, primary_service_uuid));
        }
        else
            LEV(ble_cmd_gap_discover(gap_discover_observation));       
    }
    void handle_event(const ble_msg_gap_discover_rsp_t          *e)     { ENSURE(e->result == 0, "Cannot start the Discover procedure"); set_state(STATE_DISCOVERING);  }
    void handle_event(const ble_msg_attclient_read_by_group_type_rsp_t *e) { ENSURE(e->result == 0, "Cannot start service discovery"); set_state(STATE_CONNECTED);      }
} state_connecting;

uint8_t ConnectingState :: primary_service_uuid[] = { 0x00, 0x28 };


struct ConnectedState : public BaseState  {
    ConnectedState() { new_state(STATE_CONNECTED, this); }
    
    bool grasp_uuid_found = false;
//    std::array<16, >
    /*
    unsigned char my_grasp_uuid[16] { grasp_service_uuid };
    const uint8array  grasp_uuid{ 16, std::reverse(grasp_service_uuid) };

    void handle_event(const ble_msg_connection_disconnected_evt_t *e)   { LEV(set_state(STATE_INIT); process (&start_event));  }
    void handle_event(const ble_msg_attclient_group_found_evt_t *e)     { std::cout << "\tuuid:\t" << get_uuid_str(e->uuid) << "\n\tGrasp uuid:\t" << get_uuid_str(grasp_service_uuid) << std::endl;  
        grasp_uuid_found = grasp_uuid_found || are_uuids_equal(e->uuid, grasp_service_uuid); }
    void handle_event(const ble_msg_attclient_procedure_completed_evt_t *e)  { std::cout << "grasp uuid " << (grasp_uuid_found? "found" : "not found") << std::endl; }
    */
} state_connected;





BaseState* BaseState::cur_state = &state_init;


//---------------------------------------------------------------------------------------------------------------------------------------//
#define EVENT(callback, event_struct)  void callback (const struct event_struct *msg) { \
    PRN( event_struct );\
    BaseState::process(msg);  }

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

    BaseState::start();

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



