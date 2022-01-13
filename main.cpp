/*
 * Copyright (c) 2020 Arm Limited
 * SPDX-License-Identifier: Apache-2.0
 */

#include "AlbeVedd.h"

int main() {

    //MAIN ONLY CONTAINS A BUCH OF INIT
    LogInfo("debug step 1");
    //readData.set_priority(osPriorityRealtime);
    readData.start(readDataF);
    dataHandle.start(dataHandleF);
    iotWrite.start(iotWriteF);
    readNow.attach(interruptRoutine, 10s);

    while(1); //that's all folks
    
}

//FUNCTIONS

void interruptRoutine(){

    readData.flags_set(1);

}

void readDataF(){

    //---- INIT ----
    uop_msb::EnvSensor sensor;

    RawData data;

    AnalogIn light = AnalogIn(PC_0);
    
    //NOW REPEAT FOREVER AND EVER ALLELUHIA
    while(1){
        //WAIT FOR INTERRUPT
        ThisThread::flags_wait_any(1); //sleep untill ticked by the timer interrupt
        ThisThread::flags_clear(1);
        //#TODO add watchdog

        data.temperature = sensor.getTemperature();
        data.humidity = sensor.getHumidity();
        data.pressure = sensor.getPressure();
        data.lightLevel = 10.0f * (1.0f - light.read());
        
        RawData* dataSpace = rawData.try_alloc();
        *dataSpace = data;
        rawDataQueue.try_put(&dataSpace);
        LogInfo("WRITTEN DATA");
    }
}

void dataHandleF(){

    //----- INIT -----

    int fail;
    RawData** getData;
    RawData data;
    //FormattedData iotString;
    //FormattedData sdString;

    //----- LOOP -----
    while(1){

        if(fail > 10) /* sas */; //#TODO detect problem for failed 10 in a row
        if(!rawDataQueue.try_get_for(20s, &getData)) { fail++; continue; }//if 20 s limit back to top (use of the forbidden function)
        fail = 0;
        data = **getData;
        rawData.free(*getData);

        printf("temp= %f, humid= %f, press= %f, light= %f \n", data.temperature, data.humidity, data.pressure, data.lightLevel);
        LogInfo("caught data");
        

        RawData* iotDataSpace = iotData.try_alloc();
        *iotDataSpace = data;
        iotDataQueue.try_put(&iotDataSpace);

    }

}

bool connect()
{
    LogInfo("Connecting to the network");

    _defaultSystemNetwork = NetworkInterface::get_default_instance();
    if (_defaultSystemNetwork == nullptr) {
        LogError("No network interface found");
        return false;
    }

    int ret = _defaultSystemNetwork->connect();
    if (ret != 0) {
        LogError("Connection error: %d", ret);
        return false;
    }
    LogInfo("Connection success, MAC: %s", _defaultSystemNetwork->get_mac_address());
    return true;
}

bool setTime()
{
    LogInfo("Getting time from the NTP server");

    NTPClient ntp(_defaultSystemNetwork);
    ntp.set_server("time.google.com", 123);
    time_t timestamp = ntp.get_timestamp();
    if (timestamp < 0) {
        LogError("Failed to get the current time, error: %ud", timestamp);
        return false;
    }
    LogInfo("Time: %s", ctime(&timestamp));
    printf("Time: %s", ctime(&timestamp));
    set_time(timestamp);
    return true;
}

