/*
 * File:   doublepinyin_scheme.c
 * Author: Jun WU <quark@lihdd.com>
 *
 * GNUv2 Licence
 * 
 * Created on November 3, 2009
 */

#define _REENTRANT

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "doublepinyin_scheme.h"

const char valid_pinyins[][sizeof ("zhuang")] = {"ba", "bo", "bai", "bei", "bao", "ban", "ben", "bang", "beng", "bi", "bie", "biao", "bian", "bin", "bing", "bu", "ci", "ca", "ce", "cai", "cao", "cou", "can", "cen", "cang", "ceng", "cu", "cuo", "cui", "cuan", "cun", "cong", "chi", "cha", "che", "chai", "chao", "chou", "chan", "chen", "chang", "cheng", "chu", "chuo", "chuai", "chui", "chuan", "chuang", "chun", "chong", "da", "de", "dai", "dao", "dou", "dan", "dang", "deng", "di", "die", "diao", "diu", "dian", "ding", "du", "duo", "dui", "duan", "dun", "dong", "fa", "fo", "fei", "fou", "fan", "fen", "fang", "feng", "fu", "ga", "ge", "gai", "gei", "gao", "gou", "gan", "gen", "gang", "geng", "gu", "gua", "guo", "guai", "gui", "guan", "gun", "guang", "gong", "ha", "he", "hai", "hei", "hao", "hou", "han", "hen", "hang", "heng", "hu", "hua", "huo", "huai", "hui", "huan", "hun", "huang", "hong", "ji", "jia", "jie", "jiao", "jiu", "jian", "jin", "jing", "jiang", "ju", "jue", "juan", "jun", "jiong", "ka", "ke", "kai", "kao", "kou", "kan", "ken", "kang", "keng", "ku", "kua", "kuo", "kuai", "kui", "kuan", "kun", "kuang", "kong", "la", "le", "lai", "lei", "lao", "lan", "lang", "leng", "li", "ji", "lie", "liao", "liu", "lian", "lin", "liang", "ling", "lou", "lu", "luo", "luan", "lun", "long", "lv", "lue", "ma", "mo", "me", "mai", "mei", "mao", "mou", "man", "men", "mang", "meng", "mi", "mie", "miao", "miu", "mian", "min", "ming", "mu", "na", "ne", "nai", "nei", "nao", "nou", "nan", "nen", "nang", "neng", "ni", "nie", "niao", "niu", "nian", "nin", "niang", "ning", "nu", "nuo", "nuan", "nong", "nv", "nue", "pa", "po", "pai", "pei", "pao", "pou", "pan", "pen", "pang", "peng", "pi", "pie", "piao", "pian", "pin", "ping", "pu", "qi", "qia", "qie", "qiao", "qiu", "qian", "qin", "qiang", "qing", "qu", "que", "quan", "qun", "qiong", "ri", "re", "rao", "rou", "ran", "ren", "rang", "reng", "ru", "ruo", "rui", "ruan", "run", "rong", "si", "sa", "se", "sai", "san", "sao", "sou", "sen", "sang", "seng", "su", "suo", "sui", "suan", "sun", "song", "shi", "sha", "she", "shai", "shei", "shao", "shou", "shan", "shen", "shang", "sheng", "shu", "shua", "shuo", "shuai", "shui", "shuan", "shun", "shuang", "ta", "te", "tai", "tao", "tou", "tan", "tang", "teng", "ti", "tie", "tiao", "tian", "ting", "tu", "tuan", "tuo", "tui", "tun", "tong", "wu", "wa", "wo", "wai", "wei", "wan", "wen", "wang", "weng", "xi", "xia", "xie", "xiao", "xiu", "xian", "xin", "xiang", "xing", "xu", "xue", "xuan", "xun", "xiong", "yi", "ya", "yo", "ye", "yai", "yao", "you", "yan", "yin", "yang", "ying", "yu", "yue", "yuan", "yun", "yong", "yu", "yue", "yuan", "yun", "yong", "zi", "za", "ze", "zai", "zao", "zei", "zou", "zan", "zen", "zang", "zeng", "zu", "zuo", "zui", "zun", "zuan", "zong", "zhi", "zha", "zhe", "zhai", "zhao", "zhou", "zhan", "zhen", "zhang", "zheng", "zhu", "zhua", "zhuo", "zhuai", "zhuang", "zhui", "zhuan", "zhun", "zhong", "a", "e", "ei", "ai", "ei", "ao", "ou", "an", "en", "ang", "eng", "er"};

struct doublepinyin_scheme_t default_doublepinyin_scheme;

int is_valid_pinyin(char *string) {
    int i;
    if (string == NULL) return 0;
    if (string[0] == 0) return 0;
    for (i = 0; i < sizeof (valid_pinyins) / sizeof (valid_pinyins[0]); ++i) {
        if (strcmp(string, valid_pinyins[i]) == 0) return 1;
    }
    return 0;
}

