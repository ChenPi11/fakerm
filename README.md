# fakerm

Fakerm can "dry run" `sudo rm -rf / --no-preserve-root` command.

## Build

```shell
make
```

Then you can find the binary in `./fakerm`.

## Usage

### Directly run

```shell
./fakerm
```

Then you can see the output like this:

```text
...
rm: cannot remove '/dev/tty46': Device or resource busy
rm: cannot remove '/dev/tty45': Operation not permitted
rm: cannot remove '/dev/tty44': Operation not permitted
...
```

If you Press `Ctrl+C`, it will fall back to a fake shell.
input `wtf` to exit the fake shell.

### Dpkg injection

Fakerm can inject itself into a deb package. When the package is installed, it will run automatically.

```shell
apt download xz-utils
./inject-dpkg xz-utils_5.6.2-2_amd64.deb injected-xz-utils_5.6.2-2_amd64.deb
```

Then when you install the package.

```shell
sudo apt install ./injected-xz-utils_5.6.2-2_amd64.deb
```

It will run automatically. It will stop after listing all files in `/dev`, `/sys`, `/proc`, etc.

## License

Unlicense
