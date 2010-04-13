/* 
 * File:   PinyinUtility.cpp
 * Author: WU Jun <quark@lihdd.net>
 */

#include <cstdio>
#include <cassert>
#include "PinyinUtility.h"
#include "defines.h"
#include "Configuration.h"

// no "ve" here in validPinyins, all "ue"
const set<string> PinyinUtility::validPinyins = setInitializer<string>("ba")("bo")("bai")("bei")("bao")("ban")("ben")("bang")("beng")("bi")("bie")("biao")("bian")("bin")("bing")("bu")("ci")("ca")("ce")("cai")("cao")("cou")("can")("cen")("cang")("ceng")("cu")("cuo")("cui")("cuan")("cun")("cong")("chi")("cha")("che")("chai")("chao")("chou")("chan")("chen")("chang")("cheng")("chu")("chuo")("chuai")("chui")("chuan")("chuang")("chun")("chong")("da")("de")("dei")("dai")("dao")("dou")("dan")("dang")("deng")("di")("die")("diao")("diu")("dian")("ding")("du")("duo")("dui")("duan")("dun")("dong")("fa")("fo")("fei")("fou")("fan")("fen")("fang")("feng")("fu")("ga")("ge")("gai")("gei")("gao")("gou")("gan")("gen")("gang")("geng")("gu")("gua")("guo")("guai")("gui")("guan")("gun")("guang")("gong")("ha")("he")("hai")("hei")("hao")("hou")("han")("hen")("hang")("heng")("hu")("hua")("huo")("huai")("hui")("huan")("hun")("huang")("hong")("ji")("jia")("jie")("jiao")("jiu")("jian")("jin")("jing")("jiang")("ju")("jue")("juan")("jun")("jiong")("ka")("ke")("kai")("kao")("kou")("kan")("ken")("kang")("keng")("ku")("kua")("kuo")("kuai")("kui")("kuan")("kun")("kuang")("kong")("la")("le")("lai")("lei")("lao")("lan")("lang")("leng")("li")("ji")("lie")("liao")("liu")("lian")("lin")("liang")("ling")("lou")("lu")("luo")("luan")("lun")("long")("lv")("lue")("ma")("mo")("me")("mai")("mei")("mao")("mou")("man")("men")("mang")("meng")("mi")("mie")("miao")("miu")("mian")("min")("ming")("mu")("na")("ne")("nai")("nei")("nao")("nou")("nan")("nen")("nang")("neng")("ni")("nie")("niao")("niu")("nian")("nin")("niang")("ning")("nu")("nuo")("nuan")("nong")("nv")("nue")("pa")("po")("pai")("pei")("pao")("pou")("pan")("pen")("pang")("peng")("pi")("pie")("piao")("pian")("pin")("ping")("pu")("qi")("qia")("qie")("qiao")("qiu")("qian")("qin")("qiang")("qing")("qu")("que")("quan")("qun")("qiong")("ri")("re")("rao")("rou")("ran")("ren")("rang")("reng")("ru")("ruo")("rui")("ruan")("run")("rong")("si")("sa")("se")("sai")("san")("sao")("sou")("sen")("sang")("seng")("su")("suo")("sui")("suan")("sun")("song")("shi")("sha")("she")("shai")("shei")("shao")("shou")("shan")("shen")("shang")("sheng")("shu")("shua")("shuo")("shuai")("shui")("shuan")("shun")("shuang")("ta")("te")("tai")("tao")("tou")("tan")("tang")("teng")("ti")("tie")("tiao")("tian")("ting")("tu")("tuan")("tuo")("tui")("tun")("tong")("wu")("wa")("wo")("wai")("wei")("wan")("wen")("wang")("weng")("xi")("xia")("xie")("xiao")("xiu")("xian")("xin")("xiang")("xing")("xu")("xue")("xuan")("xun")("xiong")("yi")("ya")("yo")("ye")("yai")("yao")("you")("yan")("yin")("yang")("ying")("yu")("yue")("yuan")("yun")("yong")("yu")("yue")("yuan")("yun")("yong")("zi")("za")("ze")("zai")("zao")("zei")("zou")("zan")("zen")("zang")("zeng")("zu")("zuo")("zui")("zun")("zuan")("zong")("zhi")("zha")("zhe")("zhai")("zhao")("zhou")("zhan")("zhen")("zhang")("zheng")("zhu")("zhua")("zhuo")("zhuai")("zhuang")("zhui")("zhuan")("zhun")("zhong")("a")("e")("ei")("ai")("ei")("ao")("o")("ou")("an")("en")("ang")("eng")("er")();

