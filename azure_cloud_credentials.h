/*
 * Google Cloud Certificates
 * Copyright (c) 2019-2020, Arm Limited and affiliates.
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef AZURE_CLOUD_CREDENTIALS_H
#define AZURE_CLOUD_CREDENTIALS_H

namespace azure_cloud {
    namespace credentials {
        /*
        * Primary Connecion String
        */

        // Use https://dpsgen.z8.web.core.windows.net/ to obtain YOUR connection string
        // This one will not work, but I guess you knew that ;)
        const char iothub_connection_string[] = "HostName=iotc-d730022e-8bb9-4f8c-b716-3faa76b1f69a.azure-devices.net;DeviceId=1df0s07au85;SharedAccessKey=QfcGUK7bZa/odjkXrdTqgqGgA6taFa5AdbPausGgwAE=";

    }
}
#endif
