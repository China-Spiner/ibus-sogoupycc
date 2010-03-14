/* 
 * File:   PinyinSequence.cpp
 * Author: WU Jun <quark@lihdd.net>
 */

#include "PinyinSequence.h"
#include "defines.h"
#include "PinyinUtility.h"
#include <algorithm>

void PinyinSequence::fromString(const string& pinyins, const char separator) {
    // add space to valid chinese chars
    int state = 0;
    string preprocessedPinyins;
    DEBUG_PRINT(6, "[PSEQ] PinyinSequence::fromString('%s')\n", pinyins.c_str());
    for (size_t pos = 0; pos < pinyins.length();) {
        if (PinyinUtility::isRecognisedCharacter(pinyins.substr(pos, 3))) {
            if (!preprocessedPinyins.empty()) preprocessedPinyins += separator;
            preprocessedPinyins += pinyins.substr(pos, 3);
            DEBUG_PRINT(7, "[PSEQ]  get chinese: '%s'\n", pinyins.substr(pos, 3).c_str());
            pos += 3, state = 1;
        } else {
            if (state != 2) if (!preprocessedPinyins.empty()) preprocessedPinyins += separator;
            preprocessedPinyins += pinyins.substr(pos, 1);
            DEBUG_PRINT(7, "[PSEQ]  get normal char: '%s'\n", pinyins.substr(pos, 1).c_str());
            pos++, state = 2;
        }
    }
    DEBUG_PRINT(7, "[PSEQ]  final string: '%s'\n", preprocessedPinyins.c_str());
    pinyinVector = splitString(preprocessedPinyins, separator);
}

PinyinSequence::PinyinSequence(const PinyinSequence& orig) {
    DEBUG_PRINT(7, "PinyinSequence::<copy construct>\n");
    pinyinVector = orig.pinyinVector;
}

PinyinSequence::PinyinSequence(const char* pinyins, const char separator) {
    DEBUG_PRINT(7, "PinyinSequence(\"%s\", '%c')\n", pinyins, separator);
    fromString(pinyins, separator);
}

PinyinSequence::PinyinSequence(const string& pinyins, const char separator) {
    DEBUG_PRINT(7, "PinyinSequence(\"%s\", '%c')\n", pinyins.c_str(), separator);
    fromString(pinyins, separator);
}

PinyinSequence::~PinyinSequence() {
    DEBUG_PRINT(7, "PinyinSequence::<deconstruct>\n");
}

string PinyinSequence::toString(const size_t startIndex, const size_t length, const char separator) const {
    size_t stopIndex = pinyinVector.size();
    if (length && stopIndex > startIndex + length)stopIndex = startIndex + length;

    string r = "";
    for (size_t i = startIndex; i < stopIndex; i++) {
        r += pinyinVector[i];
        if (i < stopIndex - 1 && !PinyinUtility::isRecognisedCharacter(pinyinVector[i])) r += separator;
    }
    return r;
}

const string PinyinSequence::operator [](size_t index) const {
    if (index >= 0 && index < pinyinVector.size())
        return pinyinVector[index];
    else return "";
}

const bool PinyinSequence::empty() {
    return pinyinVector.empty();
}

void PinyinSequence::clear() {
    pinyinVector.clear();
}

void PinyinSequence::removeAt(const size_t index) {
    if (index < pinyinVector.size()) {
        pinyinVector.erase(pinyinVector.begin() + index);
    }
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