map<string, string> PinyinUtility::gb2312pinyinMap;
multimap<string, map<string, string>::iterator> PinyinUtility::gb2312characterMap;
set<string> PinyinUtility::validPartialPinyins;
const int PinyinUtility::VALID_PINYIN_MAX_LENGTH = sizeof ("chuang") - 1;

PinyinUtility::PinyinUtility() {
}

PinyinUtility::PinyinUtility(const PinyinUtility& orig) {
}

PinyinUtility::~PinyinUtility() {
}

const string PinyinUtility::separatePinyins(const string& pinyins) {
    string r;
    string unparsedPinyins = pinyins;

    // greedy failure case: "leni", greedy: "len i", should be: "le ni"
    // so try to bring back the last one

    // match unparsedPinyins as long as possible in partialPinyin
    while (!unparsedPinyins.empty()) {
        if (r.length() > 0 && r.data()[r.length() - 1] != ' ' && r.data()[r.length() - 1] != '\'') r += ' ';
        bool breaked = false;
        for (int i = VALID_PINYIN_MAX_LENGTH; i > 0; --i) {
            if (validPartialPinyins.find(unparsedPinyins.substr(0, i)) != validPartialPinyins.end()) {
                // consider a futher step, next one ?
                // this should help to avoid some greedy failure case
                string nextChar;
                if (unparsedPinyins.length() > (size_t) i) nextChar = unparsedPinyins.substr(i, 1);
                if (nextChar.empty() || nextChar == "'" || nextChar == " " || isValidPartialPinyin(nextChar)) {
                    // confirm separate
                    r += unparsedPinyins.substr(0, i);
                    // use Configuration::getFullPinyinTailAdjusted to adjust full pinyins
                    r = Configuration::getFullPinyinTailAdjusted(r);
                    if (nextChar == "'") r += "'";
                    unparsedPinyins.erase(0, i);
                    breaked = true;
                    break;
                }
            }
        }
        if (breaked) continue;

        // cannot parse anyway, just eat one char:
        // and ignore space
        if (unparsedPinyins[0] != ' ' && unparsedPinyins[0] != '\'') r += unparsedPinyins[0];
        unparsedPinyins.erase(0, 1);
    }
    // replace "'" to " "
    for (string::iterator it = r.begin(); it != r.end(); ++it) {
        if (*it == '\'') *it = ' ';
    }
    return r;
}

const bool PinyinUtility::isRecognisedCharacter(const string& character) {
    return gb2312characterMap.find(character) != gb2312characterMap.end();
}

const bool PinyinUtility::isCharactersPinyinsMatch(const string& characters, const string& pinyins) {
    PinyinSequence ps = pinyins;
    // IMPROVE: handle utf-8 using glib
    if (ps.size() != (size_t) g_utf8_strlen(characters.c_str(), -1)) return false;
    for (size_t i = 0; i < ps.size(); i++) {
        if (!isCharacterPinyinMatch(characters.substr(i * 3, 3), ps[i])) {
            return false;
        }
    }
    return true;
}

const bool PinyinUtility::isCharacterPinyinMatch(const string& character, const string& pinyin) {
    pair<multimap<string, map<string, string>::iterator>::iterator, multimap<string, map<string, string>::iterator>::iterator> range = gb2312characterMap.equal_range(character);
    for (multimap<string, map<string, string>::iterator>::iterator it = range.first; it != range.second; ++it) {
        if (it->second->first.substr(0, pinyin.length()) == pinyin) {
            DEBUG_PRINT(8, "[UTIL] isMatch: '%s' => '%s': true(%s)\n", pinyin.c_str(), character.c_str(), it->second->first.c_str());
            return true;
        }
    }
    DEBUG_PRINT(8, "[UTIL] isMatch: '%s' => '%s': false\n", pinyin.c_str(), character.c_str());
    return false;
}

const bool PinyinUtility::isValidPinyin(const string& pinyin) {
    return validPinyins.count(pinyin) > 0;
}

const bool PinyinUtility::isValidPartialPinyin(const string& pinyin) {
    return validPartialPinyins.count(pinyin) > 0;
}

void PinyinUtility::staticDestruct() {

}

