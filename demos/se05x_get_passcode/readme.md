# Example To Get Pass-code of SE051H secure element

SE051H samples has a trust provisioned binary file (0x7FFF2000) containing the pass-code and salt for NFC commissioning.
The example can be used to read the pass-code value from this binary file.

# Building

The example can be built with

## Matter GN Build system as

```
cd simw-top-mini/repo/demos/se05x_get_passcode/linux
gn gen out
ninja -C out se05x_get_passcode
./se05x_get_passcode
```

## Matter (for RW612) CMake Build system as

```
user@ubuntu:~/Desktop/git/connectedhomeip$ west build -d <out_dir> -b frdmrw612 third_party/simw-top-mini/repo/demos/se05x_get_passcode/mcu
```

## Matter (for FRDMW72) CMake Build system as

```
user@ubuntu:~/Desktop/git/connectedhomeip$ west build -d <out_dir> -b frdmmcxw72 third_party/simw-top-mini/repo/demos/se05x_get_passcode/mcu/ -Dcore_id=cm33_core0 -DCONFIG_CHIP_SE05X=y
```

## Matter (for RT1060) CMake Build system as

```
user@ubuntu:~/Desktop/git/connectedhomeip$ west build -d <out_dir> -b evkcmimxrt1060 third_party/simw-top-mini/repo/demos/se05x_get_passcode/mcu/
```

> [!IMPORTANT]
> Adapt the above commands to the i.MX build commands accordingly.

# Usage

When using on Linux systems, following are the command line options available

```
--help                          ==>    Display this message.
--tp_passcode_set_no <SET_NO>   ==> Get the passcode of the set number specified. (Possible values = 1,2,3).
 Note: If no pass-code set number is passed, default pass-code set number is 1.
```

When using on supported MCUs, you can configure the passcode set number using Kconfig build options:
`TP_SPAKE_PASSCODE_SET_NO` (Possible values = 1,2,3).

Pass the configuration during build:

```
# For FRDMW72
west build -d <out_dir> -b frdmmcxw72 third_party/simw-top-mini/repo/demos/se05x_get_passcode/mcu/ -Dcore_id=cm33_core0 -DCONFIG_SE05X_TP_SPAKE_PASSCODE_SET_NO=2
```
Note: If no pass-code set number is passed, default pass-code set number is 1.