void iotWriteF(){
    
    bool trace_on = MBED_CONF_APP_IOTHUB_CLIENT_TRACE;
    tickcounter_ms_t interval = 100;
    IOTHUB_CLIENT_RESULT res;

    LogInfo("Initializing IoT Hub client");
    IoTHub_Init();
    LogInfo("Init well done");


    IOTHUB_DEVICE_CLIENT_HANDLE client_handle = IoTHubDeviceClient_CreateFromConnectionString(
        azure_cloud::credentials::iothub_connection_string,
        MQTT_Protocol
    );
    if (client_handle == nullptr) {
        LogError("Failed to create IoT Hub client handle");
        goto cleanup;
    }

    // Enable SDK tracing
    res = IoTHubDeviceClient_SetOption(client_handle, OPTION_LOG_TRACE, &trace_on);
    if (res != IOTHUB_CLIENT_OK) {
        LogError("Failed to enable IoT Hub client tracing, error: %d", res);
        goto cleanup;
    }

    // Enable static CA Certificates defined in the SDK
    res = IoTHubDeviceClient_SetOption(client_handle, OPTION_TRUSTED_CERT, certificates);
    if (res != IOTHUB_CLIENT_OK) {
        LogError("Failed to set trusted certificates, error: %d", res);
        goto cleanup;
    }

    // Process communication every 100ms
    res = IoTHubDeviceClient_SetOption(client_handle, OPTION_DO_WORK_FREQUENCY_IN_MS, &interval);
    if (res != IOTHUB_CLIENT_OK) {
        LogError("Failed to set communication process frequency, error: %d", res);
        goto cleanup;
    }

    // set incoming message callback
    res = IoTHubDeviceClient_SetMessageCallback(client_handle, on_message_received, nullptr);
    if (res != IOTHUB_CLIENT_OK) {
        LogError("Failed to set message callback, error: %d", res);
        goto cleanup;
    }

    // Set incoming command callback
    res = IoTHubDeviceClient_SetDeviceMethodCallback(client_handle, on_method_callback, nullptr);
    if (res != IOTHUB_CLIENT_OK) {
        LogError("Failed to set method callback, error: %d", res);
        goto cleanup;
    }

    // Set connection/disconnection callback
    res = IoTHubDeviceClient_SetConnectionStatusCallback(client_handle, on_connection_status, nullptr);
    if (res != IOTHUB_CLIENT_OK) {
        LogError("Failed to set connection status callback, error: %d", res);
        goto cleanup;
    }

    RawData** getData;
    RawData iotDataIn;
    IOTHUB_MESSAGE_HANDLE message_handle;
    char message[100];

    LogInfo("FINISHED INIT IOT");

    //ThisThread::sleep_for(10s);

    LogInfo("NOW AWAKE");

    while(1){

        // Send ten message to the cloud
        // or until we receive a message from the cloud
        
        
        //if (message_received) {
            // If we have received a message from the cloud, don't send more messeges
            //break;
        //}
        //Send data in this format:
        /*
            {
                "LightLevel" : 0.12,
                "Temperature" : 36.0
            }
        */
        LogInfo("Extracting data");
        if(!iotDataQueue.try_get(&getData)) LogInfo("no data in buffer");
            else{
            iotDataIn = **getData;
            iotData.free(*getData);
            LogInfo("Data extracted");

            sprintf(message, "{ \"LightLevel\" : %5.2f, \"Temperature\" : %5.2f } ", iotDataIn.lightLevel, iotDataIn.temperature);
            printf("%s \n", message);
            
            //LogInfo("Sending: \"%s\"", message);
            message_handle = IoTHubMessage_CreateFromString(message);
            if (message_handle == nullptr) {
                LogError("Failed to create message");
                goto cleanup;
            }
            res = IoTHubDeviceClient_SendEventAsync(client_handle, message_handle, on_message_sent, nullptr);
            IoTHubMessage_Destroy(message_handle); // message already copied into the SDK
            if (res != IOTHUB_CLIENT_OK) {
                LogError("Failed to send message event, error: %d", res);
                goto cleanup;
            }


        cleanup:
        //IoTHubDeviceClient_Destroy(client_handle);
        //IoTHub_Deinit();
        LogInfo("restarting loop");

        }
    }
}

static void on_connection_status(IOTHUB_CLIENT_CONNECTION_STATUS result, IOTHUB_CLIENT_CONNECTION_STATUS_REASON reason, void* user_context)
{
    if (result == IOTHUB_CLIENT_CONNECTION_AUTHENTICATED) {
        LogInfo("Connected to IoT Hub");
    } else {
        LogError("Connection failed, reason: %s", MU_ENUM_TO_STRING(IOTHUB_CLIENT_CONNECTION_STATUS_REASON, reason));
    }
}

// **************************************
// * MESSAGE HANDLER (no response sent) *
// **************************************

static IOTHUBMESSAGE_DISPOSITION_RESULT on_message_received(IOTHUB_MESSAGE_HANDLE message, void* user_context)
{
    LogInfo("Message received from IoT Hub");

    const unsigned char *data_ptr;
    size_t len;
    if (IoTHubMessage_GetByteArray(message, &data_ptr, &len) != IOTHUB_MESSAGE_OK) {
        LogError("Failed to extract message data, please try again on IoT Hub");
        return IOTHUBMESSAGE_ABANDONED;
    }

    message_received = true;
    LogInfo("Message body: %.*s", len, data_ptr);

    if (strncmp("true", (const char*)data_ptr, len) == 0) {
        //led2 = 1;
    } else {
        //led2 = 0;
    }

    return IOTHUBMESSAGE_ACCEPTED;
}

static void on_message_sent(IOTHUB_CLIENT_CONFIRMATION_RESULT result, void* userContextCallback)
{
    if (result == IOTHUB_CLIENT_CONFIRMATION_OK) {
        LogInfo("Message sent successfully");
    } else {
        LogInfo("Failed to send message, error: %s",
            MU_ENUM_TO_STRING(IOTHUB_CLIENT_CONFIRMATION_RESULT, result));
    }
}

// ****************************************************
// * COMMAND HANDLER (sends a response back to Azure) *
// ****************************************************

static int on_method_callback(const char* method_name, const unsigned char* payload, size_t size, unsigned char** response, size_t* response_size, void* userContextCallback)
{
    const char* device_id = (const char*)userContextCallback;

    printf("\r\nDevice Method called for device %s\r\n", device_id);
    printf("Device Method name:    %s\r\n", method_name);
    printf("Device Method payload: %.*s\r\n", (int)size, (const char*)payload);

    if ( strncmp("true", (const char*)payload, size) == 0 ) {
        printf("LED ON\n");
        //led1 = 1;
    } else {
        printf("LED OFF\n");
        //led1 = 0;
    }

    int status = 200;
    //char RESPONSE_STRING[] = "{ \"Response\": \"This is the response from the device\" }";
    char RESPONSE_STRING[64];
    sprintf(RESPONSE_STRING, "{ \"Response\" : %d }", 10);

    printf("\r\nResponse status: %d\r\n", status);
    printf("Response payload: %s\r\n\r\n", RESPONSE_STRING);

    int rlen = strlen(RESPONSE_STRING);
    *response_size = rlen;
    if ((*response = (unsigned char*)malloc(rlen)) == NULL) {
        status = -1;
    }
    else {
        memcpy(*response, RESPONSE_STRING, *response_size);
    }
    return status;
}
