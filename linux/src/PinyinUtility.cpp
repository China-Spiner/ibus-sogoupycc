/* 
 * File:   PinyinUtility.cpp
 * Author: WU Jun <quark@lihdd.net>
 * 
 * see .h file for changelog
 */

#include <cassert>
#include "PinyinUtility.h"

const set<string> PinyinUtility::validPinyins = setInitializer<string>("ba")("bo")("bai")("bei")("bao")("ban")("ben")("bang")("beng")("bi")("bie")("biao")("bian")("bin")("bing")("bu")("ci")("ca")("ce")("cai")("cao")("cou")("can")("cen")("cang")("ceng")("cu")("cuo")("cui")("cuan")("cun")("cong")("chi")("cha")("che")("chai")("chao")("chou")("chan")("chen")("chang")("cheng")("chu")("chuo")("chuai")("chui")("chuan")("chuang")("chun")("chong")("da")("de")("dai")("dao")("dou")("dan")("dang")("deng")("di")("die")("diao")("diu")("dian")("ding")("du")("duo")("dui")("duan")("dun")("dong")("fa")("fo")("fei")("fou")("fan")("fen")("fang")("feng")("fu")("ga")("ge")("gai")("gei")("gao")("gou")("gan")("gen")("gang")("geng")("gu")("gua")("guo")("guai")("gui")("guan")("gun")("guang")("gong")("ha")("he")("hai")("hei")("hao")("hou")("han")("hen")("hang")("heng")("hu")("hua")("huo")("huai")("hui")("huan")("hun")("huang")("hong")("ji")("jia")("jie")("jiao")("jiu")("jian")("jin")("jing")("jiang")("ju")("jue")("juan")("jun")("jiong")("ka")("ke")("kai")("kao")("kou")("kan")("ken")("kang")("keng")("ku")("kua")("kuo")("kuai")("kui")("kuan")("kun")("kuang")("kong")("la")("le")("lai")("lei")("lao")("lan")("lang")("leng")("li")("ji")("lie")("liao")("liu")("lian")("lin")("liang")("ling")("lou")("lu")("luo")("luan")("lun")("long")("lv")("lue")("ma")("mo")("me")("mai")("mei")("mao")("mou")("man")("men")("mang")("meng")("mi")("mie")("miao")("miu")("mian")("min")("ming")("mu")("na")("ne")("nai")("nei")("nao")("nou")("nan")("nen")("nang")("neng")("ni")("nie")("niao")("niu")("nian")("nin")("niang")("ning")("nu")("nuo")("nuan")("nong")("nv")("nue")("pa")("po")("pai")("pei")("pao")("pou")("pan")("pen")("pang")("peng")("pi")("pie")("piao")("pian")("pin")("ping")("pu")("qi")("qia")("qie")("qiao")("qiu")("qian")("qin")("qiang")("qing")("qu")("que")("quan")("qun")("qiong")("ri")("re")("rao")("rou")("ran")("ren")("rang")("reng")("ru")("ruo")("rui")("ruan")("run")("rong")("si")("sa")("se")("sai")("san")("sao")("sou")("sen")("sang")("seng")("su")("suo")("sui")("suan")("sun")("song")("shi")("sha")("she")("shai")("shei")("shao")("shou")("shan")("shen")("shang")("sheng")("shu")("shua")("shuo")("shuai")("shui")("shuan")("shun")("shuang")("ta")("te")("tai")("tao")("tou")("tan")("tang")("teng")("ti")("tie")("tiao")("tian")("ting")("tu")("tuan")("tuo")("tui")("tun")("tong")("wu")("wa")("wo")("wai")("wei")("wan")("wen")("wang")("weng")("xi")("xia")("xie")("xiao")("xiu")("xian")("xin")("xiang")("xing")("xu")("xue")("xuan")("xun")("xiong")("yi")("ya")("yo")("ye")("yai")("yao")("you")("yan")("yin")("yang")("ying")("yu")("yue")("yuan")("yun")("yong")("yu")("yue")("yuan")("yun")("yong")("zi")("za")("ze")("zai")("zao")("zei")("zou")("zan")("zen")("zang")("zeng")("zu")("zuo")("zui")("zun")("zuan")("zong")("zhi")("zha")("zhe")("zhai")("zhao")("zhou")("zhan")("zhen")("zhang")("zheng")("zhu")("zhua")("zhuo")("zhuai")("zhuang")("zhui")("zhuan")("zhun")("zhong")("a")("e")("ei")("ai")("ei")("ao")("o")("ou")("an")("en")("ang")("eng")("er")();
map<string, string> PinyinUtility::gb2312pinyinMap;
multimap<string, map<string, string>::iterator> PinyinUtility::gb2312characterMap;

PinyinUtility::PinyinUtility() {
}

PinyinUtility::PinyinUtility(const PinyinUtility& orig) {
}

PinyinUtility::~PinyinUtility() {
}

bool PinyinUtility::isValidPinyin(const string& pinyin) {
    return validPinyins.count(pinyin) > 0;
}

