# NFC Commissioning Provision example

The example can be used to provision the credentials required for NFC / unpowered commissioning of matter devices.
The example is be used with SE051H samples updated with NFC commissioning applet.

-   The following credentials are updated using this example :

    | Content                                   | Key id     | Type               |
    | ----------------------------------        | ---------- | ------------------ |
    | PBKDF Parameters                          | 0x7FFF3002 | Binary File        |
    | Device attestation certificate            | 0x7FFF3003 | Binary file        |
    | Product attestation Authority certificate | 0x7FFF3004 | NIST-256 Key Pair  |
    | Device attestation key pair               | 0x7FFF3007 | Binary file        |
    | Device attestation TBS                    | 0x7FFF3005 | Binary file        |
    | Select response                           | 0x7FFF3001 | Binary file        |
    | Node Operational key pair                 | 0x7FFF3101 | NIST-256 Key Pair  |
    | Node Operational certificate              | 0x7FFF3201 | Binary file        |
    | Root certificate                          | 0x7FFF3301 | Binary file        |
    | Wi-Fi Credentials                         | 0x7FFF3401 | Binary file        |
    | Access Control List                       | 0x7FFF3501 | Binary file        |
    | Identity Protection Epoch Key             | 0x7FFF3601 | Binary file        |
    |                                           |            |                    |
    | Basic Information Cluster                 | 0x7FFE0028 | Binary file        |
    | General commissioning Cluster             | 0x7FFE0030 | Binary file        |
    | Operational Credential Cluster            | 0x7FFE003E | Binary file        |
    | Access Control Cluster                    | 0x7FFE001F | Binary file        |
    | Network Commissioning CLuster             | 0x7FFE0031 | Binary file        |


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
user@ubuntu:~/Desktop/git/connectedhomeip$ west build -d <out_dir> -b frdmrw612 third_party/simw-top-mini/repo/demos/se051h_nfc_comm_prov/mcu
```

## Matter (for FRDMW72) CMake Build system as

```
user@ubuntu:~/Desktop/git/connectedhomeip$ west build -d <out_dir> -b frdmmcxw72 third_party/simw-top-mini/repo/demos/se051h_nfc_comm_prov/mcu/ -Dcore_id=cm33_core0 -DCONFIG_CHIP_SE05X=y
```

## Matter (for RT1060) CMake Build system as

```
user@ubuntu:~/Desktop/git/connectedhomeip$ west build -d <out_dir> -b evkcmimxrt1060 third_party/simw-top-mini/repo/demos/se051h_nfc_comm_prov/mcu/
```

> [!IMPORTANT]
> Adapt the above commands to the i.MX, RW612 and RT1060 build commands accordingly.

# Usage

When using on Linux systems, following are the command line options available

```
--help                      ==> Display this message.
--doreset                   ==> Delete all provisioned data (QR code is not deleted).
--only_t4t_provision        ==> Provision only QR code in T4T applet.
--qrcode <QR_CODE_VALUE>    ==> QR code to provisioned in T4T applet.
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

When using on supported MCUs, modify the main file defines (examples - simw-top-mini/repo/demos/se051h_nfc_comm_prov/mcu/main.cpp)
