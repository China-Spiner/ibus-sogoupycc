### 在 Windows 下能使用这个输入法吗？ ###

Sorry，不支持除 Linux 以外的作业系统。

### 这个项目和搜狗是何种关系？ ###

这个项目是非搜狗官方的个人行为，项目中通过某种形式使用了搜狗云输入法服务。

使用本输入法将导致中文状态下输入的拼音被上传到搜狗服务器，由此导致的隐私问题本项目不负责任。

当搜狗方面通过某种方式让第三方应用使用云输入法变得困难时，本项目可能由此中止。

如果你可以代表搜狗云输入法官方，欢迎通过本页最下方的联系方式和我联系。

### 为什么没法切换到输入法 ###

换一个窗口试试看？不成的话，重启 ibus 再试。

如果一直不行，可能是由于依赖或者兼容问题输入法没法运行，这时候从终端运行：
```
/usr/share/ibus-sogoupycc/engine/ibus-sogoupycc -d
```
可能会看到有助于问题解决的信息。

### 使用输入法很快就卡住，然后输入法程序崩溃 ###

请检查输入法构建时所用的 ibus 版本是否和系统的 ibus 版本兼容：
```
% /usr/share/ibus-sogoupycc/engine/ibus-sogoupycc -v
ibus-sogoupycc 0.2.6 [built for ibus 1.3.1]
```

如果版本号的前两个数字和正在使用的 ibus 版本号有差异，需要重新编译输入法。

### 不能选词？！ ###

由于历史原因，一些网站上可能记载着本输入法不能选词。但是实际上，自从 2010 年 2 月 27 日，输入法的第一个 release 版本开始，就可以选词了。

至于如何选，请参考 [Tutorial](http://code.google.com/p/ibus-sogoupycc/wiki/Tutorial) 页。

### 速度比较慢 ###

输入法反应速度主要和连接到搜狗云服务器的网络速度有关系，比如教育网就可能会比较慢。

即便网速慢，由于“无阻塞输入”设计，输入过程仍然可以非常流畅。

### 想要在 Firefox 这样的程序中看到彩色的 preedit ###

可以把 ibus 选项中的 "Embed preedit text in application window" 去掉：

![http://ibus-sogoupycc.googlecode.com/svn/screenshots/0.2.5/ibus_preference_embed_preedit.png](http://ibus-sogoupycc.googlecode.com/svn/screenshots/0.2.5/ibus_preference_embed_preedit.png)

### 输入拼音的时候能不能保留右边几个拼音，不要全部转换成汉字？ ###

可以。参考配置样例中的“杂项”，`ime.preedit_reserved_pinyin` 选项。

### 能不能在按回车的时候，把输入中的拼音不做转换，当做英文字符直接上屏？ ###

可以。参考配置样例中的“热键”部分，`ime.raw_preedit_key` 选项。

### 不联网会怎么样？ ###

输入法会使用很简单的算法利用本地词库给出结果，效果会差许多，但是还可以用。

为了直观说明，下面是联网和不联网结果的比较：
```
1.我坐在窗前，望着窗外，回忆满天。生命是华丽错觉，时间是贼，偷走一切。
2.我坐在窗前，望着窗外，会议漫天。声明是华丽错觉，时间是贼，偷走一切。

1.七岁的那一年，抓住那只蝉，以为能抓住夏天，十七岁的那年，吻过她的脸，就以为和他能永远。
2.七岁的那一年，抓住那置产，一位能抓住夏天，十七岁的那年，问过他的脸，就医为何他能永远。

1.有没有那么一种永远，永远不改变，拥抱过的美丽都再也不破碎，让险峻岁月不能在脸上撒野，让生离和死别都遥远，有谁能听见？
2.有没有那么一种永远，永远不改变，用包裹的美丽都再也不破碎，让险峻随越不能在脸上撒野，让胜利和四别逗遥远，有谁能听见？
```

### 扩展按钮消失了 ###

这是 ibus-1.2.0.20090927 已知问题，升级到 ibus-1.2.0.20100111 以及之后的版本就可以解决。

注意，虽然看不到扩展按钮，但是仍可以使用扩展快捷键 (如果有的话) 访问扩展功能。

### 为啥一个扩展也没有 ###

默认会从网络加载一段脚本，添加扩展。由于网络原因，可能会导致失败或延迟，这时候就没有扩展了。

### 如何重启 ibus ###

右键单击任务栏 ibus 图标，选择 Restart 来重新启动它。也可以用命令行 `ibus-daemon -rxd` 重启 ibus。

### 修改过配置没有生效 ###

需要重新启动 ibus 就可以生效了。

### 输入 `ccc` 或者是其他的一些简拼字母之后，怎么突然变到英文状态了？ ###

正常。参考设置说明中的 `ime.auto_eng_tolerance` 选项。

这个选项是为部分喜欢直接在中文状态下输入“www.”网址的用户设而计。

### 词库没有被加载 ###

确保系统中存在词库文件 (默认位置是 /usr/share/ibus-sogoupycc/db/)

加载词库会在程序开始时在后台线程执行，如果你刚刚第一次切换到输入法，依据计算机性能，等待几秒后，词库即可加载完成。

如果等待几秒后还是不行，那么请重新启动 ibus 试试看？

### 我想自造一些词，比如人名 ###

请参考配置说明中的“缓存”部分

### 输入 `xi an`，却出现了一个 `先` 字 ###

这是由于云服务器协议限制导致的，看起来它自从 2010 年 3 月 9 日 开始不再支持拼音分隔符。如果是自己选词的话，不会出现这种问题。

_如果你知道该如何向云服务器提交拼音分隔符信息，请告诉我，谢谢_

### 候选词列表不显示 ###

如果查询一长串汉字的话，又有多音字的情况，数据库操作需要时间，在配置很低的机器上可能需要几秒，请稍等。

也有极个别情况，候选词列表不见了，这时候你可以尝试用选词键（比如，`j`）直接选词，如果不行，可以按 `ESC` 取消，如果是普遍现象，请提交 Bug。

### 我想得到 `5。4`，实际却得到 `5.4` ###

正常现象。参考设置说明中的 `ime.punc_after_chinese` 选项

### 输入法状态条频繁闪烁 ###

可能是 ibus 或者当前程序的问题，这时候切换到别的窗口再切换回来，或者是按两次启用/禁用输入法热键 (默认是Ctrl + Space) 就可以了。

### 按下 Tab 键，却没有进入纠正模式 ###

纠正模式要在中文状态下使用，并且不能用于多行文本，而且默认在选词后三秒钟内按下才有效。

### 按下 Tab 键，进入了纠正模式，但是我没有选定任何文本啊 ###

由于目前 ibus 平台没有提供获取选中文字的接口，程序中使用 GTK+ 剪贴板方法获得选中文字，如果切换到其他窗口并选定一段文字再切换回来使用 Tab 键，可能纠正的是其它窗口选中的内容。为保证一定的可靠性，默认在选择文本后几秒内按下 Tab 键才有效，超过时间限制就需要重新选择。

### 如果知道云拼音服务是正常的？ ###

从命令行运行：
```
$XDG_CACHE_HOME/ibus/sogoupycc/fetcher 'wo men'
```
看看会不会有结果。

### 什么时候能够支持模糊音 ###

全民语文素质在上，本输入法大概永远不会支持模糊音。

### 我还有其他问题 ###

可以在 [Issues](http://code.google.com/p/ibus-sogoupycc/issues/list) 页提出来，或者[和作者联系](http://lihdd.net/?page_id=7)。