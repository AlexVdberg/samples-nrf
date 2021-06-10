# Introduction
This is a repository of my sample code using the nrf connect sdk. Most of these
will be samples copied from other places and modified slightly while I figure
out what I'm doing.

# Building
To build any of the samples:
```
export ZEPHYR_TOOLCHAIN_VARIANT=gnuarmemb
export GNUARMEMB_TOOLCHAIN_PATH="/opt/gnuarmemb"
source ~/dev/ncs/zephyr/zephyr-env.sh
west build -b adafruit_feather_nrf52840 <path to project>
west flash
```

# Viewing usb output
You can view the console output (from `printk`) with `minicom`

```
minicom -D /dev/ttyACM0
```

You will need permisions to connect to the tty port. This can be done by adding
yourself to the user group for the `/dev/ttyACM0` port. This can be done with
the following command (Assuming the port has the uucp user group as shown with
`ls -l /dev/ttyACM*`:

```
sudo usermod -a -G uucp $USER
```

# coc.nvim setup for nrf connect sdk

## Remove unrecognised compiler flag
To get code completion for nrf connect projects, I had to configure clangd
slightly. Namely I got errors relating to clangd not understanding
`-fno-reorder-functions`. The solution is to tell clangd to remove these options
by adding to your `~/.config/clangd/config.yaml` or a file `.clangd` in the
project, the following:

```
CompileFlags:
    Remove: -fno-reorder-functions
```

## Add compilation database to build
The CMake parameter is `CMAKE_EXPORT_COMPILE_COMMANDS`, and here's examples of
building an app:

```
west build -b <board> <app directory> -- -DCMAKE_EXPORT_COMPILE_COMMANDS=1
```

If you always want to generate the compilation database, you can instead do:

```
west config build.cmake-args -- -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
```

## Tell coc-clangd where the compilation database is
Add the follwing to .vim/coc-settings.json (your `:CocConfig` or `:CocLocalConfig`)

{
	"clangd.compilationDatabasePath": "build/"
}
