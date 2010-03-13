/* 
 * File:   PinyinUtility.h
 * Author: WU Jun <quark@lihdd.net>
 *
 * March 13, 2010
 *  split out DoublePinyinScheme
 * February 28, 2010
 *  0.1.1 major bugs fixed
 * February 27, 2010
 *  0.1.0 first release
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
using std::string;
using std::pair;

class PinyinUtility {
public:
    PinyinUtility();
    virtual ~PinyinUtility();

    static const bool isValidPinyin(const string& pinyin);
    static const bool isValidPartialPinyin(const string& pinyin);

    static const string charactersToPinyins(const string& characters, bool includeTone = false);
    static const string getCandidates(const string& pinyin, int tone);
    /**
     * add essential space as seperator (greedy)
     * "womenzaizheliparseerror" => "wo men zai zhe li pa r se er r o r"
     */
    static const string separatePinyins(const string& pinyins);
    static const int VALID_PINYIN_MAX_LENGTH;

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
    static set<string> validPartialPinyins;
    static void staticInitializer();
};

#endif	/* _PINYINUTILITY_H */