void PinyinUtility::staticInitializer() {
    static bool firstRun = true;
    if (firstRun) {
        gb2312pinyinMap.clear();
        // hardcoded gb2312 pinyin map
#define GB2312_ENTRY(pinyin, characters) gb2312pinyinMap[pinyin] = characters;
#include "gb2312List.txt"
#undef GB2312_ENTRY
        // build reverse-map, character to pinyin
        gb2312characterMap.clear();
        for (map<string, string>::iterator pinyinPair = gb2312pinyinMap.begin(); pinyinPair != gb2312pinyinMap.end(); ++pinyinPair) {
            string& characters = pinyinPair->second;
            for (size_t pos = 0; pos < characters.length(); pos += 3) {
                gb2312characterMap.insert(pair<string, map<string, string>::iterator > (characters.substr(pos, 3), pinyinPair));
            }
        }
        firstRun = false;
    }
}

string PinyinUtility::getCandidates(const string& pinyin, int tone) {
    staticInitializer();

    assert(tone <= 5 && tone > 0);
    static const string tones[6] = {"", "1", "2", "3", "4", "5"};
    map<string, string>::iterator it = gb2312pinyinMap.find(pinyin + tones[tone]);
    if (it != gb2312pinyinMap.end()) {
        return it->second;
    } else return "";
}

string PinyinUtility::charactersToPinyins(const string& characters, bool includeTone) {
    staticInitializer();

    string pinyins = "";

    for (size_t pos = 0; pos < characters.length();) {
        // each chinese character takes 3 bytes using UTF-8
        string character = characters.substr(pos, 3);
        multimap<string, map<string, string>::iterator >::iterator pinyinPair = gb2312characterMap.find(character);
        if (pinyinPair != gb2312characterMap.end()) {
            if (pinyins.length() > 0) pinyins += " ";
            if (includeTone) pinyins += pinyinPair->second->first;
            else pinyins += pinyinPair->second->first.substr(0, pinyinPair->second->first.length() - 1);
            pos += 3;
        } else {
            // unregonised, only take 1 character
            if (pinyins.length() > 0) pinyins += " ";
            pinyins += characters[pos];
            pos++;
        }
    }
    return pinyins;
}

// DoublePinyinScheme

DoublePinyinScheme::DoublePinyinScheme() {
    mapBuilt = false;
}

DoublePinyinScheme::DoublePinyinScheme(const DoublePinyinScheme& orig) {
    bindedKeys = orig.bindedKeys;
    buildMap();
}

DoublePinyinScheme::~DoublePinyinScheme() {

}

void DoublePinyinScheme::bindKey(const char key, const string& consonant, const vector<string>& vowels) {
    bindedKeys[key] = pair<string, vector<string> >(consonant, vowels);
    mapBuilt = false;
}

void DoublePinyinScheme::clear() {
    bindedKeys.clear();
    mapBuilt = false;
}

string DoublePinyinScheme::query(const char firstKey, const char secondKey) {
    if (!mapBuilt) buildMap();
    map<pair<char, char>, string>::const_iterator it =
            queryMap.find(pair<char, char>(firstKey, secondKey));

    if (it != queryMap.end()) return it->second;
    else return "";
}

string DoublePinyinScheme::query(const string& doublePinyinString) {
    string result = "";
    char keys[2];
    int p = 0;
    for (size_t i = 0; i < doublePinyinString.length(); ++i) {
        char key = doublePinyinString[i];
        if (bindedKeys.count(key) > 0) {
            keys[p] = key;
            p ^= 1;
            if (p == 0) {
                if (result.length() == 0)
                    result += query(keys[0], keys[1]);
                else
                    result += " " + query(keys[0], keys[1]);
            }
        } else {
            result += key;
        }
    }
    if (p == 1) {
        if (result.length() > 0) result += " ";
        string consonant = bindedKeys[keys[0]].first;
        if (consonant.length() == 0)
            result += "'";
        else result += consonant;
    }
    return result;
}

bool DoublePinyinScheme::isValidDoublePinyin(const string& doublePinyinString) {
    char keys[2];
    int p = 0;
    for (size_t i = 0; i < doublePinyinString.length(); ++i) {
        keys[p++] = doublePinyinString[i];
        if (p == 2) {
            if (query(keys[0], keys[1]).length() == 0) return false;
            p = 0;
        }
    }
    if (p == 1) {
        return bindedKeys[keys[0]].first != "-";
    }
    return true;
}

int DoublePinyinScheme::buildMap() {
    queryMap.clear();
    int conflictCount = 0;
    for (map<char, pair<string, vector<string> > >::iterator firstKey = bindedKeys.begin(); firstKey != bindedKeys.end(); ++firstKey) {
        for (map<char, pair<string, vector<string> > >::iterator secondKey = bindedKeys.begin(); secondKey != bindedKeys.end(); ++secondKey) {
            string& consonant = firstKey->second.first;
            for (vector<string>::iterator vowel = secondKey->second.second.begin(); vowel != secondKey->second.second.end(); ++vowel) {
                string pinyin = consonant + *vowel;
                if (PinyinUtility::isValidPinyin(pinyin)) {
                    if (queryMap.count(pair<char, char>(firstKey->first, secondKey->first)) == 0) {
                        // ok, let's set it.
                        queryMap[pair<char, char>(firstKey->first, secondKey->first)] = pinyin;
                    } else {
                        fprintf(stderr, "DoublePinyinScheme::buildMap: conflict found: %s %s\n", queryMap[pair<char, char>(firstKey->first, secondKey->first)].c_str(), pinyin.c_str());
                        ++conflictCount;
                    }
                }
            }
        }
    }
    mapBuilt = true;
    return conflictCount;
}

