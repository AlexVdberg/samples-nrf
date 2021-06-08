# Introduction
This is a repository of my sample code using the nrf connect sdk. Most of these
will be samples copied from other places and modified slightly while I figure
out what I'm doing.

# Building
To build any of the samples:
```
export ZEPHYR_TOOLCHAIN_VARIANT=gnuarmemb
export GNUARMEMB_TOOLCHAIN_PATH="/opt/gnuarmemb"
source ~/ncs/zephyr/zephyr-env.sh
west build -b adafruit_feather_nrf52840 <path to project>
west flash
```
