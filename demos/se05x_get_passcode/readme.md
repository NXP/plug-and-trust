# Example To Get Pass-code of SE051H secure element

SE051H samples has a trust provisioned binary file (0x7FFF2000) containing the pass-code and salt for NFC commissioning.
The example can be used to read the pass-code value from this binary file.

# Building

The example can be built with

## Matter Build system as

```
cd simw-top-mini/repo/demos/se05x_get_passcode/linux
gn gen out
ninja -C out se05x_get_passcode
./se05x_get_passcode
```

> [!IMPORTANT]
> Adapt the above commands to the i.MX build commands accordingly.
