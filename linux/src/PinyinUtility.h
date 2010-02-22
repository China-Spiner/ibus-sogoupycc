/* 
 * File:   PinyinUtility.h
 * Author: WU Jun <quark@lihdd.net>
 *
 * February 19, 2010
 *  extended to PinyinUtility. originally DoublePinyin Utility.
 * February 6, 2010
 *  created. reimplement original c code (doublepinyin_scheme.{c,h})
 *  this class provides interface to build double pinyin lookup map from
 *  key map and to check or convert double pinyin.
 *  as designed, it should be instantiated once by LuaIbusBinding class.
 */

#ifndef _PINYINUTILITY_H
#define	_PINYINUTILITY_H

#include <cstdio>
#include <vector>
#include <set>
#include <map>
#include <string>

using std::map;
using std::multimap;
using std::set;
using std::vector;
using std::string;
using std::pair;

class PinyinUtility {
public:
    PinyinUtility();
    virtual ~PinyinUtility();

    static bool isValidPinyin(const string& pinyin);

    static string charactersToPinyins(const string& characters, bool includeTone = false);
    static string getCandidates(const string& pinyin, int tone);

private:
    PinyinUtility(const PinyinUtility& orig);

    template<typename Type>
    class setInitializer {
        std::set<Type> s;
    public:

        setInitializer(const Type& x) {
            s.insert(x);
        }

        setInitializer & operator()(const Type& x) {
            s.insert(x);
            return *this;
        }

        const std::set<Type> & operator ()() const {
            return s;
        }
    };

    static map<string, string> gb2312pinyinMap;
    static multimap<string, map<string, string>::iterator> gb2312characterMap;
    static const set<string> validPinyins;
    static void staticInitializer();
};

// DoublePinyinScheme
class DoublePinyinScheme {
public:
    DoublePinyinScheme();
    DoublePinyinScheme(const DoublePinyinScheme& orig);

    void bindKey(const char key, const string& consonant, const vector<string>& vowels);
    void clear();
    int buildMap();
    string query(const char firstChar, const char secondChar);
    string query(const string& doublePinyinString);
    bool isValidDoublePinyin(const string& doublePinyinString);

    virtual ~DoublePinyinScheme();
private:
    bool mapBuilt;
    map<char, pair<string, vector<string> > > bindedKeys;
    map<pair<char, char>, string> queryMap;
};

#endif	/* _PINYINUTILITY_H */