void doublepinyin_scheme_init(struct doublepinyin_scheme_t *scheme) {
    if (!scheme) scheme = &default_doublepinyin_scheme;
    memset(scheme->map_string_count, 0, sizeof (scheme->map_string_count));
    int i;
    for (i = 0; i <= 'z' - 'a'; ++i) {
        scheme->map_string[i] = (char**) malloc(sizeof (char*));
    }
}

/**
 * note: the non-first pair inserted must not be used to compose a pinyin at first
 * and the first pair inserted must be use at first when composing pinyin.
 */
void doublepinyin_scheme_insert_pair(struct doublepinyin_scheme_t *scheme, char key, const char* value) {
    if (!scheme) scheme = &default_doublepinyin_scheme;
    if (key <= 'z' && key >= 'a' && value) {
        scheme->map_string[key - 'a'] =
                realloc(scheme->map_string[key - 'a'], sizeof (char*) * (scheme->map_string_count[key - 'a'] + 1));
        scheme->map_string[key - 'a'][scheme->map_string_count[key - 'a']++] = strdup(value);
    }
}

void doublepinyin_scheme_free(struct doublepinyin_scheme_t *scheme) {
    if (!scheme) scheme = &default_doublepinyin_scheme;
    int i, j;
    for (i = 0; i <= 'z' - 'a'; i++) {
        for (j = 0; j < scheme->map_string_count[i]; j++)
            free(scheme->map_string[i][j]);
        free(scheme->map_string[i]);
    }

    for (i = 0; i <= 'z' - 'a'; i++)
        for (j = 0; j <= 'z' - 'a'; j++)
            if (scheme->trie[i][j]) free(scheme->trie[i][j]);
    memset(scheme->map_string_count, 0, sizeof (scheme->map_string_count));
}

/**
 * @return conflict count
 */
int doublepinyin_scheme_build_trie(struct doublepinyin_scheme_t *scheme) {
    if (!scheme) scheme = &default_doublepinyin_scheme;
    int i, j, k, l, conflict_count = 0;
    for (i = 0; i <= 'z' - 'a'; i++) {
        for (j = 0; j <= 'z' - 'a'; j++) {
            scheme->trie[i][j] = 0;
            for (k = 0; k < scheme->map_string_count[i] && k < 1; k++) {
                for (l = 1; l < scheme->map_string_count[j]; l++) {
                    int pinyin_length = strlen(scheme->map_string[i][k]) + strlen(scheme->map_string[j][l]) + 3;
                    char *pinyin = malloc(pinyin_length);
                    strcpy(pinyin, scheme->map_string[i][k]);
                    strcat(pinyin, scheme->map_string[j][l]);
                    if (is_valid_pinyin(pinyin)) {
                        if (scheme->trie[i][j]) {
                            // conflict found
                            fprintf(stderr, "doublepinyin_scheme_build_trie: conflict found: %s %s\n", scheme->trie[i][j], pinyin);
                            ++conflict_count;
                        } else {
                            scheme->trie[i][j] = strdup(pinyin);
                        }
                    }
                    free(pinyin);
                }
            }
        }
    }
    return conflict_count;
}

char* doublepinyin_scheme_query(struct doublepinyin_scheme_t *scheme, const char first_char, const char second_char) {
    if (!scheme) scheme = &default_doublepinyin_scheme;
    if (first_char > 'z' || first_char < 'a') return 0;
    if (second_char > 'z' || second_char < 'a') return 0;
    return scheme->trie[first_char - 'a'][second_char - 'a'];
}

/**
 * convert doublepinyin to normal pinyin, seprate with space
 * if can not match at some position, mark *err_parse = pos (start at 1)
 * @param curpos cursor position (from 0) in doublepinyin_string
 * @param *retpos translated position in return string, 0 if error occurs
 * remember to free returned char*
 */
