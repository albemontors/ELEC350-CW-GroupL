/*
 * Copyright (c) 2020 Arm Limited
 * SPDX-License-Identifier: Apache-2.0
 */

#include "AlbeVedd.h"

extern void azureInit(IOTHUB_DEVICE_CLIENT_HANDLE client_handle);
extern void sendMessage(float light, float temp, IOTHUB_DEVICE_CLIENT_HANDLE client_handle);


Mutex locker;

int main() {

    //MAIN ONLY CONTAINS A BUCH OF INIT
    //LogInfo("debug step 1");
    //readData.set_priority(osPriorityRealtime);
    connect();
    setTime();

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

        locker.lock();
        LogInfo("temp: %f", data.temperature);
        locker.unlock();
        
        RawData* dataSpace = rawData.try_alloc();
        clone(&data, dataSpace);
        rawDataQueue.try_put(&dataSpace);
        //LogInfo("WRITTEN DATA");
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
        clone(*getData, &data);
        rawData.free(*getData);

        //printf("temp= %f, humid= %f, press= %f, light= %f \n", data.temperature, data.humidity, data.pressure, data.lightLevel);
        //LogInfo("caught data");
        locker.lock();
        LogInfo("2nd temp: %f", data.temperature);
        locker.unlock();
        

        //RawData* iotDataSpace = iotData.try_alloc();
        //clone(&data, iotDataSpace);
        iotDataQueue.try_put(&data);
        

    }

}

void iotWriteF() {

    int fail = 0;
    RawData** getData;
    RawData* data;

    IOTHUB_MESSAGE_HANDLE message_handle;
    IOTHUB_CLIENT_RESULT res;
    char message[100];

    locker.lock();
    IOTHUB_DEVICE_CLIENT_HANDLE client_handle = IoTHubDeviceClient_CreateFromConnectionString(
        azure_cloud::credentials::iothub_connection_string,
        MQTT_Protocol
    );
    azureInit(client_handle);
    locker.unlock();

    while(1){

        if(fail > 10) /* sas */; //#TODO detect problem for failed 10 in a row
        if(!iotDataQueue.try_get_for(20s, &data)) { fail++; continue; }//if 20 s limit back to top (use of the forbidden function)

        fail = 0;
        //clone(*getData, &data);
        //iotData.free(*getData);

        locker.lock();
        LogInfo("3rd temp: %f", data->temperature);
        sprintf(message, "{ ""LightIntensity"" : %5.2f, ""Temperature"" : %5.2f }", data->lightLevel, data->temperature);
        LogInfo("Sending: %s", message);
        message_handle = IoTHubMessage_CreateFromString(message);
        if (message_handle == nullptr) {
            LogError("Failed to create message");
        }
        res = IoTHubDeviceClient_SendEventAsync(client_handle, message_handle, on_message_sent, nullptr);
        IoTHubMessage_Destroy(message_handle); // message already copied into the SDK
        

        //sendMessage(data.lightLevel, data.temperature, client_handle);
        locker.unlock();

    }
}

bool connect()
{
    //LogInfo("Connecting to the network");

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
    //LogInfo("Connection success, MAC: %s", _defaultSystemNetwork->get_mac_address());
    return true;
}

bool setTime()
{
    //LogInfo("Getting time from the NTP server");

    NTPClient ntp(_defaultSystemNetwork);
    ntp.set_server("time.google.com", 123);
    time_t timestamp = ntp.get_timestamp();
    if (timestamp < 0) {
        LogError("Failed to get the current time, error: %ud", timestamp);
        return false;
    }
    //LogInfo("Time: %s", ctime(&timestamp));
    printf("Time: %s", ctime(&timestamp));
    set_time(timestamp);
    return true;
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


void clone(RawData* datain, RawData* dataout){
    dataout->humidity = datain->humidity;
    dataout->temperature = datain->temperature;
    dataout->lightLevel = datain->lightLevel;
    dataout->pressure = datain->pressure;
    dataout->timeStamp = datain->timeStamp;
}