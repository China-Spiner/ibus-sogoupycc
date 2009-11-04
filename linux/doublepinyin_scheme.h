/* 
 * File:   doublepinyin_scheme.h
 * Author: Jun WU <quark@lihdd.com>
 *
 * GNUv2 Licence
 *
 * Created on November 3, 2009
 */

#ifndef _DOUBLEPINYIN_SCHEME_H
#define	_DOUBLEPINYIN_SCHEME_H

#ifdef	__cplusplus
extern "C" {
#endif

    // this scheme only allow A-Z, no ';' is allowed

    struct doublepinyin_scheme_t {
        char *trie['z' - 'a' + 1]['z' - 'a' + 1];
        char **map_string['z' - 'a' + 1];
        int map_string_count['z' - 'a' + 1];
    };

    int is_valid_pinyin(char *string);
    void doublepinyin_scheme_init(struct doublepinyin_scheme_t *scheme);
    void doublepinyin_scheme_insert_pair(struct doublepinyin_scheme_t *scheme, char key, const char* value);
    void doublepinyin_scheme_free(struct doublepinyin_scheme_t *scheme);
    int doublepinyin_scheme_build_trie(struct doublepinyin_scheme_t *scheme);

    char* doublepinyin_scheme_query(struct doublepinyin_scheme_t *scheme, const char fisrt_char, const char second_char);
    char* doublepinyin_scheme_convert(struct doublepinyin_scheme_t *scheme, const char* doublepinyin_string, int curpos, int *retpos, int *err_parse, int *tail);

    void init_default_doublepinyin_scheme(void);
#ifdef	__cplusplus
}
#endif

#endif	/* _DOUBLEPINYINUTILS_H */

