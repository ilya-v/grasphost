#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

#include "cmd_def.h"

#define ENSURE(cmd, err) do { \
        if(!(cmd)) {  printf("%s: %d\n", (err),(int)GetLastError());  exit(-1);  }\
} while(0);


volatile HANDLE serial_handle;

void output(uint8 len1,uint8* data1,uint16 len2,uint8* data2)
{
    DWORD written;
    ENSURE(WriteFile (serial_handle, data1, len1, &written, NULL),  "ERROR: Writing data.");
    ENSURE(WriteFile (serial_handle, data2, len2, &written, NULL),  "ERROR: Writing data.");
}

int read_message()
{
    DWORD rread;
    const struct ble_msg *apimsg;
    struct ble_header apihdr;
    unsigned char data[256];//enough for BLE

    //read header
    if(!ReadFile(serial_handle, (unsigned char*)&apihdr, 4, &rread, NULL))
        return GetLastError();

    if(!rread)  return 0;

    //read rest if needed
    if(apihdr.lolen)
    {
        if(!ReadFile(serial_handle, data, apihdr.lolen, &rread, NULL))
            return GetLastError();
    }

    apimsg=ble_get_msg_hdr (apihdr);
    if(!apimsg)
    {
        printf("ERROR: Message not found:%d:%d\n",(int)apihdr.cls,(int)apihdr.command);
        return -1;
    }
    apimsg->handler(data);

    return 0;
}

int main(int argc, char *argv[] )
{
    char str[80];

    bglib_output = output;

    if(argc<2)  exit(-1);

    snprintf(str,sizeof(str)-1,"\\\\.\\%s",argv[1]);
    serial_handle = CreateFileA(str,
                                   GENERIC_READ | GENERIC_WRITE,
                                   FILE_SHARE_READ|FILE_SHARE_WRITE,
                                   NULL,
                                   OPEN_EXISTING,
                                   FILE_ATTRIBUTE_NORMAL,
                                   NULL);


    ENSURE (serial_handle != INVALID_HANDLE_VALUE, "Error opening serial port");

    //stop previous operation
    ble_cmd_gap_end_procedure();
    //get connection status,current command will be handled in response
    ble_cmd_connection_get_status(0);

    for (; !read_message(); );
    return 0;
}
