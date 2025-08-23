# SE05x Device Attestation Provision Example

The example can be used to provision SE05x secure element with device attestation key pair and certificate.

-   The following credentials are updated using this example :

    | Content                                   | Key id     | Type               |
    | ----------------------------------        | ---------- | ------------------ |
    | Device attestation key pair               | 0x7FFF3007 | NIST-256 Key Pair  |
    | Device attestation certificate            | 0x7FFF3003 | Binary file        |

> [!IMPORTANT]
> The certificate is prefixed with additional 4 bytes of TLV value. The SE05x device attestation class will exclude these 4 bytes on reading.

# Building

The example can be built with

## Matter Build system as

```
cd simw-top-mini/repo/demos/se05x_dev_attest_key_prov/linux
gn gen out
ninja -C out
./se05x_dev_attest_key_prov
```

> [!IMPORTANT]
> Adapt the above commands to the i.MX and RW612 Build commands accordingly.
