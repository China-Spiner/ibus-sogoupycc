从一段在 mousepad 程序中输入的录像开始介绍输入法 (输入法版本 0.2.3，双拼)：

![http://ibus-sogoupycc.googlecode.com/svn/screenshots/0.2.3/ani_good_network.gif](http://ibus-sogoupycc.googlecode.com/svn/screenshots/0.2.3/ani_good_network.gif)

在这段录像刚开始，刚输入完“突然我觉得我只是一个人”的拼音的时候，
可以直接按下逗号 (不需要按空格)，确认这段输入并加上一个逗号。

### 选词 ###

在刚才，没有按下逗号，而按下了 Tab 键，就进入了选词模式：

![http://ibus-sogoupycc.googlecode.com/svn/screenshots/0.2.3/select_word_1.png](http://ibus-sogoupycc.googlecode.com/svn/screenshots/0.2.3/select_word_1.png)

这里，不同颜色 (颜色可配置) 表示词条来源不同 (可配置禁用一些来源或者调整出现顺序)。

蓝色表示云服务器给的整句 (第一条) 结果，绿色表示云服务器给的词组结果，黑色表示本地词库结果，有时候也能看到灰色，那是内部 gb2312 字库的结果。

按下 j, k, l, ; 等键 (按键可配置) 就可以确认对应词条，使用 h 和 g 键 (按键可配置) 来翻页。

如果使用全拼，可以只输入声母，然后按下 Tab 开始选词：

![http://ibus-sogoupycc.googlecode.com/svn/screenshots/0.2.3/select_word_2.png](http://ibus-sogoupycc.googlecode.com/svn/screenshots/0.2.3/select_word_2.png)

在上面的录像的最后，可以看到使用了输入法扩展“就地转换成繁体”，将选定的一段文字转换到繁体。

### 状态条 ###

输入法扩展在状态条的第四个按钮，点击后会有一个菜单，列出可以使用的扩展，扩展也可以通过定义的快捷键直接使用：

![http://ibus-sogoupycc.googlecode.com/svn/screenshots/0.2.5/ime_panel.png](http://ibus-sogoupycc.googlecode.com/svn/screenshots/0.2.5/ime_panel.png)

状态条的第二个图标显示了当前中英文状态，第三个图标显示当前是否有正在进行的网络请求，按一下它可以看到一些统计数据：

![http://ibus-sogoupycc.googlecode.com/svn/screenshots/0.2.5/stat_notify.png](http://ibus-sogoupycc.googlecode.com/svn/screenshots/0.2.5/stat_notify.png)

下面还有一段录像，这是模拟在网络条件比较不好的情况下会发生的情况 (输入法版本 0.2.3，双拼)：

![http://ibus-sogoupycc.googlecode.com/svn/screenshots/0.2.3/ani_slow_network.gif](http://ibus-sogoupycc.googlecode.com/svn/screenshots/0.2.3/ani_slow_network.gif)

可以直观地看到中文转换没有前面一个录像流畅，但是输入还是很流畅的，这是归功于输入法无阻塞输入的设计。

### 无阻塞输入 ###

观察录像中的这个画面：

![http://ibus-sogoupycc.googlecode.com/svn/screenshots/0.2.3/preedit_1.png](http://ibus-sogoupycc.googlecode.com/svn/screenshots/0.2.3/preedit_1.png)

前面有浅蓝色背景的文字表示这段拼音被发往服务器，但是还没有返回结果。

右边带有下划线的灰色的文字表示，这段中文有本地词库参与，不是或者不全是云服务器的结果。

最右边的蓝色拼音是正在输入的内容。仔细观察会发现逗号是深蓝色的，马上会介绍到这个颜色。

输入法允许在有任意多个没有返回结果的拼音串的时候继续输入，比如这里有两个：

![http://ibus-sogoupycc.googlecode.com/svn/screenshots/0.2.3/preedit_2.png](http://ibus-sogoupycc.googlecode.com/svn/screenshots/0.2.3/preedit_2.png)

倘若右边的内容先得到了结果，会变成深蓝色表示已经确定下来：

![http://ibus-sogoupycc.googlecode.com/svn/screenshots/0.2.3/preedit_3.png](http://ibus-sogoupycc.googlecode.com/svn/screenshots/0.2.3/preedit_3.png)

由于输入法必须由左向右有序地向客户端提交文字，所以深蓝色部分的文字虽然被确定下来，但是没有被真正提交，
必须等“就算挑战”部分最后确定下来才可以。

从上图中下面的正在输入的拼音可以看到，输入法允许在有未确定内容时多行连续输入，只需要简单地用回车键换行就好了。

如果不希望等待云服务器返回结果，可以用 右Alt 键 (可配置) 强制转换前面所有未确定的内容。

### 纠正模式 ###

在上面的录像的最后，选定了一段已经输入的文字，然后立即按下了 Tab，这就进入了“纠正模式”：

![http://ibus-sogoupycc.googlecode.com/svn/screenshots/0.2.3/select_word_3.png](http://ibus-sogoupycc.googlecode.com/svn/screenshots/0.2.3/select_word_3.png)

在这种状态下可以由左到右依次选词，纠正所选内容。

### Notes ###

提倡整句输入以及使用标点符号键而不是空格键确认输入内容，否则，使用本输入法感觉并不会好。

在前面有未确定内容的时候 (状态条第三个图标显示为时钟图案)，不要使用鼠标或者是方向键等移动光标。

有些程序，比如 Firefox 不支持复杂的颜色，在有未确定内容时，重新定位光标到相同的位置也会导致问题。支持得比较好的程序有 mousepad 和 gedit。

全拼用户可以用 [`'`] 键输入隔音符，比如，输入 `keneng` 会得到 `ke neng`，如果要输入 `ken eng`，需要在 `n` 后面按一下 [`'`]。

纠正模式除了选择简体汉字，也可以选择拼音，同样可以纠正，这时候 Delete 键可用于删除一个拼音。对于不能识别的拼音或者汉字英文单词，将没有选词机会，维持原样。 ESC 键则可中止纠正过程。

当选中内容包含多行的时候，不能进入纠正模式。因为，编辑程序时常常选定一段代码，使用 Tab 来控制缩进，这样可以和此不冲突。英文状态下也不能进入纠正模式。

以上只是简单介绍了输入法的主要功能，输入法提供了灵活的配置选项，扩展功能也比较有意思，请参考[此页](http://code.google.com/p/ibus-sogoupycc/wiki/ConfigurationExample)。对于熟悉 lua 语言或者是想了解所有设置项的用户请参考[详细配置说明](http://code.google.com/p/ibus-sogoupycc/wiki/Configuration)。

遇到问题请先查看 [FAQ](http://code.google.com/p/ibus-sogoupycc/wiki/FAQ)。