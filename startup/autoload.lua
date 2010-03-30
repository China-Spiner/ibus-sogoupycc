local current_version = "0.2.3"
local note_path = os.getenv('HOME')..'/.notes.txt'

function google_translate(text, langpair)
	if (not text) or #text == 0 then ime.notify('没有选定内容', '请先选定要进行翻译的内容', 'info') return end
	local url = 'http://ajax.googleapis.com/ajax/services/language/translate?v=1.0&q='..url.escape(text)..'&langpair='..url.escape(langpair)
	http.TIMEOUT = 3
	local ret = http.request(url)
	if ret then ret = ret:match('"translatedText":"(.-)"}') end
	if not ret then ime.notify('Google Translate', '翻译超时或失败', 'error') end
	return tostring(ret or text)
end

function add_to_note(text)
	if (not text) or #text == 0 then ime.notify('没有选定内容', '请先选定要添加到备忘录的内容', 'info') return end
	local file = io.open(note_path, 'a')
	if not file then ime.notify('添加到备忘录', '无法写入备忘录文件', 'error') return end
	file:write('## ', os.date(), '\n', text, '\n\n')
	file:close()
	ime.notify('已添加到备忘录', text)
end

function view_note()
	os.execute("[ -e '"..note_path.."' ] && xdg-open '"..note_path.."' &")
end

ime.register_command(0, 0 , "查看输入法版本", "ime.notify('输入法版本', '正在使用的版本：'..ime.VERSION..'\\n最新已经发布的版本："..current_version.."', 'info')")
ime.register_command(0, 0, "全双拼切换", "ime.use_double_pinyin = not ime.use_double_pinyin ime.apply_settings() ime.notify('全双拼切换', '已经切换到'..(ime.use_double_pinyin and '双拼' or '全拼'))")
ime.register_command(('t'):byte(), key.CONTROL_MASK + key.MOD1_MASK , "就地转换成繁体 (Ctrl + Alt + T)", "ime.commit(google_translate(ime.get_selection(), 'zh-CN|zh-TW'))")
ime.register_command(('e'):byte(), key.CONTROL_MASK + key.MOD1_MASK , "就地翻译成英文 (Ctrl + Alt + E)", "ime.commit(google_translate(ime.get_selection(), 'zh-CN|en'))")
ime.register_command(('J'):byte(), key.MOD1_MASK + key.SHIFT_MASK , "添加到备忘录 (Shift + Alt + J)", "add_to_note(ime.get_selection())")
ime.register_command(('K'):byte(), key.MOD1_MASK + key.SHIFT_MASK , "查看备忘录 (Shift + Alt + K)", "view_note()")

-- ibus-sogoupycc-autoload-end --

