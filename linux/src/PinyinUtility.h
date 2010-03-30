/* 
 * File:   PinyinUtility.h
 * Author: WU Jun <quark@lihdd.net>
 *
 * provide basic api to convert / check pinyin, chinese characters
 * built-in gb2312 characters
 */

#ifndef _PINYINUTILITY_H
#define	_PINYINUTILITY_H


#include <set>
#include <map>
#include <string>
#include "PinyinSequence.h"

using std::map;
using std::multimap;
using std::set;
using std::string;
using std::pair;

class PinyinUtility {
public:
    PinyinUtility();
    virtual ~PinyinUtility();

    static const bool isRecognisedCharacter(const string& character);
    static const bool isCharactersPinyinsMatch(const string& character, const string& pinyin);
    static const bool isCharacterPinyinMatch(const string& character, const string& pinyin);
    static const bool isValidPinyin(const string& pinyin);
    static const bool isValidPartialPinyin(const string& pinyin);

    /**
     * @param index for multi-tone characters, return which one
     * @return pinyin string separated by space, "" if index out of range
     */
    static const string charactersToPinyins(const string& characters, size_t index, bool includeTone = false);
    static const string getCandidates(const string& pinyin, int tone);
    /**
     * add essential space as seperator (greedy)
     * "womenzaizheliparseerror" => "wo men zai zhe li pa r se er r o r"
     */
    static const string separatePinyins(const string& pinyins);
    static const int VALID_PINYIN_MAX_LENGTH;

    static void staticInit();
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
};

#endif	/* _PINYINUTILITY_H */

