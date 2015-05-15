**`Version 0.2.6`**

  * `[BUGFIX]` 修正 0.2.5 版本 (仅限此版本) 中无法使用字符串形式的 `ime.label_keys` 设置的问题 ([Issue 31](https://code.google.com/p/ibus-sogoupycc/issues/detail?id=31))

**2010.4.14 `Version 0.2.5`**

  * `[NEW]` 允许把拼音当作英文直接提交 (ime.raw\_preedit\_key)
  * `[NEW]` 允许在提示中保留一定数量的拼音字符 (ime.preedit\_reserved\_pinyin)
  * `[ENHANCE]` 新的图标
  * `[ENHANCE]` 如果预请求得到结果而相同的普通请求没有得到结果，及时使用预请求得到的结果。
  * `[ENHANCE]` 全拼下智能切断拼音 ([Issue 26](https://code.google.com/p/ibus-sogoupycc/issues/detail?id=26))
  * `[ENHANCE]` 一些其他的改进
  * `[BUGFIX]` 修正了选词列表中不显示云服务器给的超过两个汉字的词组的问题 ([Issue 22](https://code.google.com/p/ibus-sogoupycc/issues/detail?id=22))
  * `[BUGFIX]` 修正了用 ESC 退出纠正模式后，再进入时选词列表并不是要纠正内容的问题 ([Issue 23](https://code.google.com/p/ibus-sogoupycc/issues/detail?id=23))
  * `[BUGFIX]` 修正了 ibus 1.2.99 (仅此特定版本) 下不能使用的问题 ([Issue 25](https://code.google.com/p/ibus-sogoupycc/issues/detail?id=25))
  * `[BUGFIX]` 修正了某些系统下，嵌入应用程序的提示文字不显示的问题 ([Issue 28](https://code.google.com/p/ibus-sogoupycc/issues/detail?id=28))
  * `[BUGFIX]` 修正了没有加载词库时，程序很快崩溃的问题

**2010.4.1 `Version 0.2.4`**

> _此版本主要是对之前版本的问题作出修正_

  * `[ENHANCE]` 改进请求超时控制方法
  * `[BUGFIX]` 修正了部分扩展导致输入法死锁而无法使用的问题 ([Issue 20](https://code.google.com/p/ibus-sogoupycc/issues/detail?id=20))
  * `[BUGFIX]` 修正了 0.2.3 版本使用一段时间后，只使用本地词库结果的问题 ([Issue 21](https://code.google.com/p/ibus-sogoupycc/issues/detail?id=21))

**2010.3.31 `Version 0.2.3`**

  * `[NEW]` 允许严格控制超时时间
  * `[NEW]` 允许在选词中使用云服务器的词库
  * `[NEW]` 使用不同颜色区分不同词库结果
  * `[NEW]` 通过远程脚本默认提供一些扩展
  * `[ENHANCE]` 增强的预请求的逻辑
  * `[ENHANCE]` 许多其他的改进
  * `[BUGFIX]` 修正浮点数选项被截断为整数
  * `[REMOVE]` 删除 `ime.fetcher_buffer_size` 选项
  * `[REMOVE]` 取消内嵌 gb2312 词库二次选字

**2010.3.30 `Version 0.2.2`**

> _此版本主要是对 0.2.1 版本新引入功能作出修正_

  * `[ENHANCE]` 改进本地词库整句查询方法，大幅减少查询次数，使用缓存加快速度
  * `[ENHANCE]` 一些其他的改进
  * `[BUGFIX]` 修正智能选择中英文标点，无法正确处理需要使用 Shift 键选择的标点
  * `[BUGFIX]` 修正选词列表中出现重复的项
  * `[REMOVE]` 删除 `ime.fallback_min_cache_length` 选项

**2010.3.29 `Version 0.2.1`**

  * `[NEW]` 兼容 ibus 1.3.0
  * `[NEW]` 不合理拼音作为英文处理 (比如，中文状态下直接输入"www"会自动转为英文状态)
  * `[NEW]` 根据之前输入的内容，智能选择中英文标点 (比如，中文状态下可以直接输入"1.234"，不用担心点号变成句号)
  * `[NEW]` 云服务无效时，使用本地词库转换拼音 (在网络故障时也提供一定的输入能力)
  * `[NEW]` 一键 (默认为 右Alt) 使用本地词库强制补全，完成所有未完成的网络请求，不再需要苦苦等待
  * `[ENHANCE]` 软件包默认携带带有索引的词库，查询速度加快许多
  * `[ENHANCE]` 一些其他的改进
  * `[BUGFIX]` 修正 `ime.pre_request` 选项无作用的问题

**2010.3.18 `Version 0.2.0`**

> _此版本的配置和以前版本不再兼容_

  * `[NEW]` 自定义扩展
  * `[NEW]` 同一功能定义多个热键
  * `[NEW]` Notification 支持(libnotify)
  * `[NEW]` 纠正模式下显示汉字，并支持查询多音字
  * `[NEW]` 选词支持一词多键，支持使用 Alt 等特殊键选词
  * `[NEW]` 不严格双拼(全双拼混合输入)模式
  * `[NEW]` 预请求模式，提前获得结果并看到拼音转换结果
  * `[ENHANCE]` 后台化部分初始化过程，加快首次切换到输入法界面响应速度
  * `[ENHANCE]` 允许一定的错误，配置文件出错不再导致输入法退出
  * `[ENHANCE]` 加快词库查询速度
  * `[ENHANCE]` 优化内部逻辑，标点符号更容易定制，缓存目前全局生效
  * `[ENHANCE]` 纠正模式下 ESC 键改为取消纠正，不会删除正在纠正的内容
  * `[ENHANCE]` 简单化配置文件，删除默认配置，将大量注释说明移至在线 wiki
  * `[ENHANCE]` 使用 XDG 标准存放配置文件 ([Issue 17](https://code.google.com/p/ibus-sogoupycc/issues/detail?id=17))
  * `[BUGFIX]` `dei` 视为合法拼音
  * `[BUGFIX]` 修正一处内存泄漏
  * `[BUGFIX]` 64 位系统下通过编译
  * `[REMOVE]` pre-session 配置。现在所有配置都是全局的
  * `[REMOVE]` 用户 `processkey` 定制能力以及相关可调用 lua 函数
  * `[REMOVE]` 出于兼容性考虑，删除 `tableOrientation` 选项

**2010.3.9 `Version 0.1.3`**

  * `[NEW]` 自动更新通讯协议
  * `[ENHANCE]` 更新通讯协议至 2010.3.9 ，同官方版本(旧协议似乎被官方废弃)
  * `[ENHANCE]` 允许设置词典结果最大长度，加快查询速度
  * `[BUGFIX]` 修复足够长的拼音会导致程序崩溃的情况 ([Issue 14](https://code.google.com/p/ibus-sogoupycc/issues/detail?id=14))

**2010.3.3 `Version 0.1.2`**

  * `[NEW]` 支持全拼下使用简拼(只有声母)，以及选词，默认使用全拼 ([Issue 10](https://code.google.com/p/ibus-sogoupycc/issues/detail?id=10))
  * `[NEW]` 添加正在请求的图标提示，以及部分数据统计
  * `[ENHANCE]` 允许用 ESC 退出纠正模式
  * `[ENHANCE]` 独立纠正模式下拼音颜色选项
  * `[ENHANCE]` 缓存纠正模式选词结果
  * `[ENHANCE]` 增加网络错误时的重试次数
  * `[ENHANCE]` 在低版本 ibus (1.2.0.20090927) (ubuntu 9.10 提供) 下通过编译 ([Issue 9](https://code.google.com/p/ibus-sogoupycc/issues/detail?id=9))
  * `[BUGFIX]` 选中拼音进入纠正模式，不再提交多余的空格
  * `[BUGFIX]` 纠正模式中使用正确的提交文本方式
  * `[BUGFIX]` 有正在请求的字符串时，正确响应回车键
  * `[BUGFIX]` 不再缓存失败的请求
  * `[BUGFIX]` 修复有些系统下，中文模式下还是只能输入英文 ([Issue 8](https://code.google.com/p/ibus-sogoupycc/issues/detail?id=8))

**2010.2.28 `Version 0.1.1`**

> _此版本主要是对之前版本的问题作出修正_

  * `[ENHANCE]` 改用更稳定的 GTK 方法读取选中内容
  * `[ENHANCE]` 添加中文标点 '\' = '、'
  * `[BUGFIX]` 按下纠正模式热键后程序有时崩溃 ([Issue 7](https://code.google.com/p/ibus-sogoupycc/issues/detail?id=7))
  * `[BUGFIX]` 纠正模式中识别韵母 ing, iang

**2010.2.27 `Version 0.1.0`**

  * `[NEW]` 纠正模式和选词列表 ([Issue 3](https://code.google.com/p/ibus-sogoupycc/issues/detail?id=3))
  * `[NEW]` 通过图标显示中英文状态
  * `[NEW]` 加载 ibus-pinyin 词库
  * `[NEW]` 内嵌 gb2312 字库
  * `[NEW]` lua 配置文件
  * `[ENHANCE]` 完整支持各种双拼布局
  * `[ENHANCE]` 新的程序图标
  * `[ENHANCE]` 改用 cmake 构建系统 ([Issue 1](https://code.google.com/p/ibus-sogoupycc/issues/detail?id=1))
  * `[BUGFIX]` 多线程请求中程序崩溃
  * `[BUGFIX]` 屏幕显示中，正在请求的背景色错位
  * `[REMOVE]` 在拼音串中移动光标

**2009.11.4 `Version svn-r18`**