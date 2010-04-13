-- ibus-sogoupycc 配置文件 (Lua 兼容脚本)
-- 详细信息请参考 http://code.google.com/p/ibus-sogoupycc/wiki/Configuration

-- 加载 luasocket 模块
http, url = require('socket.http'), require('socket.url')

assert(ime.set_double_pinyin_scheme{
-- 自然码方案 (其他双拼方案参考 http://code.google.com/p/ibus-sogoupycc/wiki/Configuration)
	q = {"q", {"iu"}}, w = {"w", {"ia", "ua"}}, e = {"-", {"e"}}, r = {"r", {"uan", "er"}}, t = {"t", {"ve", "ue"}}, y = {"y", {"uai", "ing"}}, u = {"sh", {"u"}}, i = {"ch", {"i"}}, o = {"", {"o", "uo"}}, p = {"p", {"un"}},
	a = {"-", {"a"}}, s = {"s", {"ong", "iong"}}, d = {"d", {"uang", "iang"}}, f = {"f", {"en"}}, g = {"g", {"eng"}}, h = {"h", {"ang"}}, j = {"j", {"an"}}, k = {"k", {"ao"}}, l = {"l", {"ai"}},
	z = {"z", {"ei"}}, x = {"x", {"ie"}}, c = {"c", {"iao"}}, v = {"zh", {"ui", "v"}}, b = {"b", {"ou"}}, n = {"n", {"in"}}, m = {"m", {"ian"}},
} == 0)

-- 全拼分隔方案
ime.full_pinyin_adjustments = {
	["ang ang"] ="an'gang", ["ang e"] ="an'ge", ["ban a"] ="ba'na", 
	["bang e"] ="ban'ge", ["beng ai"] ="ben'gai", ["bin an"] ="bi'nan", ["chan a"] ="cha'na", 
	["chen ei"] ="che'nei", ["cheng ang"] ="chen'gang", ["chuang an"] ="chuan'gan", ["chuang ei"] ="chuan'gei", ["chun a"] ="chu'na", ["chun an"] ="chu'nan", ["dan ai"] ="da'nai", 
	["dan ao"] ="da'nao", ["dan eng"] ="da'neng", ["dang ai"] ="dan'gai", ["dang ang"] ="dan'gang", ["dang ao"] ="dan'gao", ["dang e"] ="dan'ge", ["dun ang"] ="du'nang", ["er an"] ="e'ran", 
	["er en"] ="e'ren", ["fang ao"] ="fan'gao", 
	["feng e"] ="fen'ge", ["feng ei"] ="fen'gei", ["gang a"] ="gan'ga", 
	["gang an"] ="gan'gan", ["gua o"] ="gu'ao", ["guang ai"] ="guan'gai", ["hang ai"] ="han'gai", 
	["hen an"] ="he'nan", ["hen ei"] ="he'nei", ["hen eng"] ="he'neng", ["heng ao"] ="hen'gao", ["huan a"] ="hua'na", ["huan an"] ="hua'nan", ["huan eng"] ="hua'neng", ["huang e"] ="huan'ge", ["huang ei"] ="huan'gei", ["hun ao"] ="hu'nao", ["jian a"] ="jia'na", 
	["jian eng"] ="jia'neng", ["jiang e"] ="jian'ge", ["jiang ou"] ="jian'gou", ["jin an"] ="ji'nan", ["jin eng"] ="ji'neng", ["jing ang"] ="jin'gang", ["jing en"] ="jin'gen", ["kang e"] ="kan'ge", 
	["ken a"] ="ke'na", ["ken an"] ="ke'nan", ["ken eng"] ="ke'neng", ["kun an"] ="ku'nan", ["kun ao"] ="ku'nao", ["lang an"] ="lan'gan", 
	["liang ang"] ="lian'gang", ["liang e"] ="lian'ge", ["lin a"] ="li'na", ["ling ang"] ="lin'gang", ["lun ei"] ="lu'nei", ["lun eng"] ="lu'neng", ["man ao"] ="ma'nao", 
	["nan a"] ="na'na", 
	["nan e"] ="na'ne", ["nan eng"] ="na'neng", ["nang ao"] ="nan'gao", ["niang ao"] ="nian'gao", ["nin an"] ="ni'nan", ["nin eng"] ="ni'neng", ["pang ang"] ="pan'gang", 
	["ping e"] ="pin'ge", ["qin ang"] ="qi'nang", 
	["qin ei"] ="qi'nei", ["qin eng"] ="qi'neng", ["qun a"] ="qu'na", ["qun ei"] ="qu'nei", ["ren ao"] ="re'nao", 
	["ren eng"] ="re'neng", ["reng an"] ="ren'gan", ["reng e"] ="ren'ge", ["reng ou"] ="ren'gou", ["run ei"] ="ru'nei", ["run eng"] ="ru'neng", ["sang e"] ="san'ge", 
	["sang en"] ="san'gen", ["sang eng"] ="san'geng", ["shang ao"] ="shan'gao", ["shang e"] ="shan'ge", ["shang ou"] ="shan'gou", ["sheng an"] ="shen'gan", ["sheng ang"] ="shen'gang", ["sheng ao"] ="shen'gao", ["sheng ou"] ="shen'gou", ["sun an"] ="su'nan", ["tang e"] ="tan'ge", 
	["wang e"] ="wan'ge", 
	["weng ao"] ="wen'gao", ["weng e"] ="wen'ge", ["xiang ei"] ="xian'gei", 
	["xin a"] ="xi'na", ["xin ao"] ="xi'nao", ["xin en"] ="xi'nen", ["xing an"] ="xin'gan", ["xing ang"] ="xin'gang", ["xing ao"] ="xin'gao", ["xing e"] ="xin'ge", ["yang ai"] ="yan'gai", 
	["yang e"] ="yan'ge", ["yin ei"] ="yi'nei", ["yin eng"] ="yi'neng", ["yun an"] ="yu'nan", ["zang e"] ="zan'ge", 
	["zen an"] ="ze'nan", ["zen eng"] ="ze'neng", ["zhang ang"] ="zhan'gang", ["zhang e"] ="zhan'ge", ["zhen e"] ="zhe'ne", ["zheng e"] ="zhen'ge", ["zheng ou"] ="zhen'gou", ["zhuang ao"] ="zhuan'gao", ["zhuang ei"] ="zhuan'gei", ["zun ao"] ="zu'nao", 
}
-- 如果存在，使用用户的请求脚本
local user_fetcher = ime.USERCACHEDIR..'/fetcher'
local file = io.open(user_fetcher, 'r')
if file then file:close() ime.fetcher_path = user_fetcher end

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

-- 更新用户 fetcher 脚本
if not do_not_update_fetcher then
	os.execute("mkdir '"..ime.USERCACHEDIR.."' -p")
	http.TIMEOUT = 5
	local ret, c = http.request('http://ibus-sogoupycc.googlecode.com/svn/trunk/linux/fetcher')
	if c == 200 and ret and ret:match('ibus%-sogoupycc%-fetcher%-end') then
		local fetcher_file = io.open(user_fetcher, 'w')
		fetcher_file:write(ret)
		fetcher_file:close()
		os.execute("chmod +x '"..user_fetcher.."'")
		ime.fetcher_path = user_fetcher
	end
end

-- 更新和加载远程脚本
if not do_not_load_remote_script then
	http.TIMEOUT = 5
	os.execute("mkdir '"..ime.USERCACHEDIR.."' -p")
	local autoload_file_path = ime.USERCACHEDIR..'/autoload.lua'
	local ret, c = http.request('http://ibus-sogoupycc.googlecode.com/svn/trunk/startup/autoload.lua')
	if c == 200 and ret and ret:match('ibus%-sogoupycc%-autoload%-end') then
		local autoload_file = io.open(autoload_file_path, 'w')
		autoload_file:write(ret)
		autoload_file:close()
	end
	local file = io.open(autoload_file_path, 'r')
	if file then file:close() dofile(autoload_file_path) end
end
