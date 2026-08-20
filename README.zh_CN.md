# fakerm

Fakerm 可以“预览运行” `sudo rm -rf / --no-preserve-root` 命令。

## 构建

```shell
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release
```

然后你就可以找到可执行文件 `./fakerm` 。

## 用法

### 直接运行

```shell
./fakerm
```

然后你可以看到输出：

```text
...
rm: cannot remove '/dev/vcs': 设备或资源忙
rm: cannot remove '/dev/tty0': 设备或资源忙
rm: cannot remove '/dev/console': 设备或资源忙
rm: cannot remove '/dev/tty': 设备或资源忙
rm: cannot remove '/dev/kmsg': 设备或资源忙
rm: cannot remove '/dev/urandom': 设备或资源忙
rm: cannot remove '/dev/random': 不允许的操作
rm: cannot remove '/dev/full': 不允许的操作
rm: cannot remove '/dev/zero': 设备或资源忙
rm: cannot remove '/dev/port': 设备或资源忙
rm: cannot remove '/dev/null': 设备或资源忙
rm: cannot remove '/dev/mem': 不允许的操作
rm: cannot remove '/dev/vga_arbiter': 设备或资源忙
...
```

如果你按下 `Ctrl+C`，它将回退到一个假 shell。
输入 `wtf` 退出假 shell。

### Dpkg 注入

Fakerm 可以将自己注入到一个 deb 包中。当该包被安装时，它会自动运行。

```shell
apt download xz-utils
./inject-dpkg xz-utils_5.6.2-2_amd64.deb injected-xz-utils_5.6.2-2_amd64.deb
```

然后当你安装该包时：

```shell
sudo apt install ./injected-xz-utils_5.6.2-2_amd64.deb
```

它会自动运行。它会在列出 `/dev`、`/sys`、`/proc` 等所有文件后停止。

## 许可证

The Unlicense
