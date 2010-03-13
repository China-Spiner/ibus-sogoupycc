/* 
 * File:   DoublePinyinScheme.h
 * Author: WU Jun <quark@lihdd.net>
 *
 * March 13, 2010
 *  created. split from PinyinUtility
 */

#ifndef _DOUBLEPINYINSCHEME_H
#define	_DOUBLEPINYINSCHEME_H

#include <string>
#include <vector>
#include <map>

using std::string;
using std::vector;
using std::map;
using std::pair;

class DoublePinyinScheme {
public:
    DoublePinyinScheme();
    DoublePinyinScheme(const DoublePinyinScheme& orig);

    void bindKey(const char key, const string& consonant, const vector<string>& vowels);
    void clear();
    const int buildMap();
    const string query(const char firstChar, const char secondChar);
    const string query(const string& doublePinyinString);
    const bool isValidDoublePinyin(const string& doublePinyinString);
    const bool isKeyBinded(const char key);

    virtual ~DoublePinyinScheme();

private:
    bool mapBuilt;
    map<char, pair<string, vector<string> > > bindedKeys;
    map<pair<char, char>, string> queryMap;
};

#endif	/* _DOUBLEPINYINSCHEME_H */

