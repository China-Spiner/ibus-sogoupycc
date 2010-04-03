/*
 * File:   PinyinDatabase.h
 * Author: WU Jun <quark@lihdd.net>
 */

#include <cstdio>
#include <cmath>
#include <unistd.h>
#include <sstream>
#include <glib.h>

#include "PinyinDatabase.h"
#include "defines.h"
#include "PinyinSequence.h"
#include "Configuration.h"

#define DB_CACHE_SIZE "16384"
#define DB_PREFETCH_LEN 6 

using std::istringstream;
using std::ostringstream;

PinyinDatabase::PinyinDatabase(const string dbPath, const double weight) {
    DEBUG_PRINT(1, "[PYDB] PinyinDatabase(%s, %.2lf)\n", dbPath.c_str(), weight);
    this->weight = weight;
    // do not direct write to PinyinDatabase::db (for thread safe)
    sqlite3 *db;
    if (dbPath.empty()) db = NULL;
    else if (sqlite3_open_v2(dbPath.c_str(), &db, SQLITE_OPEN_READONLY | SQLITE_OPEN_EXCLUSIVE | SQLITE_OPEN_FULLMUTEX, NULL) != SQLITE_OK) {
        db = NULL;
    }
    if (db) {
        // set PRAGMA parameters
        string sql = "PRAGMA cache_size = " DB_CACHE_SIZE ";\n";
        sql += "PRAGMA temp_store = MEMORY;\n";
        char *errMessage;
        if (sqlite3_exec(db, sql.c_str(), NULL, NULL, &errMessage) == SQLITE_OK) {
            // prefetch then
            sql = "";
            for (size_t i = 0; i < DB_PREFETCH_LEN; i++) {
                char buf[8];
                snprintf(buf, sizeof (buf), "%d", i);
                sql += string("SELECT * FROM py_phrase_") + buf + ";\n";
            }
            sqlite3_exec(db, sql.c_str(), NULL, NULL, NULL);
        } else {
            fprintf(stderr, "%s", errMessage);
            sqlite3_close(db);
            db = NULL;
        }
        sqlite3_free(errMessage);
    }
    this->db = db;
}

PinyinDatabase::PinyinDatabase(const PinyinDatabase& orig) {
}

PinyinDatabase::~PinyinDatabase() {
    DEBUG_PRINT(1, "[PYDB] ~PinyinDatabase\n");
    if (db) {
        if (sqlite3_close(db) == SQLITE_OK) db = NULL;
    }
}

const bool PinyinDatabase::isDatabaseOpened() const {
    return (db != NULL);
}

void PinyinDatabase::query(const string pinyins, CandidateList& candidateList, const int limitCount, const double longPhraseAdjust, const int limitLength) {
    query(PinyinSequence(pinyins), candidateList, limitCount, longPhraseAdjust, limitLength);
}

