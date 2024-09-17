# EmbeddedProto_Example_FRDM_CAN_Bootloader
Example in which Embedded Proto is used to do CAN net booting on a NXP FRDM-KE06Z


![alt text](https://embeddedproto.com/wp-content/uploads/2022/04/Embedded_Proto.png "Embedded Proto Logo")


Embedded Proto is a product of Embedded AMS B.V. For more information about Embedded Proto please visit [EmbeddedProto.com](https://EmbeddedProto.com).

Copyrights 2020-2024 Embedded AMS B.V. Amsterdam, [www.EmbeddedAMS.nl](https://www.EmbeddedAMS.nl), [info@EmbeddedAMS.nl](mailto:info@EmbeddedAMS.nl)


# Introduction

This repository hosts example code for Embedded Proto, the embedded implementation of Google Protocol Buffers. 
This is an example showing how the NXP FRDM-KE06Z boots with firmware it receives from a CAN network.

Commands are sent from the PC to a USB to CAN converter which is connected to the FDRM-KE06Z.

You can read the full tutorial [here]().

# Requirements

* [FDRM-KE06Z](https://www.nxp.com/design/design-center/development-boards-and-designs/general-purpose-mcus/freedom-development-platform-for-kinetis-ke06-mcus:FRDM-KE06Z)
* MCUExpresso
* USB to CAN adapter, like [this one](https://openlightlabs.com/collections/frontpage/products/canable-pro-isolated-usb-to-can-adapter)
* Dupont cables
* Embedded Proto version 3.4.1 (included as a submodule). The requirements of Embedded Proto are listed [here](https://github.com/Embedded-AMS/EmbeddedProto/blob/master/README.md).

# Installation

1. Install MCUXpresso if you have not already.
2. Install the dependencies required by Embedded Proto. They are listed [here](https://github.com/Embedded-AMS/EmbeddedProto).
3. Clone the repository and auto generate the protobuf files.

```shell
git clone --recursive https://github.com/Embedded-AMS/EmbeddedProto_Example_FRDM_CAN_Bootloader.git 
cd EmbeddedProto_Example_FRDM_CAN_Bootloader
python3 setup.py
```

# Running the code

1. Connect the USB to CAN to the FDRM-KE06Z CAN header. 
2. Upload the code to the FRDM-KE06Z using MCUExpresso. The board includes the PE-Micro debugger.
3. Run the following instructions

```shell
cd EmbeddedProto_Example_FRDM_CAN_Bootloader
source desktop/venv/bin/activate
python3 desktop/main.py desktop/assets/frdmke06z_led_blinky_RED.bin
```

Debug information will be shown in the terminal. After the booting is done the led will start blinking RED.

There is another example binary to make the led blink GREEN
```shell
python3 desktop/main.py desktop/assets/frdmke06z_led_blinky_RED.bin
```

Have fun!

---

Three simple things you can do to help improve Embedded Proto: 
* Give private [feedback](https://EmbeddedProto.com/feedback).
* Report an issue in public on [Github](https://github.com/Embedded-AMS/EmbeddedProto/issues).
* Stay up to date on Embedded Proto via our [User mailing list](https://EmbeddedProto.com/signup).