void PinyinUtility::staticInit() {
    gb2312pinyinMap.clear();
    // hardcoded gb2312 pinyin map
#define GB2312_ENTRY(pinyin, characters) gb2312pinyinMap[pinyin] = characters;
#include "gb2312List.txt"
#undef GB2312_ENTRY
    // build reverse-map, character to pinyin
    // gb2312characterMap[chinese_character] : map<pinyin, character>::iterator
    gb2312characterMap.clear();

    // IMPROVE: use g_utf8_... method to travel characters
    for (map<string, string>::iterator pinyinPair = gb2312pinyinMap.begin(); pinyinPair != gb2312pinyinMap.end(); ++pinyinPair) {
        string& characters = pinyinPair->second;
        for (size_t pos = 0; pos < characters.length(); pos += 3) {
            gb2312characterMap.insert(pair<string, map<string, string>::iterator > (characters.substr(pos, 3), pinyinPair));
        }
    }

    validPartialPinyins.clear();
    for (set<string>::const_iterator it = validPinyins.begin(); it != validPinyins.end(); ++it) {
        string pinyin = *it, partialPinyin;
        for (const char* p = pinyin.c_str(); *p; ++p) {
            partialPinyin += *p;
            validPartialPinyins.insert(partialPinyin);
        }
    }
}

const string PinyinUtility::getCandidates(const string& pinyin, int tone) {
    assert(tone <= 5 && tone > 0);
    static const string tones[6] = {"", "1", "2", "3", "4", "5"};
    map<string, string>::const_iterator it = gb2312pinyinMap.find(pinyin + tones[tone]);
    if (it != gb2312pinyinMap.end()) {
        return it->second;
    } else return "";
}

const string PinyinUtility::charactersToPinyins(const string& characters, size_t index, bool includeTone) {
    PinyinSequence ps = characters;

    size_t id = index;
    string r;

    for (size_t i = 0; i < ps.size(); ++i) {
        DEBUG_PRINT(7, "[UTIL] ps[%d] = '%s'\n", i, ps[i].c_str());
        if (isRecognisedCharacter(ps[i])) {
            // no additional space, should be converted
            DEBUG_PRINT(7, "[UTIL]  found chs char: '%s'\n", ps[i].c_str());
            typedef multimap<string, map<string, string>::iterator >::iterator cmIterator;
            size_t rangeSize = gb2312characterMap.count(ps[i]), j = 0;
            if (rangeSize > 0) {
                // found, convert it to pinyin
                pair<cmIterator, cmIterator> range = gb2312characterMap.equal_range(ps[i]);
                for (cmIterator it = range.first; it != range.second; ++it) {
                    if (j++ == id % rangeSize) {
                        // pick up this
                        string pinyin = it->second->first;
                        DEBUG_PRINT(7, "[UTIL]  pick up pinyin: '%s'\n", pinyin.c_str());
                        if (!includeTone) pinyin.erase(pinyin.length() - 1, 1);
                        r += pinyin + " ";
                        break; // break for
                    }
                }
                id /= rangeSize;
            } else {
                r += ps[i];
            }
        } else {
            DEBUG_PRINT(7, "[UTIL]  found normal char: '%s'\n", ps[i].c_str());
            r += ps[i] + " ";
        }
    }
    // remove last space
    if (r.c_str()[r.length() - 1] == ' ') r.erase(r.length() - 1, 1);
    if ((index > 0 && id > 0) || index >= Configuration::multiToneLimit) {
        // index overflow
        return "";
    } else {
        return r;
    }
}

// Lua C Functions

/**
 * in: string
 * out: boolean
 */
static int l_isValidPinyin(lua_State *L) {
    DEBUG_PRINT(2, "[LUA] l_isValidPinyin\n");
    LuaBinding& lib = LuaBinding::getLuaBinding(L);
    pthread_mutex_lock(lib.getAtomMutex());
    luaL_checktype(L, 1, LUA_TSTRING);
    lua_checkstack(L, 1);
    lua_pushboolean(L, PinyinUtility::isValidPinyin(lua_tostring(L, 1)));
    pthread_mutex_unlock(lib.getAtomMutex());
    return 1;
}

/**
 * in: string
 * out: string
 */
static int l_charsToPinyin(lua_State *L) {
    DEBUG_PRINT(2, "[LUA] l_charsToPinyin\n");
    LuaBinding& lib = LuaBinding::getLuaBinding(L);
    pthread_mutex_lock(lib.getAtomMutex());
    luaL_checktype(L, 1, LUA_TSTRING);
    lua_checkstack(L, 1);
    lua_pushstring(L, PinyinUtility::charactersToPinyins(lua_tostring(L, 1), 0).c_str());
    pthread_mutex_unlock(lib.getAtomMutex());
    return 1;
}

void PinyinUtility::registerLuaFunctions() {
    LuaBinding::getStaticBinding().registerFunction(l_charsToPinyin, "chars_to_pinyin");
    LuaBinding::getStaticBinding().registerFunction(l_isValidPinyin, "is_valid_pinyin");
}