void PinyinDatabase::query(const PinyinSequence& pinyins, CandidateList& candidateList, const int countLimit, const double longPhraseAdjust, const int lengthLimit) {
    DEBUG_PRINT(3, "[PYDB] query: %s\n", pinyins.toString().c_str());
    if (!db) return;

    ostringstream queryWhere;
    ostringstream query;
    int lengthMax = lengthLimit;

    if (lengthMax < 0) lengthMax = 1;
    if (lengthMax > PINYIN_DB_ID_MAX) lengthMax = PINYIN_DB_ID_MAX;

    /**
     * sql sample:
     * SELECT  phrase, freqadj FROM (
     *   SELECT phrase, freq* 1 AS freqadj FROM main.py_phrase_0 WHERE s0=4 AND y0=29 UNION ALL
     *   SELECT phrase, freq * 2.33 AS freqadj FROM main.py_phrase_1 WHERE s0=4 AND y0=29 AND s1=4 AND y1=28
     * ) GROUP BY phrase ORDER BY freqadj DESC LIMIT 30
     */
    query << "SELECT phrase, freqadj FROM (";
    // build query statement
    for (size_t id = 0; id < pinyins.size(); ++id) {
        // "chuang qian ming yue guang"
        //        ^    ^
        //  lastPos  pos
        if ((int) id > lengthMax) break;

        string pinyin = pinyins[id];

        DEBUG_PRINT(5, "[PYDB] for length = %d, pinyin = %s\n", id + 1, pinyin.c_str());

        int cid, vid;
        PinyinDatabase::getPinyinIDs(pinyin, cid, vid);

        // consonant not available, stop here
        if (cid == PinyinDefines::PINYIN_ID_VOID) break;

        // update WHERE sent.
        if (!queryWhere.str().empty()) queryWhere << " AND ";

        queryWhere << "s" << id << "=" << cid;
        // vowel not available, only use consonant
        if (vid != PinyinDefines::PINYIN_ID_VOID) {
            queryWhere << " AND y" << id << "=" << vid;
        }

        // update query
        if (id > 0) query << " UNION ALL ";
        query << "SELECT phrase, freq * " << pow(id + 1, longPhraseAdjust) * weight <<
                " AS freqadj FROM main.py_phrase_" << id << " WHERE " << queryWhere.str();
    }

    // finish constructing query
    query << string(") GROUP BY phrase ORDER BY freqadj DESC");
    if (countLimit > 0) {
        query << " LIMIT " << countLimit;
    }

    DEBUG_PRINT(5, "[PYDB] query SQL: %s\n", query.str().c_str());

    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, query.str().c_str(), query.str().length(), &stmt, NULL) != SQLITE_OK) return;

    for (bool running = true; running;) {
        switch (sqlite3_step(stmt)) {
            case SQLITE_ROW:
            {
                string phrase = (const char*) sqlite3_column_text(stmt, 0);
                double freq = sqlite3_column_double(stmt, 1);
                candidateList.insert(pair<double, string > (freq, phrase));

                // do not quit abnormally, use LIMIT in sql query
                // if (limitCount > 0 && ++count >= limitCount) running = false;
                break;
            }
            case SQLITE_DONE:
            {
                running = false;
                break;
            }
            case SQLITE_BUSY:
            {
                // sleep a short time, say, 1 ms
                usleep(1024);
            }
            case SQLITE_MISUSE:
            {
                fprintf(stderr, "sqlite3_step() misused.\nthis should not happen.");
                running = false;
                break;
            }
            case SQLITE_ERROR: default:
            {
                fprintf(stderr, "sqlite3_step() error: %s\n (ignored).\n", sqlite3_errmsg(db));
                running = false;
                break;
            }
        }
    }
    sqlite3_finalize(stmt);
}

string PinyinDatabase::greedyConvert(const string& pinyins, const double longPhraseAdjust, int lengthLimit) {
    return greedyConvert(PinyinSequence(pinyins), longPhraseAdjust, lengthLimit);
}

string PinyinDatabase::greedyConvert(const PinyinSequence& pinyins, const double longPhraseAdjust, int lengthLimit) {
    DEBUG_PRINT(3, "[PYDB] greedyConvert: %s\n", pinyins.toString().c_str());

    string r;
    if (!db) return r;

    if (sqlite3_threadsafe()) {
        DEBUG_PRINT(3, "[PYDB] sqlite3 is thread safe\n");
    } else {
        ibus_warning("sqlite is not thread safe! program is likely to crash soon.\n");
    }

    if (lengthLimit > PINYIN_DB_ID_MAX) lengthLimit = PINYIN_DB_ID_MAX;
    else if (lengthLimit < 1) lengthLimit = 1;

    // build query SQL
    for (int id = (int) pinyins.size() - 1; id >= 0;) {
        int matchLength = 0;
        // check cache first
        string phrase = Configuration::getGlobalCache(pinyins.toString(0, id + 1), true);

        if (phrase.empty()) {
            int lengthMax = id + 1;
            if (lengthMax > lengthLimit) lengthMax = lengthLimit;

            ostringstream query;
            query << "SELECT phrase, freqadj FROM (";

            for (int l = lengthMax; l > 0; --l) {
                // try construct from pinyins[id - l + 1 .. id]
                ostringstream queryWhere;
                for (int p = id - l + 1; p <= id; p++) {
                    string pinyin = pinyins[p];
                    int cid, vid;
                    PinyinDatabase::getPinyinIDs(pinyin, cid, vid);
                    if (!queryWhere.str().empty()) queryWhere << " AND ";
                    queryWhere << "s" << p - (id - l + 1) << "=" << cid;
                    if (vid != PinyinDefines::PINYIN_ID_VOID) queryWhere << " AND y" << p - (id - l + 1) << "=" << vid;
                }
                if (l != lengthMax) query << " UNION ALL ";
                query << "SELECT phrase, freq * " << pow(l, longPhraseAdjust) <<
                        " AS freqadj FROM main.py_phrase_" << l - 1 << " WHERE " << queryWhere.str();
            }
            query << string(") GROUP BY phrase ORDER BY freqadj DESC LIMIT 1");

            sqlite3_stmt *stmt;
            if (sqlite3_prepare_v2(db, query.str().c_str(), query.str().length(), &stmt, NULL) != SQLITE_OK) return r;

            for (bool running = true; running;) {
                switch (sqlite3_step(stmt)) {
                    case SQLITE_ROW:
                    {
                        // got it !
                        phrase = (const char*) sqlite3_column_text(stmt, 0);
                        break;
                    }
                    case SQLITE_DONE:
                    {
                        running = false;
                        break;
                    }
                    case SQLITE_BUSY:
                    {
                        // sleep a short time, say, 1 ms
                        usleep(1024);
                    }
                    case SQLITE_MISUSE:
                    {
                        fprintf(stderr, "sqlite3_step() misused.\nthis should not happen.");
                        running = false;
                        break;
                    }
                    case SQLITE_ERROR: default:
                    {
                        fprintf(stderr, "sqlite3_step() error: %s\n (ignored).\n", sqlite3_errmsg(db));
                        running = false;
                        break;
                    }
                }
            }
            sqlite3_finalize(stmt);
        } else {
            DEBUG_PRINT(3, "[PYDB] got cache[%s] = %s\n", pinyins.toString(0, id + 1).c_str(), phrase.c_str());
        }
        matchLength = (int) g_utf8_strlen(phrase.c_str(), -1);

        if (matchLength == 0) {
            // can't convert just skip this pinyin -,-
            r = pinyins[id] + r;
            id--;
        } else {
            r = phrase + r;
            id -= matchLength;
        }

        if (id < 0) {
            // all done
            break;
        }
    }
    DEBUG_PRINT(3, "[PYDB] greedyConvert result %s\n", r.c_str());
    // write to cache
    Configuration::writeGlobalCache(pinyins.toString(), r, true);
    return r;
}

