/* 
 * File:   DoublePinyinScheme.cpp
 * Author: WU Jun <quark@lihdd.net>
 */

#include <cstdio>
#include "DoublePinyinScheme.h"
#include "PinyinUtility.h"

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

const string DoublePinyinScheme::query(const char firstKey, const char secondKey) {
    if (!mapBuilt) buildMap();
    map<pair<char, char>, string>::const_iterator it =
            queryMap.find(pair<char, char>(firstKey, secondKey));

    if (it != queryMap.end()) return it->second;
    else return "";
}

const string DoublePinyinScheme::query(const string& doublePinyinString) {
    string result;
    char keys[2];
    int p = 0;
    for (size_t i = 0; i < doublePinyinString.length(); ++i) {
        char key = doublePinyinString[i];
        if (bindedKeys.count(key) > 0) {
            keys[p] = key;
            p ^= 1;
            if (p == 0) {
                if (result.empty())
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
        if (consonant.empty())
            result += "'";
        else result += consonant;
    }
    return result;
}

const bool DoublePinyinScheme::isValidDoublePinyin(const string& doublePinyinString) {
    char keys[2];
    int p = 0;
    for (size_t i = 0; i < doublePinyinString.length(); ++i) {
        keys[p++] = doublePinyinString[i];
        if (p == 2) {
            if (query(keys[0], keys[1]).empty()) return false;
            p = 0;
        }
    }
    if (p == 1) {
        return bindedKeys[keys[0]].first != "-";
    }
    return true;
}

const int DoublePinyinScheme::buildMap() {
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

const bool DoublePinyinScheme::isKeyBinded(const char key) {
    return bindedKeys.find(key) != bindedKeys.end();
}