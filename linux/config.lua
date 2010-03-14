-- ibus-sogoupycc 配置文件 (Lua 兼容脚本)
-- 详细信息请参考 http://code.google.com/p/ibus-sogoupycc/wiki/Configuration

assert(ime.set_double_pinyin_scheme{
-- 自然码方案 (其他双拼方案参考 http://code.google.com/p/ibus-sogoupycc/wiki/Configuration)
	q = {"q", {"iu"}}, w = {"w", {"ia", "ua"}}, e = {"-", {"e"}}, r = {"r", {"uan", "er"}}, t = {"t", {"ve", "ue"}}, y = {"y", {"uai", "ing"}}, u = {"sh", {"u"}}, i = {"ch", {"i"}}, o = {"", {"o", "uo"}}, p = {"p", {"un"}},
	a = {"-", {"a"}}, s = {"s", {"ong", "iong"}}, d = {"d", {"uang", "iang"}}, f = {"f", {"en"}}, g = {"g", {"eng"}}, h = {"h", {"ang"}}, j = {"j", {"an"}}, k = {"k", {"ao"}}, l = {"l", {"ai"}},
	z = {"z", {"ei"}}, x = {"x", {"ie"}}, c = {"c", {"iao"}}, v = {"zh", {"ui", "v"}}, b = {"b", {"ou"}}, n = {"n", {"in"}}, m = {"m", {"ian"}},
} == 0)

-- 如果存在，使用用户的请求脚本
local user_fetcher = ime.USERCACHEDIR..'/fetcher'
local file = io.open(user_fetcher, 'r')
if file then file:close() ime.fetcher = user_fetcher end

-- 如果存在，则加载用户配置文件
local user_config = ime.USERCONFIGDIR..'/config.lua'
local file = io.open(user_config, 'r')
if file then file:close() dofile(user_config) ime.apply_settings() end

-- 加载 ime.PKGDATADIR .. '/db' 和 ime.USERDATADIR .. '/db' 下所有 .db 文件
if not do_not_load_database then
	local dbs, db = io.popen('ls ' .. ime.PKGDATADIR .. '/db/*.db ' .. ime.USERDATADIR .. '/db/*.db')
	repeat db = dbs:read('*line') if (db) then ime.load_database(db, 1) end until not db
	dbs:close()
end

-- 自动更新用户 fetcher 脚本
if not do_not_update_fetcher then
	os.execute("mkdir '"..ime.USERCACHEDIR.."' -p")
	pcall(function() require 'luarocks.require' end)
	local http = require('socket.http')
	local url = require('socket.url')
	http.TIMEOUT = 5
	local ret, c = http.request('http://ibus-sogouime.googlecode.com/svn/trunk/linux/fetcher')
	if c == 200 and ret and #ret > 100 then
		local fetcher_file = io.open(ime.USERCACHEDIR..'/fetcher','w')
		fetcher_file:write(ret)
		fetcher_file:close()
		os.execute("chmod +x '"..ime.USERCACHEDIR..'/fetcher'.."'")
	end
end