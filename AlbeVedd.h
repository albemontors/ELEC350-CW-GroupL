//==============================================================
//        File edit by Alberto Montorsi and Vedhant Naik
//
//        This is main include file for the project
//==============================================================

#ifndef _ALBEVEDD_H
#define _ALBEVEDD_H

#include "mbed.h"
#include "rtos/ThisThread.h"
#include "NTPClient.h"
#include "azure_c_shared_utility/shared_util_options.h"
#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/tickcounter.h"
#include "azure_c_shared_utility/xlogging.h"
#include "iothubtransportmqtt.h"
#include "certs.h"
#include "iothub.h"
#include "iothub_client_options.h"
#include "iothub_device_client.h"
#include "iothub_message.h"
#include "azure_cloud_credentials.h"

#include "uop_msb.h"
#include <cstring>
#include <string.h>

//struct FormattedData {
//    char string[100];
//};

struct RawData {
    float temperature;
    float humidity;
    float lightLevel;
    float pressure;
    time_t timeStamp;
};

Queue<RawData*, 8> rawDataQueue;
MemoryPool<RawData, 8> rawData;        //vector of raw data to be processed (could be size sized 1)
Queue<RawData*, 16> sdDataQueue;
MemoryPool<RawData, 16> sdData;  //vector of formatted data for sd (bigger = less frequent writes)
Queue<RawData*, 8> iotDataQueue;
MemoryPool<RawData, 8> iotData;  //vector of formatted data for cloud (could be sized 1)

Ticker readNow;     //defines when to start reading
Thread readData;    //reads the data off the sensors
Thread dataHandle;  //handles the raw data and buffers them into SD and CLOUD DATA POOLS
Thread sdWrite;     //writes the data in SD DATA POOL to sd
Thread iotWrite;    //writes the data in CLOUD DATA POOL in cloud

extern NetworkInterface *_defaultSystemNetwork;

static bool message_received = false;

bool connect();
bool setTime();
void interruptRoutine();
void readDataF();
void dataHandleF();
void iotWriteF();
static void on_connection_status(IOTHUB_CLIENT_CONNECTION_STATUS result, IOTHUB_CLIENT_CONNECTION_STATUS_REASON reason, void* user_context);
static IOTHUBMESSAGE_DISPOSITION_RESULT on_message_received(IOTHUB_MESSAGE_HANDLE message, void* user_context);
static void on_message_sent(IOTHUB_CLIENT_CONFIRMATION_RESULT result, void* userContextCallback);
static int on_method_callback(const char* method_name, const unsigned char* payload, size_t size, unsigned char** response, size_t* response_size, void* userContextCallback);

#endif
