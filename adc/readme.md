# Analog-to-Digital Converter (ADC)

## Overview

This sample demonstrates how to use the ADC driver API on the adafruit feather
nrf52840 express.

It reads the ADC samples from all 8 ADC channels (including the AREF and VDIV
channels) and prints the readings to the console. The raw readings are also
converted to millivolts.

The pins of the ADC channels are board-specific. Please refer to the board
or MCU datasheet for further details.

The pins of the ADC channels are defined in
`boards/adafruit_feather_nrf52840.overlay`. The channel number corrosponts to
the `AIN#`, not the `A#`; ie. `channel 0` corrosponds to `A4`.


## Building and Running

The ADC peripheral and pinmux is configured in the board's `.dts` file. Make
sure that the ADC is enabled (`status = "okay";`).

In addition to that, this sample requires an ADC channel specified in the
`io-channels` property of the `zephyr,user` node. This is usually done with
a devicetree overlay. The example overlay in the `boards` subdirectory for
the `adafruit_feather_nrf52840` can be easily adjusted for other boards.

### Building and Running for Adafruit nrf52840 express

The sample can be built and executed for the
`adafruit_feather_nrf52840` as follows:

```
west build -b adafruit_feather_nrf52840 adc
west flash
```

To build for another board, change `adafruit_feather_nrf52840` above to that
board's name and provide a corresponding devicetree overlay.

### Sample output

You should get a similar output as below, repeated every second:

```
Channel 0: 1104 = 970 mV  Channel 1: 1108 = 973 mV  Channel 2: 339 = 297 mV  Channel 3: 207 = 181 mV  Channel 4: 241 = 211 mV  Channel 5: 2410 = 2118 mV  Channel 6: 729 = 640 mV  Channel 7: 371 = 326 mV
ADC_GAIN: 0, ADC_RESOLUTION 12, ADC_NUM_CHANNELS 8, ADC_REFERENCE 4
```