string PinyinDatabase::getPinyinFromID(int consonantId, int vowelId) {
    string consonant, vowel;
    switch (consonantId) {
        case PinyinDefines::PINYIN_ID_Q: consonant = "q";
            break;
        case PinyinDefines::PINYIN_ID_W: consonant = "w";
            break;
        case PinyinDefines::PINYIN_ID_R: consonant = "r";
            break;
        case PinyinDefines::PINYIN_ID_T: consonant = "t";
            break;
        case PinyinDefines::PINYIN_ID_Y: consonant = "y";
            break;
        case PinyinDefines::PINYIN_ID_P: consonant = "p";
            break;
        case PinyinDefines::PINYIN_ID_S: consonant = "s";
            break;
        case PinyinDefines::PINYIN_ID_D: consonant = "d";
            break;
        case PinyinDefines::PINYIN_ID_F: consonant = "f";
            break;
        case PinyinDefines::PINYIN_ID_G: consonant = "g";
            break;
        case PinyinDefines::PINYIN_ID_H: consonant = "h";
            break;
        case PinyinDefines::PINYIN_ID_J: consonant = "j";
            break;
        case PinyinDefines::PINYIN_ID_K: consonant = "k";
            break;
        case PinyinDefines::PINYIN_ID_L: consonant = "l";
            break;
        case PinyinDefines::PINYIN_ID_Z: consonant = "z";
            break;
        case PinyinDefines::PINYIN_ID_X: consonant = "x";
            break;
        case PinyinDefines::PINYIN_ID_C: consonant = "c";
            break;
        case PinyinDefines::PINYIN_ID_B: consonant = "b";
            break;
        case PinyinDefines::PINYIN_ID_N: consonant = "n";
            break;
        case PinyinDefines::PINYIN_ID_M: consonant = "m";
            break;
        case PinyinDefines::PINYIN_ID_ZH: consonant = "zh";
            break;
        case PinyinDefines::PINYIN_ID_CH: consonant = "ch";
            break;
        case PinyinDefines::PINYIN_ID_SH: consonant = "sh";
            break;
    }

    switch (vowelId) {
        case PinyinDefines::PINYIN_ID_A: vowel = "a";
            break;
        case PinyinDefines::PINYIN_ID_AI: vowel = "ai";
            break;
        case PinyinDefines::PINYIN_ID_AN: vowel = "an";
            break;
        case PinyinDefines::PINYIN_ID_ANG: vowel = "ang";
            break;
        case PinyinDefines::PINYIN_ID_AO: vowel = "ao";
            break;
        case PinyinDefines::PINYIN_ID_E: vowel = "e";
            break;
        case PinyinDefines::PINYIN_ID_EI: vowel = "ei";
            break;
        case PinyinDefines::PINYIN_ID_EN: vowel = "en";
            break;
        case PinyinDefines::PINYIN_ID_ENG: vowel = "eng";
            break;
        case PinyinDefines::PINYIN_ID_ER: vowel = "er";
            break;
        case PinyinDefines::PINYIN_ID_I: vowel = "i";
            break;
        case PinyinDefines::PINYIN_ID_IA: vowel = "ia";
            break;
        case PinyinDefines::PINYIN_ID_IAN: vowel = "ian";
            break;
        case PinyinDefines::PINYIN_ID_IANG: vowel = "iang";
            break;
        case PinyinDefines::PINYIN_ID_IAO: vowel = "iao";
            break;
        case PinyinDefines::PINYIN_ID_IE: vowel = "ie";
            break;
        case PinyinDefines::PINYIN_ID_IN: vowel = "in";
            break;
        case PinyinDefines::PINYIN_ID_ING: vowel = "ing";
            break;
        case PinyinDefines::PINYIN_ID_IONG: vowel = "iong";
            break;
        case PinyinDefines::PINYIN_ID_IU: vowel = "iu";
            break;
        case PinyinDefines::PINYIN_ID_O: vowel = "o";
            break;
        case PinyinDefines::PINYIN_ID_ONG: vowel = "ong";
            break;
        case PinyinDefines::PINYIN_ID_OU: vowel = "ou";
            break;
        case PinyinDefines::PINYIN_ID_U: vowel = "u";
            break;
        case PinyinDefines::PINYIN_ID_UA: vowel = "ua";
            break;
        case PinyinDefines::PINYIN_ID_UAI: vowel = "uai";
            break;
        case PinyinDefines::PINYIN_ID_UAN: vowel = "uan";
            break;
        case PinyinDefines::PINYIN_ID_UANG: vowel = "uang";
            break;
        case PinyinDefines::PINYIN_ID_UE:/*case PinyinDefines::PINYIN_ID_VE:*/
            vowel = "ue";
            break;
        case PinyinDefines::PINYIN_ID_UI: vowel = "ui";
            break;
        case PinyinDefines::PINYIN_ID_UN: vowel = "un";
            break;
        case PinyinDefines::PINYIN_ID_UO: vowel = "uo";
            break;
        case PinyinDefines::PINYIN_ID_V: vowel = "v";
            break;

    }
    return consonant + vowel;
}