char* doublepinyin_scheme_convert(struct doublepinyin_scheme_t *scheme, const char* doublepinyin_string, int curpos, int *retpos, int *err_parse, int *tail) {
    if (!scheme) scheme = &default_doublepinyin_scheme;
    int i, lo = strlen(doublepinyin_string), l = (lo | 1) ^ 1, bi = l, ret_length = 0, have_tail = 0;
    char *ret_string;
    char *buffer;

    if (err_parse) *err_parse = 0;
    if (retpos) *retpos = 0;

    for (i = 0; i < l; i += 2) {
        buffer = doublepinyin_scheme_query(scheme, doublepinyin_string[i], doublepinyin_string[i + 1]);
        
        if (((curpos | 1) == (i | 1)) && retpos) {
            *retpos = ret_length;
            if (curpos == i + 1 && doublepinyin_string[i] >= 'a' && doublepinyin_string[i] <= 'z'
                    && scheme->map_string_count[doublepinyin_string[i] - 'a'] > 0) {
                *retpos += strlen(scheme->map_string[doublepinyin_string[i] - 'a'][0]);
            }
        }

        if (buffer) {
            ret_length += strlen(buffer) + 1;
        } else {
            if (err_parse) *err_parse = i + 1;
            bi = i;
            break;
        }
    }

    if (curpos >= i && retpos) *retpos = ret_length;

    if (bi == lo - 1 && doublepinyin_string[lo - 1] >= 'a' && doublepinyin_string[lo - 1] <= 'z'
            && scheme->map_string_count[doublepinyin_string[lo - 1] - 'a'] > 0) {
        ret_length += strlen(scheme->map_string[doublepinyin_string[lo - 1] - 'a'][0]) + 1;
        have_tail = 1;
        if (curpos >= lo && retpos) *retpos = ret_length;
    }

    if (tail) *tail = have_tail;

    ret_string = malloc(sizeof (char) * ret_length + 2);
    ret_length = 0;
    for (i = 0; i < bi; i += 2) {
        buffer = doublepinyin_scheme_query(scheme, doublepinyin_string[i], doublepinyin_string[i + 1]);
        strcpy(ret_string + ret_length, buffer);
        ret_length += strlen(buffer);
        ret_string[ret_length++] = ' ';
    }

    if (have_tail) {
        strcpy(ret_string + ret_length, scheme->map_string[doublepinyin_string[lo - 1] - 'a'][0]);
        ret_length += strlen(scheme->map_string[doublepinyin_string[lo - 1] - 'a'][0]) + 1;
    }
    if (ret_length <= 0) ret_length = 1;

    ret_string[ret_length - 1] = 0;
    if (retpos && *retpos > ret_length - 1) *retpos = ret_length - 1;

    return ret_string;
}

void init_default_doublepinyin_scheme(void) {
    doublepinyin_scheme_init(&default_doublepinyin_scheme);

    // hardcoded default double pinyin scheme
    // slightly different than MSPY, should be same as UCDOS 6.0 if I remember clearly
    // differece from MSPY: remove ';', 'y' - 'v', 'v' - 've', add 'v' - 'v', 'y' - 'ing'

#define doublepinyin_scheme_insert_default_3pairs(k, va, vb, vc) \
    doublepinyin_scheme_insert_pair(&default_doublepinyin_scheme, k, va); \
    doublepinyin_scheme_insert_pair(&default_doublepinyin_scheme, k, vb); \
    doublepinyin_scheme_insert_pair(&default_doublepinyin_scheme, k, vc);

    doublepinyin_scheme_insert_default_3pairs('q', "q", "iu", 0);
    doublepinyin_scheme_insert_default_3pairs('w', "w", "ia", "ua");
    doublepinyin_scheme_insert_default_3pairs('e', " ", "e", 0);
    doublepinyin_scheme_insert_default_3pairs('r', "r", "uan", "er");
    doublepinyin_scheme_insert_default_3pairs('t', "t", "ve", "ue");
    doublepinyin_scheme_insert_default_3pairs('y', "y", "uai", "ing");
    doublepinyin_scheme_insert_default_3pairs('u', "sh", "u", 0);
    doublepinyin_scheme_insert_default_3pairs('i', "ch", "i", 0);
    doublepinyin_scheme_insert_default_3pairs('o', "", "o", "uo");
    doublepinyin_scheme_insert_default_3pairs('p', "p", "un", 0);
    doublepinyin_scheme_insert_default_3pairs('a', " ", "a", 0);
    doublepinyin_scheme_insert_default_3pairs('s', "s", "ong", "iong");
    doublepinyin_scheme_insert_default_3pairs('d', "d", "uang", "iang");
    doublepinyin_scheme_insert_default_3pairs('f', "f", "en", 0);
    doublepinyin_scheme_insert_default_3pairs('g', "g", "eng", 0);
    doublepinyin_scheme_insert_default_3pairs('h', "h", "ang", 0);
    doublepinyin_scheme_insert_default_3pairs('j', "j", "an", 0);
    doublepinyin_scheme_insert_default_3pairs('k', "k", "ao", 0);
    doublepinyin_scheme_insert_default_3pairs('l', "l", "ai", 0);
    doublepinyin_scheme_insert_default_3pairs('z', "z", "ei", 0);
    doublepinyin_scheme_insert_default_3pairs('x', "x", "ie", 0);
    doublepinyin_scheme_insert_default_3pairs('c', "c", "iao", 0);
    doublepinyin_scheme_insert_default_3pairs('v', "zh", "ui", "v");
    doublepinyin_scheme_insert_default_3pairs('b', "b", "ou", 0);
    doublepinyin_scheme_insert_default_3pairs('n', "n", "in", 0);
    doublepinyin_scheme_insert_default_3pairs('m', "m", "ian", 0);
#undef doublepinyin_scheme_insert_default_3pairs

    doublepinyin_scheme_build_trie(&default_doublepinyin_scheme);
}
