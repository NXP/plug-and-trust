# NFC Commissioning Provision example

The example can be used to provision the credentials required for NFC / unpowered commissioning of matter devices.
The example is be used with SE051H samples updated with NFC commissioning applet.

-   The following credentials are updated using this example :

    | Content                                   | Key id         | Type               |
    | ----------------------------------        | ----------     | ------------------ |
    | PBKDF Parameters                          | 0x7FFF3002     | Binary File        |
    | Device attestation certificate            | 0x7FFF3003     | Binary file        |
    | Product attestation Authority certificate | 0x7FFF3004     | NIST-256 Key Pair  |
    | Device attestation key pair               | 0x7FFF3007     | Binary file        |
    | Device attestation TBS                    | 0x7FFF3005     | Binary file        |
    | Select response                           | 0x7FFF3001     | Binary file        |
    | Node Operational key pair                 | 0x7FFF3101     | NIST-256 Key Pair  |
    | Node Operational certificate              | 0x7FFF3201     | Binary file        |
    | Root certificate                          | 0x7FFF3301     | Binary file        |
    | Wi-Fi Credentials                         | 0x7FFF3401     | Binary file        |
    | Access Control List                       | 0x7FFF3501     | Binary file        |
    | Identity Protection Epoch Key             | 0x7FFF3601     | Binary file        |
    |                                           |                |                    |
    | Basic Information Cluster                 | 0x7FFE0028     | Binary file        |
    | General commissioning Cluster             | 0x7FFE0030     | Binary file        |
    | Operational Credential Cluster            | 0x7FFE003E     | Binary file        |
    | Access Control Cluster                    | 0x7FFE001F     | Binary file        |
    | Network Commissioning Cluster             | 0x7FFE0031     | Binary file        |
    |                                           |                |                    |
    | Vendor reserved binary file               | 0x7FFF3006     | Binary file        |
    | Descriptive cluster (End point 0)         | 0x7FFE001D     | Binary file        |
    | Descriptive cluster (End point 1)         | 0x00017FFE001D | Binary file        |


NOTE: The example can also be used to provision the QR code into T4T applet.

# Building

The example can be built with

## cmake build system as -

```
cd simw-top-mini/repo/demos/se051h_nfc_comm_prov/cmake_build
mkdir build
cd build
cmake ..
cmake --build .
./se051h_nfc_comm_prov
```

## Matter GN Build system as

```
cd simw-top-mini/repo/demos/se051h_nfc_comm_prov/linux
gn gen out
ninja -C out se051h_nfc_comm_prov
./se051h_nfc_comm_prov
```
## Matter (for RW612) CMake Build system as

```
user@ubuntu:~/Desktop/git/connectedhomeip$ west build -d <out_dir> -b frdmrw612 third_party/simw-top-mini/repo/demos/se051h_nfc_comm_prov/mcu -DCONFIG_CHIP_SE05X=y
```

## Matter (for FRDMW72) CMake Build system as

```
user@ubuntu:~/Desktop/git/connectedhomeip$ west build -d <out_dir> -b frdmmcxw72 third_party/simw-top-mini/repo/demos/se051h_nfc_comm_prov/mcu/ -Dcore_id=cm33_core0 -DCONFIG_CHIP_SE05X=y
```

## Matter (for RT1060) CMake Build system as

```
user@ubuntu:~/Desktop/git/connectedhomeip$ west build -d <out_dir> -b evkcmimxrt1060 third_party/simw-top-mini/repo/demos/se051h_nfc_comm_prov/mcu/ -DCONFIG_CHIP_SE05X=y
```

> [!IMPORTANT]
> Adapt the above commands to the i.MX, RW612 and RT1060 build commands accordingly.

## Applet session

To run the provision example with applet session (say AES key), use the below command for build

```
user@ubuntu:~/Desktop/git/connectedhomeip$ west build -d <out_dir> -b frdmrw612 third_party/simw-top-mini/repo/demos/se051h_nfc_comm_prov/mcu -DCONFIG_CHIP_SE05X=y -DCONFIG_CHIP_SE05X_AES_KEY=y
```
```
user@ubuntu:~/Desktop/git/connectedhomeip$ west build -d <out_dir> -b frdmmcxw72 third_party/simw-top-mini/repo/demos/se051h_nfc_comm_prov/mcu/ -Dcore_id=cm33_core0 -DCONFIG_CHIP_SE05X_AES_KEY=y
```
```
user@ubuntu:~/Desktop/git/connectedhomeip$ west build -d <out_dir> -b evkcmimxrt1060 third_party/simw-top-mini/repo/demos/se051h_nfc_comm_prov/mcu/ -DCONFIG_CHIP_SE05X_AES_KEY=y
```

