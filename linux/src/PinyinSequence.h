/* 
 * File:   PinyinSequence.h
 * Author: WU Jun <quark@lihdd.net>
 *
 * convert between vector<string>("a", "ba") with string("a ba")
 * also split recognised chinese characters
 */

#ifndef _PINYINSEQUENCE_H
#define	_PINYINSEQUENCE_H

#include <string>
#include <vector>

using std::string;
using std::vector;

/**
 * PinyinSequence is a sequence of full or partical pinyins
 */
class PinyinSequence {
public:
    PinyinSequence(const char* pinyins = "", const char separator = ' ');
    PinyinSequence(const string& pinyins, const char separator = ' ');
    PinyinSequence(const PinyinSequence& orig);

    const bool empty();
    void removeAt(const size_t index);
    void clear();
    const string operator [] (size_t index) const;
    const size_t size() const;
    /**
     * @param length 0 indicates full length
     */
    string toString(const size_t startIndex = 0, const size_t length = 0, const char separator = ' ') const;
    void fromString(const string& pinyins, const char separator = ' ');

    virtual ~PinyinSequence();
private:
    vector<string> pinyinVector;
};

vector<string> splitString(string toBeSplited, char separator);

#endif	/* _PINYINSEQUENCE_H */

