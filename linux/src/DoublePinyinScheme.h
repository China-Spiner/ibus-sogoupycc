/*
 * File:   DoublePinyinScheme.h
 * Author: WU Jun <quark@lihdd.net>
 *
 * February 6, 2010
 *  created. reimplement original c code (doublepinyin_scheme.{c,h})
 *  this class provides interface to build double pinyin lookup map from
 *  key map and to check or convert double pinyin.
 *  as designed, it should be instantiated once by LuaIbusBinding class.
 */

#ifndef _DOUBLEPINYINSCHEME_H
#define	_DOUBLEPINYINSCHEME_H

#include <string>
#include <set>
#include <map>
#include <vector>

using std::string;
using std::set;
using std::map;
using std::vector;
using std::pair;

class DoublePinyinScheme {
public:
    DoublePinyinScheme();
    DoublePinyinScheme(const DoublePinyinScheme& orig);
    static bool isValidPinyin(const string& pinyin);
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
        const std::set<Type>& operator ()() const {
            return s;
        }
    };

    static const set<string> validPinyins;
};

#endif	/* _DOUBLEPINYINSCHEME_H */

