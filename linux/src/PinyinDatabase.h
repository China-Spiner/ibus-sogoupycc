/* 
 * File:   PinyinDatabase.h
 * Author: WU Jun
 *
 * this class bind to a ibus-pinyin 1.2.9 compatible database
 * providing query api.
 */

#ifndef _PINYINDATABASE_H
#define	_PINYINDATABASE_H

#include <string>
#include <map>
#include <sqlite3.h>

#include "PinyinSequence.h"

#define PINYIN_DB_ID_MAX 15

using std::string;
using std::multimap;
using std::greater;
using std::pair;

// freq, cadidate
typedef multimap<double, string, greater<double> > CandidateList;

class PinyinDatabase {
public:
    PinyinDatabase(const string dbPath, const double weight = 1.0);
    
    /**
     * query pinyin database
     * @param pinyins space seperated pinyin sequence, e.g. "wo men kan dao le"
     * @param candidateList CandidateList which this function will add results to
     * @param limitCount limit result count (per length), 0 if no limit
     * @param longPhraseAdjust 0 if no adjust, positive move long phrase front, negative move short phrase front
     */
    void query(const string pinyins, CandidateList& candidateList, const int limitCount = 0, const double longPhraseAdjust = 0, const int limitLength = PINYIN_DB_ID_MAX);
    void query(const PinyinSequence& pinyins, CandidateList& candidateList, const int limitCount = 0, const double longPhraseAdjust = 0, const int limitLength = PINYIN_DB_ID_MAX);

    string greedyConvert(const string& pinyins, const double longPhraseAdjust = 4, int limitLength = PINYIN_DB_ID_MAX);
    string greedyConvert(const PinyinSequence& pinyins, const double longPhraseAdjust = 4, int limitLength = PINYIN_DB_ID_MAX);
    
    const bool isDatabaseOpened() const;

    virtual ~PinyinDatabase();

    /**
     * get consonant and vowel id of a full or partial pinyin
     * ids are defined below
     * here, hardcode.
     * @param pinyin e.g. "huang", "wo", "zhua"
     * @param consonantId return consonant id, if not parsable, it is -1, when no consonant, it is 0
     * @param vowelId return vowel id, if not parsable, it is -1
     */
    static void getPinyinIDs(const string pinyin, int& consonantId, int& vowelId);
    static string getPinyinFromID(int consonantId, int vowelId);
private:
    PinyinDatabase(const PinyinDatabase& orig);
    sqlite3 *db;
    double weight;
};


// from ibus-pinyin 1.2.99.20100212/src/Types.h, partical
namespace PinyinDefines {
    const int PINYIN_ID_VOID = -1;
    const int PINYIN_ID_ZERO = 0;
    const int PINYIN_ID_B = 1;
    const int PINYIN_ID_C = 2;
    const int PINYIN_ID_CH = 3;
    const int PINYIN_ID_D = 4;
    const int PINYIN_ID_F = 5;
    const int PINYIN_ID_G = 6;
    const int PINYIN_ID_H = 7;
    const int PINYIN_ID_J = 8;
    const int PINYIN_ID_K = 9;
    const int PINYIN_ID_L = 10;
    const int PINYIN_ID_M = 11;
    const int PINYIN_ID_N = 12;
    const int PINYIN_ID_P = 13;
    const int PINYIN_ID_Q = 14;
    const int PINYIN_ID_R = 15;
    const int PINYIN_ID_S = 16;
    const int PINYIN_ID_SH = 17;
    const int PINYIN_ID_T = 18;
    const int PINYIN_ID_W = 19;
    const int PINYIN_ID_X = 20;
    const int PINYIN_ID_Y = 21;
    const int PINYIN_ID_Z = 22;
    const int PINYIN_ID_ZH = 23;
    const int PINYIN_ID_A = 24;
    const int PINYIN_ID_AI = 25;
    const int PINYIN_ID_AN = 26;
    const int PINYIN_ID_ANG = 27;
    const int PINYIN_ID_AO = 28;
    const int PINYIN_ID_E = 29;
    const int PINYIN_ID_EI = 30;
    const int PINYIN_ID_EN = 31;
    const int PINYIN_ID_ENG = 32;
    const int PINYIN_ID_ER = 33;
    const int PINYIN_ID_I = 34;
    const int PINYIN_ID_IA = 35;
    const int PINYIN_ID_IAN = 36;
    const int PINYIN_ID_IANG = 37;
    const int PINYIN_ID_IAO = 38;
    const int PINYIN_ID_IE = 39;
    const int PINYIN_ID_IN = 40;
    const int PINYIN_ID_ING = 41;
    const int PINYIN_ID_IONG = 42;
    const int PINYIN_ID_IU = 43;
    const int PINYIN_ID_O = 44;
    const int PINYIN_ID_ONG = 45;
    const int PINYIN_ID_OU = 46;
    const int PINYIN_ID_U = 47;
    const int PINYIN_ID_UA = 48;
    const int PINYIN_ID_UAI = 49;
    const int PINYIN_ID_UAN = 50;
    const int PINYIN_ID_UANG = 51;
    const int PINYIN_ID_UE = 52;
    const int PINYIN_ID_VE = PINYIN_ID_UE;
    const int PINYIN_ID_UI = 53;
    const int PINYIN_ID_UN = 54;
    const int PINYIN_ID_UO = 55;
    const int PINYIN_ID_V = 56;
    const int PINYIN_ID_NG = PINYIN_ID_VOID;
}

#endif	/* _PINYINDATABASE_H */
