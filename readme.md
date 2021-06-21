# Introduction
This is a repository of my sample code using the nrf connect sdk. Most of these
will be samples copied from other places and modified slightly while I figure
out what I'm doing.

# Setup
Ensure your environment is setup by creating a `~/.zephyrrc` file with the
following variables directed to your toolchain installation:
```
export ZEPHYR_TOOLCHAIN_VARIANT=gnuarmemb
export GNUARMEMB_TOOLCHAIN_PATH="/opt/gnuarmemb"
```

# Building
To build any of the samples:
```
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
I use `vim` as my primary editor. The `coc.nvim` plugin and `coc-clangd`
extension work nicely with the nrf connect sdk with a couple of minor tweaks.

Clangd does have a `--query-driver` option that would allow it to extract the
include paths from the gnuarmemb compiler. I haven't noticed a need to do this
yet and the coc-clangd plugin doesn't have an obvious way to do this.

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
Add the follwing to `.vim/coc-settings.json` (your `:CocConfig` or
`:CocLocalConfig`)

```
{
	"clangd.compilationDatabasePath": "build/"
}
```

## Tell coc-clangd to query for system include files from compilers
Add the follwing to `.vim/coc-settings.json` (your `:CocConfig` or
`:CocLocalConfig`). Change the path to your arn-none-eabi-gcc executable.
There's probably a better way to do this one at a system level with some
conditions around environment variables, but this works for now. This is also
related to [issue #539 of clangd](https://github.com/clangd/clangd/issues/539).

```
{
	"clangd.arguments": ["--query-driver", "/opt/gnuarmemb/bin/arm-none-eabi-gcc"]
}
```