# Usage

When using on Linux systems, following are the command line options available

```
--help                      ==> Display this message.
--doreset                   ==> Delete all provisioned data (QR code is not deleted).
--only_t4t_provision        ==> Provision only QR code in T4T applet.
--qrcode <QR_CODE_VALUE>    ==> QR code to provisioned in T4T applet.
--rawdata <raw_text_file>   ==> Raw data (hex bytes) to be provisioned in T4T applet. Ensure to pass a valid ndef header also.
--tp_spake_passcode_set_no  ==> Pass-code set to be used for SPAKE2P connection. (Possible values = 1,2,3).
--tp_spake_itter_to_be_used ==> Iterations to be used for SPAKE2P connection. (Possible values 1000,5000,10000, 50000, 100000)
--wifi_net_interface        ==> Enable only Wi-Fi network interface for NFC commissioning.
--thread_net_interface      ==> Enable only Thread network interface for NFC commissioning.
--ethernet_net_interface    ==> Enable only Ethernet network interface for NFC commissioning.
Note: It is mandatory to pass at-least one network interface.
--ec_key_session_key        ==> Provision the key for EC key Applet session
--user_id_session_key       ==> Provision the key for User ID Applet session
--aes_key_session_key       ==> Provision the key for AES key Applet session
```

# Running provision example on MCUs

When using on supported MCUs, you can configure the provisioning options using Kconfig build options.

## Configuration Options

The following Kconfig options are available:

| Option | Type | Default | Description |
|--------|------|---------|-------------|
| `-DCONFIG_SE05X_DO_RESET` | bool | n | Delete all provisioned data (QR code is not deleted) |
| `-DCONFIG_SE05X_DO_EC_KEY_PROVISION` | bool | n | Provision EC key for applet session |
| `-DCONFIG_SE05X_DO_USER_ID_PROVISION` | bool | n | Provision User ID for applet session |
| `-DCONFIG_SE05X_DO_AES_KEY_PROVISION` | bool | n | Provision AES key for applet session |
| `-DCONFIG_SE05X_ONLY_T4T_PROVISION` | bool | n | Provision only QR code in T4T applet |
| `-DCONFIG_SE05X_TP_SPAKE_PASSCODE_SET_NO` | int | 1 | Trust Provisioned pass-code set number (1-3) |
| `-DCONFIG_SE05X_TP_SPAKE_ITER_TO_BE_USED` | int | 1000 | Trust Provisioned iteration count (1000, 5000, 10000, 50000, 100000) |
| `-DCONFIG_SE05X_DEVICE_NETWORK_TYPE_WIFI` | bool | y | Enable WiFi network interface for NFC commissioning |
| `-DCONFIG_SE05X_DEVICE_NETWORK_TYPE_THREAD` | bool | n | Enable Thread network interface for NFC commissioning |
| `-DCONFIG_SE05X_DEVICE_NETWORK_TYPE_ETHERNET` | bool | n | Enable Ethernet network interface for NFC commissioning |
| `-DCONFIG_SE05X_PROVISION_WITH_POLICY` | bool | n | Enable provisioning with policy |


The example has the default DAC keys and certificates from MATTER SDK. To use custom DAC keys and certificates, modify the header file contents (file - connectedhomeip/third_party/simw-top-mini/repo/demos/se051h_nfc_comm_prov/common/se051h_nfc_comm_prov.h),

```c
#define DAC_CERTIFICATE                                                        \
  0x31, 0x00, 0xED, 0x01, 0x30, 0x82, 0x01, 0xE9, 0x30, 0x82, 0x01, 0x8E,      \
...
...
```

```c
#define DA_KEY_PAIR_DATA                                                       \
  0x30, 0x77, 0x02, 0x01, 0x01, 0x04, 0x20, 0xCC, 0xCF, 0x9D, 0xC7, 0x05,      \
      0x0E, 0xF5, 0xD9, 0x0B, 0xE4, 0x57, 0x07, 0x
...
...
```
