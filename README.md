# fakerm

Fakerm can "dry run" `sudo rm -rf / --no-preserve-root` command.

## Build

```shell
make
```

Then you can find the binary in `./fakerm`.

## Usage

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

## License

Unlicense
