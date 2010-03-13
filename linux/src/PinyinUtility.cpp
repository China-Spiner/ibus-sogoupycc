/* 
 * File:   PinyinUtility.cpp
 * Author: WU Jun <quark@lihdd.net>
 */

#include <cassert>
#include "PinyinUtility.h"

// no "ve" here in validPinyins, all "ue"
const set<string> PinyinUtility::validPinyins = setInitializer<string>("ba")("bo")("bai")("bei")("bao")("ban")("ben")("bang")("beng")("bi")("bie")("biao")("bian")("bin")("bing")("bu")("ci")("ca")("ce")("cai")("cao")("cou")("can")("cen")("cang")("ceng")("cu")("cuo")("cui")("cuan")("cun")("cong")("chi")("cha")("che")("chai")("chao")("chou")("chan")("chen")("chang")("cheng")("chu")("chuo")("chuai")("chui")("chuan")("chuang")("chun")("chong")("da")("de")("dai")("dao")("dou")("dan")("dang")("deng")("di")("die")("diao")("diu")("dian")("ding")("du")("duo")("dui")("duan")("dun")("dong")("fa")("fo")("fei")("fou")("fan")("fen")("fang")("feng")("fu")("ga")("ge")("gai")("gei")("gao")("gou")("gan")("gen")("gang")("geng")("gu")("gua")("guo")("guai")("gui")("guan")("gun")("guang")("gong")("ha")("he")("hai")("hei")("hao")("hou")("han")("hen")("hang")("heng")("hu")("hua")("huo")("huai")("hui")("huan")("hun")("huang")("hong")("ji")("jia")("jie")("jiao")("jiu")("jian")("jin")("jing")("jiang")("ju")("jue")("juan")("jun")("jiong")("ka")("ke")("kai")("kao")("kou")("kan")("ken")("kang")("keng")("ku")("kua")("kuo")("kuai")("kui")("kuan")("kun")("kuang")("kong")("la")("le")("lai")("lei")("lao")("lan")("lang")("leng")("li")("ji")("lie")("liao")("liu")("lian")("lin")("liang")("ling")("lou")("lu")("luo")("luan")("lun")("long")("lv")("lue")("ma")("mo")("me")("mai")("mei")("mao")("mou")("man")("men")("mang")("meng")("mi")("mie")("miao")("miu")("mian")("min")("ming")("mu")("na")("ne")("nai")("nei")("nao")("nou")("nan")("nen")("nang")("neng")("ni")("nie")("niao")("niu")("nian")("nin")("niang")("ning")("nu")("nuo")("nuan")("nong")("nv")("nue")("pa")("po")("pai")("pei")("pao")("pou")("pan")("pen")("pang")("peng")("pi")("pie")("piao")("pian")("pin")("ping")("pu")("qi")("qia")("qie")("qiao")("qiu")("qian")("qin")("qiang")("qing")("qu")("que")("quan")("qun")("qiong")("ri")("re")("rao")("rou")("ran")("ren")("rang")("reng")("ru")("ruo")("rui")("ruan")("run")("rong")("si")("sa")("se")("sai")("san")("sao")("sou")("sen")("sang")("seng")("su")("suo")("sui")("suan")("sun")("song")("shi")("sha")("she")("shai")("shei")("shao")("shou")("shan")("shen")("shang")("sheng")("shu")("shua")("shuo")("shuai")("shui")("shuan")("shun")("shuang")("ta")("te")("tai")("tao")("tou")("tan")("tang")("teng")("ti")("tie")("tiao")("tian")("ting")("tu")("tuan")("tuo")("tui")("tun")("tong")("wu")("wa")("wo")("wai")("wei")("wan")("wen")("wang")("weng")("xi")("xia")("xie")("xiao")("xiu")("xian")("xin")("xiang")("xing")("xu")("xue")("xuan")("xun")("xiong")("yi")("ya")("yo")("ye")("yai")("yao")("you")("yan")("yin")("yang")("ying")("yu")("yue")("yuan")("yun")("yong")("yu")("yue")("yuan")("yun")("yong")("zi")("za")("ze")("zai")("zao")("zei")("zou")("zan")("zen")("zang")("zeng")("zu")("zuo")("zui")("zun")("zuan")("zong")("zhi")("zha")("zhe")("zhai")("zhao")("zhou")("zhan")("zhen")("zhang")("zheng")("zhu")("zhua")("zhuo")("zhuai")("zhuang")("zhui")("zhuan")("zhun")("zhong")("a")("e")("ei")("ai")("ei")("ao")("o")("ou")("an")("en")("ang")("eng")("er")();

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
    staticInitializer();

    string r;
    string unparsedPinyins = pinyins;

    // greedy failure case: "leni", greedy: "len i", should be: "le ni"
    // so try to bring back the last one

    // match unparsedPinyins as long as possible in partialPinyin
    while (!unparsedPinyins.empty()) {
        if (r.length() > 0 && r.data()[r.length() - 1] != ' ') r += ' ';
        bool breaked = false;
        for (int i = VALID_PINYIN_MAX_LENGTH; i > 0; --i) {
            if (validPartialPinyins.find(unparsedPinyins.substr(0, i)) != validPartialPinyins.end()) {
                // consider a futher step, next one ?
                // this should help to avoid some greedy failure case
                string nextOne;
                if (unparsedPinyins.length() > (size_t)i) nextOne = unparsedPinyins.substr(i, 1);
                if (nextOne.empty() || nextOne == "'" || nextOne == " " || isValidPartialPinyin(nextOne)) {
                    r += unparsedPinyins.substr(0, i);
                    unparsedPinyins.erase(0, i);
                    breaked = true;
                    break;
                }
            }
        }
        if (breaked) continue;

        // cannot parse anyway, just eat one char:
        // and ignore space
        if (unparsedPinyins[0] != ' ') r += unparsedPinyins[0];
        unparsedPinyins.erase(0, 1);
    }
    return r;
}

