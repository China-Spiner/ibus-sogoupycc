/* 
 * File:   PinyinSequence.cpp
 * Author: WU Jun <quark@lihdd.net>
 */

#include "PinyinSequence.h"
#include "defines.h"
#include <algorithm>


PinyinSequence::PinyinSequence(const PinyinSequence& orig) {
    DEBUG_PRINT(7, "PinyinSequence::<copy construct>\n");
    pinyinVector = orig.pinyinVector;
}

PinyinSequence::PinyinSequence(const char* pinyins, const char separator) {
    DEBUG_PRINT(7, "PinyinSequence(\"%s\", '%c')\n", pinyins, separator);
    pinyinVector = splitString((string)pinyins, separator);
}

PinyinSequence::PinyinSequence(const string& pinyins, const char separator) {
    DEBUG_PRINT(7, "PinyinSequence(\"%s\", '%c')\n", pinyins.c_str(), separator);
    pinyinVector = splitString(pinyins, separator);
}

PinyinSequence::~PinyinSequence() {
    DEBUG_PRINT(7, "PinyinSequence::<deconstruct>\n");
}

string PinyinSequence::toString(const size_t startIndex, const size_t length, const char separator) const {
    size_t stopIndex = pinyinVector.size();
    if (length && stopIndex > startIndex + length)stopIndex = startIndex + length;

    string r = "";
    for (size_t i = startIndex; i < stopIndex; i++) {
        if (i > startIndex) r += separator;
        r += pinyinVector[i];
    }
    return r;
}

const string PinyinSequence::operator [](size_t index) const {
    if (index >= 0 && index < pinyinVector.size())
        return pinyinVector[index];
    else return "";
}

const size_t PinyinSequence::size() const {
    return pinyinVector.size();
}

vector<string> splitString(string toBeSplited, char separator) {
    vector<string> r;
    size_t lastPos = 0;
    for (size_t pos = toBeSplited.find(separator);; pos = toBeSplited.find(separator, pos + 1)) {
        r.push_back(toBeSplited.substr(lastPos, pos - lastPos));
        if (pos == string::npos) break;
        lastPos = pos + 1;
    }
    return r;
}
