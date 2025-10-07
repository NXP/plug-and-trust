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

## Matter CMake Build system as

```
user@ubuntu:~/Desktop/git/connectedhomeip$ west build -d <out_dir> -b frdmrw612 third_party/simw-top-mini/repo/demos/se05x_get_passcode/rw61x
```

> [!IMPORTANT]
> Adapt the above commands to the i.MX build commands accordingly.
