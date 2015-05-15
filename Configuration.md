## 用户配置文件 ##

一般作为普通用户，没有权限修改全局配置文件，可以将配置文件放在用户目录，就可以编辑：

```
mkdir -p $XDG_CONFIG_HOME/ibus/sogoupycc
gvim $XDG_CONFIG_HOME/ibus/sogoupycc/config.lua
```

默认情况下，用户配置文件和全局配置文件同时生效。`$XDG_CONFIG_HOME` 一般为 `$HOME/.config/`。

## 配置文件说明 ##

### 语法说明 ###
配置文件实际上是一个 lua 脚本，遵循 lua 语法。

如果你不熟悉 lua，修改配置文件没有生效，请先确认代码不是注释。lua 的注释规则是：
```
-- 以双减号开头，到行尾都是的是注释，所以这一行是注释
blabla -- 这里也是注释，但前面的 blabla 不是
-- 被 --[[ 和 --]] 括起来的部分都是注释
--[[ 这里就是注释
这一行也是注释
这里还是注释，但后面的 blabla 不是 --]] blabla
```

如果配置文件中有产生错误，从命令行运行输入法可以看到错在哪里。

**注意** 如果你熟悉 lua ，那么下面的内容将对你有帮助。如果你不熟悉，也可以直接修改配置，请参考[配置样例](http://code.google.com/p/ibus-sogoupycc/wiki/Tutorial/ConfigurationExample)

### 常量说明 ###

| **名称** | **说明** |
|:-----------|:-----------|
| ime.VERSION | 版本字符串 |
| ime.PKGDATADIR | 软件包路径<br>比如 "/usr/share/ibus-sogoupycc" <br>
<tr><td> ime.USERCONFIGDIR </td><td> 用户配置路径<br>比如 "/home/user/.config/ibus/sogoupycc" </td></tr>
<tr><td> ime.USERCACHEDIR </td><td> 用户缓存路径<br>比如 "/home/user/.cache/ibus/sogoupycc" </td></tr>
<tr><td> ime.USERDATADIR </td><td> 用户数据路径<br>比如 "/home/quark/.local/share/ibus/sogoupycc"  </td></tr>
<tr><td> ime.DIRSEPARATOR </td><td> 目录分割符号<br>应该是 "/"</td></tr>
<tr><td> ime.INVALID_COLOR </td><td> 无效的颜色，表示不修改该项颜色 </td></tr>
<tr><td> key.None </td><td> 不是任何一个键 </td></tr>
<tr><td> key.Shift_L </td><td> Left Shift </td></tr>
<tr><td> key.Shift_R </td><td> Right Shift </td></tr>
<tr><td> key.Control_L </td><td> Left Control </td></tr>
<tr><td> key.Control_R </td><td> Right Control </td></tr>
<tr><td> key.Alt_L </td><td> Left Alt </td></tr>
<tr><td> key.Alt_R </td><td> Right Alt </td></tr>
<tr><td> key.Tab </td><td> Tab </td></tr>
<tr><td> key.space </td><td> Space </td></tr>
<tr><td> key.Return </td><td> Enter </td></tr>
<tr><td> key.BackSpace </td><td> Backspace </td></tr>
<tr><td> key.Escape </td><td> Escape </td></tr>
<tr><td> key.Delete </td><td> Delete </td></tr>
<tr><td> key.Page_Up </td><td> Page Up </td></tr>
<tr><td> key.Page_Down </td><td> Page Down </td></tr>
<tr><td> key.CONTROL_MASK </td><td> Control (在 <i>modifiers</i> 中使用，下同) </td></tr>
<tr><td> key.SHIFT_MASK </td><td> Shift </td></tr>
<tr><td> key.LOCK_MASK </td><td> Caps Lock </td></tr>
<tr><td> key.MOD1_MASK </td><td> 通常是  Alt_L, Alt_R, Meta_L </td></tr>
<tr><td> key.MOD4_MASK </td><td> 通常是 Super_L, Hyper_L </td></tr>
<tr><td> key.SUPER_MASK </td><td> Super (可能不能用) </td></tr>
<tr><td> key.META_MASK </td><td> Meta </td></tr></tbody></table>

<i>如果你想使用上表未列出的键，可以参考 ibuskeysyms.h</i>

<h3>配置项说明</h3>

<table><thead><th> <b>名称</b> </th><th> <b>说明</b> </th><th> <b>默认值</b> </th><th> <b>要求版本</b><br><b>最后改动版本</b> </th></thead><tbody>
<tr><td> ime.db_result_limit </td><td> 外部字词库结果限制<br>0 表示无限制 </td><td> 128，结果最多 128 个 </td><td> 0.2.0 </td></tr>
<tr><td> ime.db_length_limit </td><td> 外部词库查询时长度限制<br>应该在 1 到 15 之间 </td><td> 10，不查询超过 10 个汉字的词 </td><td> 0.2.0 </td></tr>
<tr><td> ime.db_phrase_adjust </td><td> 长词组位置调整<br>0 为没有调整，负数向后调整，正数向前调整 </td><td> 1.2，将长词组位置稍微提前 </td><td> 0.2.0 </td></tr>
<tr><td> ime.db_completion_adjust </td><td> 使用本地词库补全时，长词组位置调整<br>0 为没有调整，负数向后调整，正数向前调整 </td><td> 4，将长词组位置大幅度提前 </td><td> 0.2.1 </td></tr>
<tr><td> ime.db_query_order </td><td> 字词库查询顺序<br>"2" 为内部 gb2312 单字库 (将按音调列出)<br>"d" 为外部字词库 (ibus-pinyin 1.2.99 兼容)<br>"c" 为部分缓存结果<br>"w" 为云服务器提供的词语<br>'2d' 即为，先列出内部 gb2312 单字库，<br>紧跟着列出外部字词库 (如果有的话)。 </td><td> 'cwd2' </td><td> 0.2.0<br>0.2.3 </td></tr>
<tr><td> ime.fallback_use_db </td><td> 如果请求有失败的情况，使用加载的第一个本地词库补全 </td><td> true<br>如果没有加载词库，那么此选项无效 </td><td> 0.2.1 </td></tr>
<tr><td> ime.fallback_pre_request </td><td> 是否在预请求超时的时候，也使用本地词库补全<br>此项为 true 会重写 ime.cache_requests </td><td> true </td><td> 0.2.1 </td></tr>
<tr><td> ime.preedit_fore_color </td><td> 正在编辑的拼音前景色 </td><td> 0x0050FF </td><td> 0.2.0 </td></tr>
<tr><td> ime.preedit_back_color </td><td> 正在编辑的拼音背景色 </td><td> ime.COLOR_NOCHANGE </td><td> 0.2.0 </td></tr>
<tr><td> ime.local_cache_fore_color </td><td> 本地词库预请求结果前景色 </td><td> 0x8C8C8C </td><td> 0.2.1 </td></tr>
<tr><td> ime.local_cache_back_color </td><td> 本地词库预请求结果背景色 </td><td> ime.COLOR_NOCHANGE </td><td> 0.2.1 </td></tr>
<tr><td> ime.cloud_cache_fore_color </td><td> 云预请求结果前景色 </td><td> 0x0050FF </td><td> 0.2.1 </td></tr>
<tr><td> ime.cloud_cache_back_color </td><td> 云预请求结果前景色 </td><td> ime.COLOR_NOCHANGE </td><td> 0.2.1 </td></tr>
<tr><td> ime.requesting_fore_color </td><td> 请求中的拼音前景色 </td><td> ime.COLOR_NOCHANGE </td><td> 0.2.0 </td></tr>
<tr><td> ime.requesting_back_color </td><td> 请求中的拼音背景色 </td><td> 0xB8CFE5 </td><td> 0.2.0 </td></tr>
<tr><td> ime.responsed_fore_color </td><td> 请求完成的字串前景色 </td><td> 0x0024B2 </td><td> 0.2.0 </td></tr>
<tr><td> ime.responsed_back_color </td><td> 请求完成的字串背景色 </td><td> ime.COLOR_NOCHANGE </td><td> 0.2.0 </td></tr>
<tr><td> ime.correcting_fore_color </td><td> 纠正模式中文字前景色 </td><td> ime.COLOR_NOCHANGE </td><td> 0.2.0 </td></tr>
<tr><td> ime.correcting_back_color </td><td> 纠正模式中文字背景色 </td><td> 0xFFB442 </td><td> 0.2.0 </td></tr>
<tr><td> ime.cloud_cache_candicate_color </td><td> 选词时缓存词条的颜色 </td><td> 0x0050FF </td><td> 0.2.3 </td></tr>
<tr><td> ime.cloud_word_candicate_color </td><td> 选词时云服务器给出词条的颜色 </td><td> 0x00A811 </td><td> 0.2.3 </td></tr>
<tr><td> ime.database_candicate_color </td><td> 选词时本地词库词条的颜色 </td><td> ime.COLOR_NOCHANGE </td><td> 0.2.3 </td></tr>
<tr><td> ime.internal_candicate_color </td><td> 选词时内置 gb2312 字库词条的颜色 </td><td> 0x8C8C8C </td><td> 0.2.3 </td></tr>
<tr><td> ime.multi_tone_limit </td><td> 纠正模式允许匹配的多音字情况数<br>过大的值会使得纠正模式查词缓慢 </td><td> 4<br>允许有 4 种不同的读音出现 </td><td> 0.2.0 </td></tr>
<tr><td> ime.sel_timeout </td><td> 当选择一段新的文字之后，<br>如果超过这个时间，<br>则再按纠正热键也不会起作用 </td><td> 3.0<br>3 秒钟后超时 </td><td> 0.2.0 </td></tr>
<tr><td> ime.show_cache_preedit </td><td> 是否在正在编辑的拼音串中显示有缓存的结果 </td><td> true </td><td> 0.2.0 </td></tr>
<tr><td> ime.pre_request </td><td> 是否预先发送请求并缓存，如果为 true<br>则 <code>ime.cache_requests = true</code>  </td><td> true </td><td> 0.2.0 </td></tr>
<tr><td> ime.pre_request_timeout </td><td> 预请求超时 </td><td> 0.6，0.6 秒钟 </td><td> 0.2.0<br>0.2.5 </td></tr>
<tr><td> ime.pre_request_retry </td><td> 为相同的内容发送预请求最多多少次 </td><td> 4，4次 </td><td> 0.2.3 </td></tr>
<tr><td> ime.preedit_reserved_pinyin </td><td> 拼音编辑串中保留最右边多上个拼音 </td><td> 0 </td><td> 0.2.5 </td></tr>
<tr><td> ime.request_timeout </td><td> 普通请求的超时 </td><td> 12.0，12 秒钟 </td><td> 0.2.1<br>0.2.3 </td></tr>
<tr><td> ime.strict_timeout </td><td> 使用严格的超时控制，否则，使用粗略的超时控制<br>如果此选项造成输入法不稳定，请改成 false </td><td> true </td><td> 0.2.3 </td></tr>
<tr><td> ime.quick_response_key </td><td> 非选词时，迅速让正在进行的所有请求超时，<br>完成未完成的请求，不管结果质量如何 (需要词库支持) </td><td> key.Alt_R </td><td> 0.2.1 </td></tr>
<tr><td> ime.eng_mode_key </td><td> 中文模式切换到英文模式热键 </td><td> key.Shift_L </td><td> 0.2.0 </td></tr>
<tr><td> ime.chs_mode_key </td><td> 英文模式切换到中文模式热键 </td><td> key.Shift_L </td><td> 0.2.0 </td></tr>
<tr><td> ime.correction_mode_key </td><td> 纠正模式和自选词热键 </td><td> key.Tab </td><td> 0.2.0 </td></tr>
<tr><td> ime.page_down_key </td><td> 选词列表向下翻页热键 </td><td> ('h'):byte() </td><td> 0.2.0 </td></tr>
<tr><td> ime.page_down_key </td><td> 选词列表向上翻页热键 </td><td> ('g'):byte() </td><td> 0.2.0 </td></tr>
<tr><td> ime.raw_preedit_key </td><td> (中文状态下)直接提交输入英文内容热键 </td><td> key.Shift_R </td><td> 0.2.5 </td></tr>
<tr><td> ime.show_notificaion </td><td> 是否显示桌面提示 (via libnotify) </td><td> true </td><td> 0.2.0 </td></tr>
<tr><td> ime.static_notification </td><td> 桌面提示是否最多显示一个<br>(建议 notify-osd 用户开启此项) </td><td> false </td><td> 0.2.0 </td></tr>
<tr><td> ime.use_double_pinyin </td><td> 是否使用双拼 </td><td> false </td><td> 0.2.0 </td></tr>
<tr><td> ime.strict_double_pinyin </td><td> 是否只接受严格的双拼输入<br>(双拼模式下) </td><td> false，不能解析的双拼字符串视为全拼<br>如果为 true，拒绝不合法的双拼输入 </td><td> 0.2.0<br>0.2.1 </td></tr>
<tr><td> ime.auto_eng_tolerance </td><td> 当连续多少个不完整的拼音，认为是实际不是拼音，自动切换到英文状态<br>如果是负数，则禁用自动切换到英文功能 </td><td> 5 </td><td> 0.2.1<br>0.2.5 </td></tr>
<tr><td> ime.start_in_eng_mode </td><td> 是否英文模式 </td><td> false </td><td> 0.2.0 </td></tr>
<tr><td> ime.cache_requests </td><td> 是否缓存请求和自选词的结果，<br>可能被 <code>ime.pre_request</code> 改写为 true </td><td> true </td><td> 0.2.0 </td></tr>
<tr><td> ime.label_keys </td><td> 用于选词的按键<br>由于 ibus 限制，最多 16 个<br>可以是字符串或者按键数组 </td><td> "jkl;uiopasdf" </td><td> 0.2.0 </td></tr>
<tr><td> ime.fetcher_path </td><td> 云请求脚本路径<br>该脚本接受一个可能用空格做分割的拼音字符串为参数，<br>以及一个可选的浮点数作为超时时间 (第二个参数)<br>将请求结果写到标准输出，<br>该脚本需处理超时情况，<br>若出现错误，应该输出一空行 </td><td> ime.PKGDATADIR .. "/fetcher" </td><td> 0.2.0 </td></tr>
<tr><td> ime.full_pinyin_adjustments </td><td> 全拼分隔符修正表格 </td><td> 参见全局配置文件 </td><td> 0.2.5 </td></tr>
<tr><td> ime.punc_after_chinese </td><td> 出现在其中的英文标点，<br>只有在刚输入完汉字之后才使用对应的中文标点 </td><td> <code>".,:"</code> </td><td> 0.2.1<br>0.2.2 </td></tr>
<tr><td> ime.punc_map </td><td> 中英文标点符号转换表<br>对于多状态的标点，允许使用数组表示 </td><td> 默认为 nil，此时相当于下方代码 </td><td> 0.2.0 </td></tr></tbody></table>

<pre><code>ime.punc_map = { ['.'] = '。', [','] = '，', ['^'] = '……', ['@'] = '·',<br>
 ['!'] = '！', ['~'] = '～', ['?'] = '？',<br>
 ['#'] = '＃', ['$'] = '￥', ['&amp;'] = '＆',<br>
 ['('] = ' (', [')'] = ')', ['{'] = '｛',<br>
 ['}'] = '｝', ['['] = '［', [']'] = '］',<br>
 [';'] = '；', [':'] = '：', ['&lt;'] = '《',<br>
 ['&gt;'] = '》', ['\\'] = '、',<br>
 ["'"] = { '‘', '’'}, ['"'] = { '“', '”'}<br>
 }<br>
</code></pre>

<h3>缓存</h3>

<code>_</code>G.request_cache 是一个用于请求和自选词的结果的缓存，可以用来设置简单的用户词库，例如<br>
<pre><code>request_cache['w'] = '我'<br>
request_cache['zhen de ma'] = '真的吗？'<br>
</code></pre>

<h3>函数说明</h3>

通过函数设置的选项皆为立即生效<br>
<br>
<table><thead><th> <b>名称</b> </th><th> <b>说明</b> </th><th> <b>参数</b> </th><th> <b>返回值</b> </th><th> <b>要求版本</b> </th></thead><tbody>
<tr><td> ime.set_debug_level(level) </td><td> 设置调试信息输出级别 </td><td> level<br>0 到 10 之间的整数<br>0 为不输出任何调试信息 </td><td> 无 </td><td> 0.2.0 </td></tr>
<tr><td> ime.load_database<br>(filename, weight) </td><td> 加载 ibus-pinyin 1.2 兼容词库 </td><td> filename 字符串，文件名<br>weight 浮点数，权重系数<br>加载多个词库后，查询时将乘以各自的权重 </td><td> 第一次加载成功为 1<br>否则为 0 </td><td> 0.2.0 </td></tr>
<tr><td> ime.chars_to_pinyin(chars) </td><td> 将汉字转换回拼音 </td><td> chars 字符串，一串汉字 </td><td> 用空格分隔的全拼字符串 </td><td> 0.2.0 </td></tr>
<tr><td> ime.double_to_full_pinyin(double_pinyin) </td><td> 将双拼字符串转换成全拼 </td><td> double_pinyin 双拼字符串，比如 "womf" </td><td> 用空格分隔的全拼字符串 </td><td> 0.2.0 </td></tr>
<tr><td> ime.database_count() </td><td> 查询已经加载的外部词库个数 </td><td> 无 </td><td> 一个整数，表示已经加载的词库数 </td><td> 0.2.0 </td></tr>
<tr><td> ime.apply_settings() </td><td> 立即应用设置<br>一般设置会在配置脚本执行完后读入<br>这个函数可以让输入法立即读入那些变量表示的设置 </td><td> 无 </td><td> 无 </td><td> 0.2.0 </td></tr>
<tr><td> ime.set_double_pinyin_scheme<br>(scheme) </td><td> 设置双拼方案<br>输入法没有内置默认双拼方案<br>使用双拼一定要先设置双拼方案 </td><td> scheme 双拼方案，<br>在声母部分使用 "-" 表示禁用，零声母使用 ""，<br>具体请参考默认配置文件，常见的双拼方案见本页后面 </td><td> 歧义数 (按两个键，可能有多种全拼解释为歧义)<br>正确的双拼布局歧义数应该是 0 </td><td> 0.2.0 </td></tr>
<tr><td> ime.register_command<br>(key, modifiers, label, script) </td><td> 注册一个扩展 </td><td> key 按键 <b>(只能是数字，即 <code>key.xxx</code> 形式或者 <code>('k'):byte()</code> )</b><br>modifiers 组合键状态，只能是数字<br>label 字符串，名称<br>script 字符串，一段 lua 脚本 </td><td> 无 </td><td> 0.2.0 </td></tr>
<tr><td> ime.bitand(a, b) </td><td> 按位与 </td><td> a, b 两个整数 </td><td> 按位与结果 </td><td> 0.2.0 </td></tr>
<tr><td> ime.bitor(a, b) </td><td> 按位或 </td><td> a, b 两个整数 </td><td> 按位或结果 </td><td> 0.2.0 </td></tr>
<tr><td> ime.get_selection() </td><td> 获得当前选定内容 </td><td> 无 </td><td> 字符串，选定内容，可能是用户很久以前选定的内容 </td><td> 0.2.0 </td></tr>
<tr><td> ime.commit(text) </td><td> 向输入法提交文本 </td><td> text 字符串，要提交的文本 </td><td> 无 </td><td> 0.2.0 </td></tr>
<tr><td> ime.request(text, func) </td><td> 向云服务器发送请求 </td><td> text 字符串，要提交的文本<br>func 可选，字符串，函数名，该函数负责转换文本(可能阻塞)<br>该函数接受一个字符串，返回一个字符串 </td><td> 无 </td><td> 0.2.0 </td></tr>
<tr><td> ime.execute(script) </td><td> 执行脚本 </td><td> script lua脚本 </td><td> 布尔值，如果没有错误则为 true </td><td> 0.2.0 </td></tr>
<tr><td> ime.notify<br>(summary, details, icon) </td><td> 显示消息 </td><td> summary 字符串，details 可选字符串<br>icon 可选字符串，图标文件路径 </td><td> 布尔值，如果成功， true </td><td> 0.2.0 </td></tr></tbody></table>

<h2>双拼方案</h2>

这里列出了一些常用的双拼方案<br>
<pre><code>--[[<br>
-- 微软拼音方案<br>
	q = {"q", {"iu"}}, w = {"w", {"ia", "ua"}}, e = {"-", {"e"}}, r = {"r", {"uan", "er"}}, t = {"t", {"ue"}}, y = {"y", {"uai", "v"}}, u = {"sh", {"u"}}, i = {"ch", {"i"}}, o = {"", {"o", "uo"}}, p = {"p", {"un"}},<br>
	a = {"-", {"a"}}, s = {"s", {"ong", "iong"}}, d = {"d", {"uang", "iang"}}, f = {"f", {"en"}}, g = {"g", {"eng"}}, h = {"h", {"ang"}}, j = {"j", {"an"}}, k = {"k", {"ao"}}, l = {"l", {"ai"}}, [';'] = {"-", {"ing"}},<br>
	z = {"z", {"ei"}}, x = {"x", {"ie"}}, c = {"c", {"iao"}}, v = {"zh", {"ui", "ue", "ve"}}, b = {"b", {"ou"}}, n = {"n", {"in"}}, m = {"m", {"ian"}},<br>
-- 搜狗拼音方案 (类似微软拼音方案，不同在于 've' 的位置，微软拼音方案在 V 键，搜狗拼音方案在 T 键。目前没有将 've' 列入合法拼音，全部使用 'ue' ，所以两方案实际相同)<br>
	q = {"q", {"iu"}}, w = {"w", {"ia", "ua"}}, e = {"-", {"e"}}, r = {"r", {"uan", "er"}}, t = {"t", {"ue", "ve"}}, y = {"y", {"uai", "v"}}, u = {"sh", {"u"}}, i = {"ch", {"i"}}, o = {"", {"o", "uo"}}, p = {"p", {"un"}},<br>
	a = {"-", {"a"}}, s = {"s", {"ong", "iong"}}, d = {"d", {"uang", "iang"}}, f = {"f", {"en"}}, g = {"g", {"eng"}}, h = {"h", {"ang"}}, j = {"j", {"an"}}, k = {"k", {"ao"}}, l = {"l", {"ai"}}, [';'] = {"-", {"ing"}},<br>
	z = {"z", {"ei"}}, x = {"x", {"ie"}}, c = {"c", {"iao"}}, v = {"zh", {"ui"}}, b = {"b", {"ou"}}, n = {"n", {"in"}}, m = {"m", {"ian"}},<br>
-- 自然码方案 (类似搜狗拼音和微软拼音方案，自然码方案中 'ing' 由 ; 移动到了 Y 键， 'v' 由 Y 移到了 V 键)<br>
	q = {"q", {"iu"}}, w = {"w", {"ia", "ua"}}, e = {"-", {"e"}}, r = {"r", {"uan", "er"}}, t = {"t", {"ve", "ue"}}, y = {"y", {"uai", "ing"}}, u = {"sh", {"u"}}, i = {"ch", {"i"}}, o = {"", {"o", "uo"}}, p = {"p", {"un"}},<br>
	a = {"-", {"a"}}, s = {"s", {"ong", "iong"}}, d = {"d", {"uang", "iang"}}, f = {"f", {"en"}}, g = {"g", {"eng"}}, h = {"h", {"ang"}}, j = {"j", {"an"}}, k = {"k", {"ao"}}, l = {"l", {"ai"}},<br>
	z = {"z", {"ei"}}, x = {"x", {"ie"}}, c = {"c", {"iao"}}, v = {"zh", {"ui", "v"}}, b = {"b", {"ou"}}, n = {"n", {"in"}}, m = {"m", {"ian"}},<br>
-- 拼音加加方案<br>
	q = {"q", {"er", "ing"}}, w = {"w", {"ei"}}, e = {"-", {"e"}}, r = {"r", {"en"}}, t = {"t", {"eng"}}, y = {"y", {"iong", "ong"}}, u = {"ch", {"u"}}, i = {"sh", {"i"}}, o = {"", {"o", "uo"}}, p = {"p", {"ou"}},<br>
	a = {"-", {"a"}}, s = {"s", {"ai"}}, d = {"d", {"ao"}}, f = {"f", {"an"}}, g = {"g", {"ang"}}, h = {"h", {"iang", "uang"}}, j = {"j", {"ian"}}, k = {"k", {"iao"}}, l = {"l", {"in"}},<br>
	z = {"z", {"un"}}, x = {"x", {"uai", "ue", "ve"}}, c = {"c", {"uan"}}, v = {"zh", {"ui", "v"}}, b = {"b", {"ia", "ua"}}, n = {"n", {"iu"}}, m = {"m", {"ie"}},<br>
-- 智能 ABC 方案<br>
	q = {"q", {"ei"}}, w = {"w", {"ian"}}, e = {"-", {"e"}}, r = {"r", {"iu", "er"}}, t = {"t", {"iang", "uang"}}, y = {"y", {"ing"}}, u = {"-", {"u"}}, i = {"-", {"i"}}, o = {"", {"o", "uo"}}, p = {"p", {"uan"}},<br>
	a = {"zh", {"a"}}, s = {"s", {"ong", "iong"}}, d = {"d", {"ia", "ua"}}, f = {"f", {"en"}}, g = {"g", {"eng"}}, h = {"h", {"ang"}}, j = {"j", {"an"}}, k = {"k", {"ao"}}, l = {"l", {"ai"}},<br>
	z = {"z", {"iao"}}, x = {"x", {"ie"}}, c = {"c", {"in", "uai"}}, v = {"sh", {"v", "ve", "ue"}}, b = {"b", {"ou"}}, n = {"n", {"un"}}, m = {"m", {"ue", "ve", "ui"}},<br>
-- 紫光拼音方案<br>
	q = {"q", {"ao"}}, w = {"w", {"en"}}, e = {"-", {"e"}}, r = {"r", {"an"}}, t = {"t", {"eng"}}, y = {"y", {"in", "uai"}}, u = {"zh", {"u"}}, i = {"sh", {"i"}}, o = {"", {"o", "uo"}}, p = {"p", {"ai"}},<br>
	a = {"-", {"a"}}, s = {"s", {"ang"}}, d = {"d", {"ie"}}, f = {"f", {"ian"}}, g = {"g", {"iang", "uang"}}, h = {"h", {"iong", "ong"}}, j = {"j", {"er", "iu"}}, k = {"k", {"ei"}}, l = {"l", {"uan"}}, [';'] = {"-", {"ing"}},<br>
	z = {"z", {"ou"}}, x = {"x", {"ia", "ua"}}, c = {"c", {}}, v = {"-", {"ui", "v"}}, b = {"b", {"iao"}}, n = {"n", {"ue", "ve", "ui"}}, m = {"m", {"un"}},<br>
-- 紫光拼音方案改 ('ing' 由 ; 移动到 C 键)<br>
	q = {"q", {"ao"}}, w = {"w", {"en"}}, e = {"-", {"e"}}, r = {"r", {"an"}}, t = {"t", {"eng"}}, y = {"y", {"in", "uai"}}, u = {"zh", {"u"}}, i = {"sh", {"i"}}, o = {"", {"o", "uo"}}, p = {"p", {"ai"}},<br>
	a = {"-", {"a"}}, s = {"s", {"ang"}}, d = {"d", {"ie"}}, f = {"f", {"ian"}}, g = {"g", {"iang", "uang"}}, h = {"h", {"iong", "ong"}}, j = {"j", {"er", "iu"}}, k = {"k", {"ei"}}, l = {"l", {"uan"}},<br>
	z = {"z", {"ou"}}, x = {"x", {"ia", "ua"}}, c = {"c", {"ing"}}, v = {"-", {"ui", "v"}}, b = {"b", {"iao"}}, n = {"n", {"ue", "ve", "ui"}}, m = {"m", {"un"}},<br>
--]]<br>
</code></pre>