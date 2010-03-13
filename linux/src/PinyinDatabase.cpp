/*
 * File:   PinyinDatabase.h
 * Author: WU Jun <quark@lihdd.net>
 */

#include <cstdio>
#include <cmath>
#include <unistd.h>

#include "PinyinDatabase.h"
#include "defines.h"

#define DB_CACHE_SIZE "16384"
#define DB_PREFETCH_LEN 6 

PinyinDatabase::PinyinDatabase(const string dbPath, const double weight) {
    DEBUG_PRINT(1, "[PYDB] PinyinDatabase(%s, %.2lf)\n", dbPath.c_str(), weight);
    this->weight = weight;
    // do not direct write to PinyinDatabase::db (for thread safe)
    sqlite3 *db;
    if (dbPath.empty()) db = NULL;
    else if (sqlite3_open_v2(dbPath.c_str(), &db, SQLITE_OPEN_READONLY, NULL) != SQLITE_OK) {
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
                snprintf(buf, sizeof(buf), "%d", i);
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

void PinyinDatabase::query(const string pinyins, CandidateList& candidateList, const int countLimit, const double longPhraseAdjust, const int lengthLimit) {
    DEBUG_PRINT(3, "[PYDB] query: %s\n", pinyins.c_str());

    if (!db) return;

    string s = pinyins + " ";
    string queryWhere;
    char idString[12], whereBuffer[128], limitBuffer[32];
    int lengthMax = lengthLimit;

    if (lengthMax < 0) lengthMax = 1;
    if (lengthMax > PINYIN_DB_ID_MAX) lengthMax = PINYIN_DB_ID_MAX;

    for (size_t pos = s.find(' '), lastPos = -1, id = 0; pos != string::npos; id++, lastPos = pos, pos = s.find(' ', pos + 1)) {
        // "chuang qian ming yue guang"
        //        ^    ^
        //  lastPos  pos
        if ((int)id > lengthMax) break;
        string pinyin = s.substr(lastPos + 1, pos - lastPos - 1);

        snprintf(idString, sizeof (idString), "%d", id);

        DEBUG_PRINT(5, "[PYDB] for length = %d, pinyin = %s\n", id + 1, pinyin.c_str());

        int cid, vid;
        PinyinDatabase::getPinyinIDs(pinyin, cid, vid);

        // consonant not available, stop here
        if (cid == PinyinDefines::PINYIN_ID_VOID) break;

        // build query and quey
        if (countLimit > 0) {
            snprintf(limitBuffer, sizeof (limitBuffer), " LIMIT %d", countLimit);
        } else {
            limitBuffer[0] = '\0';
        }

        // vowel not available, only use consonant
        if (vid == PinyinDefines::PINYIN_ID_VOID) {
            snprintf(whereBuffer, sizeof (whereBuffer), "s%d=%d", id, cid);
        } else {
            snprintf(whereBuffer, sizeof (whereBuffer), "s%d=%d AND y%d=%d", id, cid, id, vid);
        }

        if (!queryWhere.empty()) queryWhere += " AND ";
        queryWhere += whereBuffer;

        string query = "SELECT phrase, freq FROM main.py_phrase_";
        query += idString;
        query += " WHERE ";
        query += queryWhere + " GROUP BY phrase ORDER BY freq DESC " + limitBuffer;

        DEBUG_PRINT(5, "[PYDB] query SQL: %s\n", query.c_str());

        sqlite3_stmt *stmt;

        if (sqlite3_prepare_v2(db, query.c_str(), query.length(), &stmt, NULL) != SQLITE_OK) break;

        for (bool running = true; running;) {
            switch (sqlite3_step(stmt)) {
                case SQLITE_ROW:
                {
                    string phase = (const char*) sqlite3_column_text(stmt, 0);
                    double freq = sqlite3_column_double(stmt, 1) * weight * pow(id + 1, longPhraseAdjust);
                    candidateList.insert(pair<double, string > (freq, phase));

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
                    // sleep a short time, say, 64 ms
                    usleep(64000);
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

