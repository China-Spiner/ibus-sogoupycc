## Archlinux ##

ibus-sogoupycc 已经处于 Archlinux 软件仓库中，可以直接使用 pacman 安装：
```
sudo pacman -S ibus-sogoupycc
```

[AUR](http://aur.archlinux.org/) 上的 svn 版本是我本人在维护，可能会新一点。如果要从 AUR 安装 svn 版本，建议使用 [yaourt](http://wiki.archlinux.org/index.php/Yaourt) 工具：
```
sudo yaourt -S ibus-sogoupycc-svn
```

以上两个软件包都已经包含 open-phrase 0.1.10.6 (2008 年 9月版本) 词库。

## Debian / Ubuntu ##

[下载页面](http://code.google.com/p/ibus-sogoupycc/downloads)提供了 deb 版本的程序 (32 位系统, ibus 1.2.0 - 1.2.1) 的安装包，以及词库文件安装包，下载后即可直接安装。

注意：如果使用的 ibus 版本 >= 1.2.99 (包括 1.2.99)，**不要**使用下载页面提供的程序软件包。

## 从源代码安装 ##

从 [下载页面](http://code.google.com/p/ibus-sogoupycc/downloads) 下载源代码，解压缩，进入 build.sh 所在目录，执行下列命令：

```
./build.sh
cd build
sudo make install
```

构建需要用 `cmake` ， 然后由 `cmake` 负责检查其它的依赖。

比如，ubuntu 系统上，大概需要这些依赖才能正常编译和运行：

> `liblua5.1-0-dev, liblua5.1-socket2, libsqlite3-dev, libgtk2.0-dev, libibus-dev, libnotify-dev, lua5.1`

Archlinux 系统上，则是这些软件包，其中的 ibus 可能需要从 AUR 安装：

> `lua (5.1), luasocket, glib, dbus, sqlite3, gtk2, ibus (1.2.0.20100111), libnotify`

运行程序需要保证 lua 解释器以及 luasocket 存在，这个依赖是在编译阶段不检查的。推荐使用 lua 5.1.4 版本，终端下执行命令
```
lua -e 'require "socket.http"'
```
应该是没有输出，无错误结束。

自行编译的版本没有包含词库，将只能使用 gb2312 字库，输入法的一些功能会因此受到限制。

词库可以使用[带有索引的 open-phrase 0.1.10.6](http://ibus-sogoupycc.googlecode.com/files/open-phrase-201003.tar.gz)，默认情况下，下载解压后放在 `/usr/share/ibus-sogoupycc/db/` 即可。

## 添加输入法 ##

正确安装之后，在 ibus 选项中就可以添加本输入法，切换到即可使用。

![http://ibus-sogoupycc.googlecode.com/svn/screenshots/0.2.5/ibus_preference.png](http://ibus-sogoupycc.googlecode.com/svn/screenshots/0.2.5/ibus_preference.png)

可能需要先重新启动 ibus 才能在 ibus 选项中看到本输入法。重新启动 ibus 的方法是右键单击任务栏 ibus 图标，选择 Restart，或者是使用命令行:
```
ibus-daemon -rxd
```

接着，您可能对输入法的一些功能感兴趣，请参考[此页](http://code.google.com/p/ibus-sogoupycc/wiki/Tutorial)