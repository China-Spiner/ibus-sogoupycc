这里列出一些设置样例。这些设置应该存放在 `$XDG_CONFIG_HOME/ibus/sogoupycc/config.lua` 文件中。

`$XDG_CONFIG_HOME` 默认是 `$HOME/.config`，如果你的系统中 `$XDG_CONFIG_HOME` 是空白的话，请改用 `$HOME/.config`。

默认情况下，需要重新启动输入法让设置修改生效。

### 双拼 ###

启用双拼：
```
ime.use_double_pinyin = true
```

启用双拼，并和全拼混合输入（优先考虑双拼合法性）：
```
ime.use_double_pinyin = true
ime.strict_double_pinyin = false
```

使用智能 ABC 的双拼方案（其他双拼方案见 [配置](http://code.google.com/p/ibus-sogoupycc/wiki/Configuration) 页）：
```
ime.set_double_pinyin_scheme{
	q = {"q", {"ei"}}, w = {"w", {"ian"}}, e = {"-", {"e"}}, r = {"r", {"iu", "er"}}, t = {"t", {"iang", "uang"}}, y = {"y", {"ing"}}, u = {"-", {"u"}}, i = {"-", {"i"}}, o = {"", {"o", "uo"}}, p = {"p", {"uan"}},
	a = {"zh", {"a"}}, s = {"s", {"ong", "iong"}}, d = {"d", {"ia", "ua"}}, f = {"f", {"en"}}, g = {"g", {"eng"}}, h = {"h", {"ang"}}, j = {"j", {"an"}}, k = {"k", {"ao"}}, l = {"l", {"ai"}},
	z = {"z", {"iao"}}, x = {"x", {"ie"}}, c = {"c", {"in", "uai"}}, v = {"sh", {"ve", "v"}}, b = {"b", {"ou"}}, n = {"n", {"un"}}, m = {"m", {"ue", "ve", "ui"}},
}
```

### 中文标点 ###

不使用中文标点：
```
ime.punc_map = { }
```

只要逗号、句号、双引号中文标点，其他的不要：
```
-- 对于多状态的标点，比如引号有两种状态，用数组表示：
ime.punc_map = { [','] = '，', ['.'] = '。', ['"'] = {'“', '”'} }
```

`[GEEK]` 把 右 Shift 键作为标点，中文状态下每按一次，输出《春晓》的一句：
```
ime.punc_map = { [key.Shift_R] = {'春眠不觉晓\n', '处处闻啼鸟\n', '夜来风雨声\n', '花落知多少\n'} }
```

### 热键 ###

#### 直接提交拼音 ####

使用回车键直接把正在编辑的拼音串当做英文输入：
```
ime.raw_preedit_key = key.Return
```

#### 中英文状态切换 ####

让左右 Shift 都能用来切换中英文状态：
```
ime.eng_mode_key = {key.Shift_R, key.Shift_L}
ime.chs_mode_key = ime.eng_mode_key
```

用左 Shift 进入中文模式，右 Shift 进入英文模式：
```
ime.eng_mode_key = key.Shift_R
ime.chs_mode_key = key.Shift_L
```

用左 Shift 切换中英文模式，右 Shift 进入英文模式：
```
ime.eng_mode_key = {key.Shift_L, key.Shift_R}
ime.chs_mode_key = key.Shift_L
```

#### 翻页 ####

用 `PageDown` 在选词列表中向下翻页：
```
ime.page_down_key = key.Page_Down
```

让 h，`PageDown`，逗号，`]`，`=`键 都能用来向下翻页：
```
ime.page_down_key = {'h', key.Page_Down, ',', ']', '='}
-- 类似地，可以设置向上翻页按键：
ime.page_up_key = {'g', key.Page_Up, '.', '[', '-'}
```

#### 选词 ####

用 123456789 来选词：
```
ime.label_keys = '123456789'
```

用 `jkl;` 选词：
```
ime.label_keys = 'jkl;'
-- 或者
ime.label_keys = {'j', 'k', 'l', ';'}
```

用 `jkl;` 选词，让分号实际显示为 '分号'：
```
ime.label_keys = {'j', 'k', 'l', {';', label = '分号'}}
```

用 `jkl;` 和 `1234` 选词，其中 `j` 和 `1` 同是选第一个词，依次类推。第一个词，除了用 `j` 和 `1`，也可以用空格选，显示为'1234'：
```
ime.label_keys = { {'1', 'j', key.space}, {'2', 'k'}, {'3', 'l'}, {'4', ';'} }
-- 或者
ime.label_keys = { {key.space, '1', 'j', label = '1'}, {'k', '2', label = '2'}, {'l', '3', label = '3'}, {'4', ';', label = '4'} }
```

用 空格 选第一个词，显示为 'Spc'，左 Alt 选第二个词，右 Alt 选第三个词：
```
ime.label_keys = { {key.space, label = 'Spc'}, {key.Alt_L, label = 'LAlt'}, {key.Alt_R, label = 'RAlt'}}
```

### 缓存 ###

输入 'w' 的时候，不请求云服务器，直接返回 '我'，'真的吗' 同理：
```
request_cache['w'] = '我'
request_cache['zhen de ma'] = '真的吗？'
```

对于没有空格的单字拼音，按照 lua 的语法糖，也可以这样设置：
```
request_cache.yi = '咦'
request_cache.wu = '唔'
request_cache.e = '呃'
```

### 超时 ###

预请求默认是 0.8 秒超时，可以修改得大一些：
```
ime.pre_request_timeout = 1.2
```

普通的请求默认是 12 秒超时，可以改得小一点：
```
ime.request_timeout = 5.0
```

当选择一段新的文字之后，默认在 3 秒中按下有效，可以改得小一点：
```
ime.sel_timeout = 2
```

### 颜色 ###

使用部分灰度风格的配色方案：
```
ime.preedit_fore_color = ime.COLOR_NOCHANGE
ime.preedit_fore_color = ime.COLOR_NOCHANGE
ime.responsed_fore_color = ime.COLOR_NOCHANGE
ime.correcting_back_color = 0xAAAAAA
ime.requesting_back_color = 0xCCCCCC
```

### 桌面提示 ###

在切换中英文状态的时候不要显示桌面提示：
```
ime.show_notificaion = false
```

Ubuntu 发行版默认安装的是 `notify-osd` 而不是标准的 `libnotify` ，
`notify-osd` 不允许一个程序弹出多个提示，
也不能手动关闭一个正出现在屏幕上的提示，
导致新的提示需要等到旧的提示慢慢超时消失后才会显示出来。
如果等不及，可以：

直接用新的提示覆盖旧的提示：
```
ime.static_notification = true
```

### 启动选项 ###

不要尝试启动时更新 fetcher 脚本：
```
do_not_update_fetcher = true
```

不要在启动时加载词库：
```
do_not_load_database = true
```

不要尝试在启动时加载[远程脚本](http://ibus-sogoupycc.googlecode.com/svn/trunk/startup/autoload.lua)：
```
do_not_load_remote_script = true
```

### 扩展 ###

输入法支持自定义扩展，默认会加载远程脚本，添加一些扩展。

倘若要自己添加扩展，只要在配置文件中写上简单的几行代码，比如，“Hello world” 扩展：
```
ime.register_command(0, 0, "Hello world", "ime.notify('Hello world!')")
```

清空缓存，一般没有必要进行这个操作，但倘若有时候要试试看网络情况，可能需要这个功能：
```
ime.register_command(0, 0 , "清空缓存", "request_cache = {}")
```

插入完整的系统时间：
```
ime.register_command(0, 0, "插入系统时间", "ime.commit(os.date())")
```

通过 Ctrl + I 插入小时和分钟，比如 `7:02` 这种格式：
```
function insert_short_time()
	local t = os.date("*t")
	ime.commit(t.hour..":"..('%02d'):format(t.min))
end
ime.register_command(('i'):byte(), key.CONTROL_MASK, "插入短的时间 (^I)", "insert_short_time()")
```

热键是 Ctrl + Alt + N 的名为“春眠不觉晓”的扩展，它会提交 8 串拼音请求，完成《春晓》和《赋得古原草送别》两首诗的输入：
```
function foo()
	for _, v in pairs
	{'chun mian bu jue xiao', 'chu chu wen ti niao',
	 'ye lai feng yu sheng', 'hua luo zhi duo shao', '',
	 'li li yuan shang cao', 'yi sui yi ku rong',
	 'ye huo shao bu jin', 'chun feng chui you sheng'} do
		if #v > 1 then ime.request(v) end
		ime.commit('\n')
	end
end

ime.register_command(('n'):byte(), key.CONTROL_MASK + key.MOD1_MASK, "春眠不觉晓", "foo()")
```

扩展中也可以有网络请求，比如使用 Web 接口发送飞信，或者是使用在线翻译功能等，但要严格控制超时。

扩展中可以做太多的事情。With great power comes great responsibility ，请尽量避免让输入法泪流满面，比如，在扩展中开始一个小时的午睡。

有兴趣的同学可以参考[这篇文章](http://lihdd.net/2010/03/%E5%85%B3%E4%BA%8E-ibus-sogoupycc-%E7%9A%84%E6%89%A9%E5%B1%95/)，它讲述了现在扩展设计背后的故事，也给出了一些操作(比如发送飞信)的可能的做法。

### 杂项 ###

输入时保留拼音编辑串的最后一个字是拼音，不要出于预览目的转换成汉字：
```
ime.preedit_reserved_pinyin = 1 -- 如果要保留多个拼音，改成更大的数字即可
```