#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

#include "cmd_def.h"

#define ENSURE(cmd, err) do { \
        if(!(cmd)) {  printf("%s: %d\n", (err),(int)GetLastError());  exit(-1);  } else if (0) {printf("%s: OK\n", #cmd);}\
} while(0);


namespace
{
    HANDLE serial_handle;
}

void output(uint8 len1, uint8* data1, uint16 len2, uint8* data2)
{
    DWORD written;
    ENSURE(WriteFile (serial_handle, data1, len1, &written, NULL),  "ERROR: Writing data.1");
    ENSURE(WriteFile (serial_handle, data2, len2, &written, NULL),  "ERROR: Writing data.2");
}

int read_message()
{
    DWORD rread;
    struct ble_header apihdr;
    if(!ReadFile(serial_handle, (unsigned char*)&apihdr, 4, &rread, NULL))  //read header
        return GetLastError();

    if(!rread)  return 0;

    //read rest if needed
    unsigned char data[256];//enough for BLE
    if(apihdr.lolen)
        if(!ReadFile(serial_handle, data, apihdr.lolen, &rread, NULL))
            return GetLastError();

    const ble_msg * apimsg = ble_get_msg_hdr (apihdr);
    if(!apimsg)
    {
        printf("ERROR: Message not found:%d:%d\n",(int)apihdr.cls,(int)apihdr.command);
        return -1;
    }
    apimsg->handler(data);
    return 0;
}

HANDLE open_serial_port (const char *port_name_arg)
{
    char serial_port_name[80];
    HANDLE serial_handle = INVALID_HANDLE_VALUE;
    snprintf(serial_port_name, sizeof(serial_port_name)-1, "\\\\.\\%s", port_name_arg);
    
    for (int attempt_count = 20; attempt_count > 0 && serial_handle == INVALID_HANDLE_VALUE; attempt_count --)
    {
        Sleep(500);    
        serial_handle  =  CreateFileA (serial_port_name, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    } 
    return  serial_handle;
}

void close_serial_port(HANDLE serial_handle)
{
    CloseHandle(serial_handle);    
}

//-----------------------------------------------------------------------------------------------------------------------------------------------------------//

class state_machine
{
    enum state_t { not_connected, discovered,  };
    state_t  my_state = not_connected;
public:
    explicit state_machine () {}
};

void ble_evt_connection_status(const struct ble_msg_connection_status_evt_t *msg)
{
    if(msg->flags & connection_connected)
    {
        printf("#connected -> disconnect\n");
        ble_cmd_connection_disconnect(msg->connection);
    }else
    {
        printf("#Not connected -> Scan\n");
        ble_cmd_gap_discover(gap_discover_observation);
    }
}

void ble_evt_gap_scan_response(const struct ble_msg_gap_scan_response_evt_t *msg)
{
    int i;
    for(i=0;i<6;i++)
        printf("%02x%s",msg->sender.addr[5-i],i<5?":":"");
    printf("\t%d\n",msg->rssi);
}

int main(int argc, char *argv[] )
{
    bglib_output = output;
    if ( argc < 2 )  exit(-1);
    
    serial_handle = open_serial_port( argv[1] );
    ble_cmd_system_reset(0);
    close_serial_port(serial_handle);
    
    serial_handle = open_serial_port( argv[1] );
    ENSURE (serial_handle != INVALID_HANDLE_VALUE, "Error opening serial port");

    ble_cmd_gap_end_procedure();         //stop previous operation
    
    
    
    
    ble_cmd_connection_get_status(0);    //get connection status,current command will be handled in response
    
    
    //ble_cmd_gap_discover(gap_discover_observation);

    for (; !read_message(); )
    {
    
    }
    return 0;
}
