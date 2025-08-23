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

## Matter Build system as

```
cd simw-top-mini/repo/demos/se051h_nfc_comm_prov/linux
gn gen out
ninja -C out se051h_nfc_comm_prov
./se051h_nfc_comm_prov
```

> [!IMPORTANT]
> Adapt the above commands to the i.MX and RW612 Build commands accordingly.