const bool PinyinUtility::isValidPinyin(const string& pinyin) {
    return validPinyins.count(pinyin) > 0;
}

const bool PinyinUtility::isValidPartialPinyin(const string& pinyin) {
    staticInitializer();

    return validPartialPinyins.count(pinyin) > 0;
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

        validPartialPinyins.clear();
        for (set<string>::const_iterator it = validPinyins.begin(); it != validPinyins.end(); ++it) {
            string pinyin = *it, partialPinyin;
            for (const char* p = pinyin.c_str(); *p; ++p) {
                partialPinyin += *p;
                validPartialPinyins.insert(partialPinyin);
            }
        }

        firstRun = false;
    }
}

const string PinyinUtility::getCandidates(const string& pinyin, int tone) {
    staticInitializer();

    assert(tone <= 5 && tone > 0);
    static const string tones[6] = {"", "1", "2", "3", "4", "5"};
    map<string, string>::iterator it = gb2312pinyinMap.find(pinyin + tones[tone]);
    if (it != gb2312pinyinMap.end()) {
        return it->second;
    } else return "";
}

const string PinyinUtility::charactersToPinyins(const string& characters, bool includeTone) {
    staticInitializer();

    string pinyins, unregonised;

    for (size_t pos = 0; pos < characters.length();) {
        // each chinese character takes 3 bytes using UTF-8
        // TODO: use glib to handle utf8 characters
        string character = characters.substr(pos, 3);
        multimap<string, map<string, string>::iterator >::iterator pinyinPair = gb2312characterMap.find(character);
        if (pinyinPair != gb2312characterMap.end()) {
            if (unregonised.length() > 0) {
                // if it is partical pinyin, it will be converted, no additional space
                if (pinyins.length() > 0) pinyins += " ";
                pinyins += unregonised;
                unregonised = "";
            }
            if (pinyins.length() > 0) pinyins += " ";
            if (includeTone) pinyins += pinyinPair->second->first;
            else pinyins += pinyinPair->second->first.substr(0, pinyinPair->second->first.length() - 1);
            pos += 3;
        } else {
            // unregonised, only take 1 character, buffered
            if (characters[pos] == ' ') {
                // add space when necessary
                if (unregonised.length() > 1 && unregonised[unregonised.length() - 1] != ' ') {
                    // if it is partical pinyin, do not add space
                    size_t lastPinyinPos = unregonised.find_last_of(' ', unregonised.length() - 1);

                    if (lastPinyinPos == string::npos) lastPinyinPos = 0;
                    else lastPinyinPos++;

                    string possibleParticalPinyin = unregonised.substr(lastPinyinPos, unregonised.length() - lastPinyinPos);
                    if (!isValidPartialPinyin(possibleParticalPinyin)) unregonised += ' ';
                }
            }
            unregonised += characters[pos];
            pos++;
        }
    }
    if (unregonised.length() > 0) {
        if (pinyins.length() > 0) pinyins += " ";
        if (isValidPartialPinyin(unregonised.substr(0, unregonised.length() - 1)))
            unregonised.erase(unregonised.length() - 1, 1);
        pinyins += unregonised;
        unregonised = "";
    }
    return pinyins;
}