void PinyinDatabase::getPinyinIDs(const string pinyin, int& consonantId, int& vowelId) {
    // IMPROVE: use efficiant lookup method
    if (pinyin.empty()) {
        consonantId = vowelId = PinyinDefines::PINYIN_ID_VOID;
        return;
    }

    // consonant
    string consonant = pinyin.substr(0, 2);
    if (consonant == "zh") consonantId = PinyinDefines::PINYIN_ID_ZH;
    else if (consonant == "ch") consonantId = PinyinDefines::PINYIN_ID_CH;
    else if (consonant == "sh") consonantId = PinyinDefines::PINYIN_ID_SH;
    else {
        consonant = consonant.substr(0, 1);
        switch (consonant.data()[0]) {
            case 'q': consonantId = PinyinDefines::PINYIN_ID_Q;
                break;
            case 'w': consonantId = PinyinDefines::PINYIN_ID_W;
                break;
            case 'r': consonantId = PinyinDefines::PINYIN_ID_R;
                break;
            case 't': consonantId = PinyinDefines::PINYIN_ID_T;
                break;
            case 'y': consonantId = PinyinDefines::PINYIN_ID_Y;
                break;
            case 'p': consonantId = PinyinDefines::PINYIN_ID_P;
                break;
            case 's': consonantId = PinyinDefines::PINYIN_ID_S;
                break;
            case 'd': consonantId = PinyinDefines::PINYIN_ID_D;
                break;
            case 'f': consonantId = PinyinDefines::PINYIN_ID_F;
                break;
            case 'g': consonantId = PinyinDefines::PINYIN_ID_G;
                break;
            case 'h': consonantId = PinyinDefines::PINYIN_ID_H;
                break;
            case 'j': consonantId = PinyinDefines::PINYIN_ID_J;
                break;
            case 'k': consonantId = PinyinDefines::PINYIN_ID_K;
                break;
            case 'l': consonantId = PinyinDefines::PINYIN_ID_L;
                break;
            case 'z': consonantId = PinyinDefines::PINYIN_ID_Z;
                break;
            case 'x': consonantId = PinyinDefines::PINYIN_ID_X;
                break;
            case 'c': consonantId = PinyinDefines::PINYIN_ID_C;
                break;
            case 'b': consonantId = PinyinDefines::PINYIN_ID_B;
                break;
            case 'n': consonantId = PinyinDefines::PINYIN_ID_N;
                break;
            case 'm': consonantId = PinyinDefines::PINYIN_ID_M;
                break;
            default:
                consonantId = PinyinDefines::PINYIN_ID_ZERO;
                consonant = "";
        }
    }

    // vowel, no loner than 32 characters
    string vowel = pinyin.substr(consonant.length(), 32);
    if (vowel == "a") vowelId = PinyinDefines::PINYIN_ID_A;
    else if (vowel == "ai") vowelId = PinyinDefines::PINYIN_ID_AI;
    else if (vowel == "an") vowelId = PinyinDefines::PINYIN_ID_AN;
    else if (vowel == "ang") vowelId = PinyinDefines::PINYIN_ID_ANG;
    else if (vowel == "ao") vowelId = PinyinDefines::PINYIN_ID_AO;
    else if (vowel == "e") vowelId = PinyinDefines::PINYIN_ID_E;
    else if (vowel == "ei") vowelId = PinyinDefines::PINYIN_ID_EI;
    else if (vowel == "en") vowelId = PinyinDefines::PINYIN_ID_EN;
    else if (vowel == "eng") vowelId = PinyinDefines::PINYIN_ID_ENG;
    else if (vowel == "er") vowelId = PinyinDefines::PINYIN_ID_ER;
    else if (vowel == "i") vowelId = PinyinDefines::PINYIN_ID_I;
    else if (vowel == "ia") vowelId = PinyinDefines::PINYIN_ID_IA;
    else if (vowel == "ian") vowelId = PinyinDefines::PINYIN_ID_IAN;
    else if (vowel == "iang") vowelId = PinyinDefines::PINYIN_ID_IANG;
    else if (vowel == "iao") vowelId = PinyinDefines::PINYIN_ID_IAO;
    else if (vowel == "ie") vowelId = PinyinDefines::PINYIN_ID_IE;
    else if (vowel == "in") vowelId = PinyinDefines::PINYIN_ID_IN;
    else if (vowel == "ing") vowelId = PinyinDefines::PINYIN_ID_ING;
    else if (vowel == "iong") vowelId = PinyinDefines::PINYIN_ID_IONG;
    else if (vowel == "iu") vowelId = PinyinDefines::PINYIN_ID_IU;
    else if (vowel == "o") vowelId = PinyinDefines::PINYIN_ID_O;
    else if (vowel == "ong") vowelId = PinyinDefines::PINYIN_ID_ONG;
    else if (vowel == "ou") vowelId = PinyinDefines::PINYIN_ID_OU;
    else if (vowel == "u") vowelId = PinyinDefines::PINYIN_ID_U;
    else if (vowel == "ua") vowelId = PinyinDefines::PINYIN_ID_UA;
    else if (vowel == "uai") vowelId = PinyinDefines::PINYIN_ID_UAI;
    else if (vowel == "uan") vowelId = PinyinDefines::PINYIN_ID_UAN;
    else if (vowel == "uang") vowelId = PinyinDefines::PINYIN_ID_UANG;
    else if (vowel == "ue") vowelId = PinyinDefines::PINYIN_ID_UE;
    else if (vowel == "ve") vowelId = PinyinDefines::PINYIN_ID_VE;
    else if (vowel == "ui") vowelId = PinyinDefines::PINYIN_ID_UI;
    else if (vowel == "un") vowelId = PinyinDefines::PINYIN_ID_UN;
    else if (vowel == "uo") vowelId = PinyinDefines::PINYIN_ID_UO;
    else if (vowel == "v") vowelId = PinyinDefines::PINYIN_ID_V;
        //else if (vowel == "ng") vowelId = PinyinDefines::PINYIN_ID_VOID;
    else {
        vowelId = PinyinDefines::PINYIN_ID_VOID;
        if (consonantId == PinyinDefines::PINYIN_ID_ZERO)
            consonantId = PinyinDefines::PINYIN_ID_VOID;
    }
}

