/*
 * File:   engine.c
 * Author: Jun WU <quark@lihdd.com>
 *
 * GNUv2 Licence
 *
 * Created on November 3, 2009
 */

#define _REENTRANT

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "engine.h"
#include "sogoupycc.h"
#include "doublepinyin_scheme.h"
#ifndef UNUSED
#define UNUSED(x) ((void)(x));
#endif

typedef struct _IBusSgpyccEngine IBusSgpyccEngine;
typedef struct _IBusSgpyccEngineClass IBusEnchantEngineClass;

struct _IBusSgpyccEngine {
    IBusEngine parent;

    /* members */
    GString *preedit;
    gint cursor_pos;

    IBusLookupTable *table;

    int temporary_disabled;
    int use_double_pinyin;
    int last_keyvalue;

    // TODO: apply GString to these vars ?
    // pro: no length limit, con: slightly slow ?
#define IBUS_SGPYCC_FULLPINYIN_LENGTH_MAX 2048
#define IBUS_SGPYCC_PREEDIT_LENGTH_MAX 2304
    char fullpinyin[IBUS_SGPYCC_FULLPINYIN_LENGTH_MAX];
    char onscreen_preedit[IBUS_SGPYCC_PREEDIT_LENGTH_MAX];
};

struct _IBusSgpyccEngineClass {
    IBusEngineClass parent;
};


/* functions prototype */
static void ibus_sgpycc_engine_class_init(IBusEnchantEngineClass *klass);
static void ibus_sgpycc_engine_init(IBusSgpyccEngine *engine);
static void ibus_sgpycc_engine_destroy(IBusSgpyccEngine *engine);
static gboolean ibus_sgpycc_engine_process_key_event(IBusEngine *engine, guint keyval, guint modifiers, guint state);

static void ibus_sgpycc_engine_commit_string(IBusSgpyccEngine *sgpycc, const gchar *string);
static void ibus_sgpycc_engine_update(IBusSgpyccEngine *sgpycc);
static void ibus_sgpycc_callback_commit_string_func(void *sgpycc, const char *str);

static IBusEngineClass *parent_class = NULL;

GType ibus_sgpycc_engine_get_type(void) {
    static GType type = 0;

    static const GTypeInfo type_info = {
        sizeof (IBusEnchantEngineClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) ibus_sgpycc_engine_class_init,
        NULL,
        NULL,
        sizeof (IBusSgpyccEngine),
        0,
        (GInstanceInitFunc) ibus_sgpycc_engine_init,
    };

    if (type == 0) {
        type = g_type_register_static(IBUS_TYPE_ENGINE,
                "IBusSogouPinyinCloudClientEngine",
                &type_info,
                (GTypeFlags) 0);
    }

    return type;
}

static void
ibus_sgpycc_engine_class_init(IBusEnchantEngineClass *klass) {
    IBusObjectClass *ibus_object_class = IBUS_OBJECT_CLASS(klass);
    IBusEngineClass *engine_class = IBUS_ENGINE_CLASS(klass);

    parent_class = (IBusEngineClass *) g_type_class_peek_parent(klass);

    ibus_object_class->destroy = (IBusObjectDestroyFunc) ibus_sgpycc_engine_destroy;
    engine_class->process_key_event = &ibus_sgpycc_engine_process_key_event;
}

static void ibus_sgpycc_engine_init(IBusSgpyccEngine *sgpycc) {
    sgpycc->preedit = g_string_new("");
    sgpycc->cursor_pos = 0;
    sgpycc->temporary_disabled = 0;
#ifdef ALWAYS_USE_DOUBLEPINYIN
    sgpycc->use_double_pinyin = 1;
#else
    sgpycc->use_double_pinyin = (getenv("USE_DOUBLEPINYIN") != NULL);
#endif
    sgpycc->fullpinyin[0] = sgpycc->onscreen_preedit[0] = 0;
}

static void
ibus_sgpycc_engine_destroy(IBusSgpyccEngine *sgpycc) {

    if (sgpycc->preedit) {
        g_string_free(sgpycc->preedit, TRUE);
        sgpycc->preedit = NULL;
    }

    if (sgpycc->table) {
        g_object_unref(sgpycc->table);
        sgpycc->table = NULL;
    }

    // notify soupycc to remove from request queue before real destroy
    sogoupycc_mute_queued_requests_by_callback_para_first((void*) sgpycc, 0);

    IBUS_OBJECT_CLASS(parent_class)->destroy((IBusObject *) sgpycc);
}

static void ibus_sgpycc_engine_update_preedit(IBusSgpyccEngine *sgpycc) {

    IBusText *text;
    int fullpinyin_pos;

    if (sgpycc->use_double_pinyin) {
        int errparse, with_tail;
        char *fullpinyin_string_tmp;
        fullpinyin_string_tmp = doublepinyin_scheme_convert(NULL, sgpycc->preedit->str, sgpycc->cursor_pos, &fullpinyin_pos, &errparse, &with_tail);

        strncpy(sgpycc->fullpinyin, fullpinyin_string_tmp, IBUS_SGPYCC_FULLPINYIN_LENGTH_MAX - 1);
        free(fullpinyin_string_tmp);
        if (fullpinyin_pos > IBUS_SGPYCC_FULLPINYIN_LENGTH_MAX) fullpinyin_pos = IBUS_SGPYCC_FULLPINYIN_LENGTH_MAX;
    } else {
        strcpy(sgpycc->fullpinyin, sgpycc->preedit->str);
        fullpinyin_pos = sgpycc->cursor_pos;
    }

    char *preedit_tmp;
    int requesting, i, preedit_prefix_length, l, l_next;
    sgpycc->onscreen_preedit[0] = 0;

    sogoupycc_get_queued_request_string_begin();
    {
        for (i = 0; (preedit_tmp = sogoupycc_get_queued_request_string_by_index(sgpycc, i, &requesting)) != 0; ++i) {
            strcat(sgpycc->onscreen_preedit, preedit_tmp);
        }

        preedit_prefix_length = strlen(sgpycc->onscreen_preedit);
        strcat(sgpycc->onscreen_preedit, sgpycc->fullpinyin);

        text = ibus_text_new_from_static_string(sgpycc->onscreen_preedit);
        text->attrs = ibus_attr_list_new();

        ibus_attr_list_append(text->attrs,
                ibus_attr_underline_new(IBUS_ATTR_UNDERLINE_SINGLE, preedit_prefix_length, strlen(sgpycc->onscreen_preedit)));

        for (i = l = 0; (preedit_tmp = sogoupycc_get_queued_request_string_by_index(sgpycc, i, &requesting)) != 0; ++i) {
            l_next = l + strlen(preedit_tmp);
            if (requesting) {
                ibus_attr_list_append(text->attrs, ibus_attr_background_new(0xE9EFF8, l, l_next));
                ibus_attr_list_append(text->attrs, ibus_attr_foreground_new(0xC2C2C2, l, l_next));
            }
            l = l_next;
        }
    }
    sogoupycc_get_queued_request_string_end();

    ibus_engine_update_preedit_text((IBusEngine *) sgpycc,
            text,
            fullpinyin_pos + preedit_prefix_length,
            TRUE);
    // remember to unref text
    g_object_unref(text);
}

/**
 * commit preedit to client and update preedit
 * @return TRUE if no other handle func should be called
 */
static gboolean
ibus_sgpycc_engine_commit_preedit(IBusSgpyccEngine *sgpycc) {
    // just send into queue

    if (sgpycc->preedit->len == 0) return FALSE;

    if (sgpycc->use_double_pinyin) {
        sogoupycc_commit_request(sgpycc->fullpinyin, &ibus_sgpycc_callback_commit_string_func, (void*) sgpycc);
    } else {
        sogoupycc_commit_request(sgpycc->preedit->str, &ibus_sgpycc_callback_commit_string_func, (void*) sgpycc);
    }

    // cleaning
    g_string_assign(sgpycc->preedit, "");
    sgpycc->cursor_pos = 0;

    ibus_sgpycc_engine_update(sgpycc);

    return TRUE;
}

static void ibus_sgpycc_engine_commit_string(IBusSgpyccEngine *sgpycc, const gchar *string) {
    IBusText *text;
    text = ibus_text_new_from_static_string(string);
    ibus_engine_commit_text((IBusEngine *) sgpycc, text);
    g_object_unref(text);
}

static void ibus_sgpycc_engine_update(IBusSgpyccEngine *sgpycc) {
    ibus_sgpycc_engine_update_preedit(sgpycc);
}

#define is_alpha(c) (((c) >= IBUS_a && (c) <= IBUS_z)||((c) >= IBUS_A && (c) <= IBUS_Z))

static int _is_valid_doublepinyin_string(GString *doublepinyin_string) {
    int errparse;
    free(doublepinyin_scheme_convert(NULL, doublepinyin_string->str, 0, 0, &errparse, 0));
    return (errparse == 0);
}

static gboolean ibus_sgpycc_engine_process_key_event(IBusEngine *engine, guint keyval, guint modifiers, guint state) {
    IBusSgpyccEngine *sgpycc = (IBusSgpyccEngine *) engine;

    if ((keyval == IBUS_Shift_R || keyval == IBUS_Shift_L) &&
            (sgpycc->last_keyvalue == IBUS_Shift_R || sgpycc->last_keyvalue == IBUS_Shift_L)) {
        // soft switch enable
        if (sgpycc->temporary_disabled == 0 && sgpycc->preedit->len > 0) {
            ibus_sgpycc_engine_commit_preedit(sgpycc);
        }
        sgpycc->last_keyvalue = 0;
        sgpycc->temporary_disabled = 1 - sgpycc->temporary_disabled;
        return TRUE;
    }
    if (keyval > 255) sgpycc->last_keyvalue = keyval;

    // state is introduced in ibus upgrade 1.1 -> 1.2
    // it seems no document is available now and state just
    //  take some value previously belong to modifiers
    // replace all state with modifiers if you want to compile at ibus 1.1
    if ((modifiers & IBUS_RELEASE_MASK) || (state & IBUS_RELEASE_MASK)) {
        return FALSE;
    }

    state &= (IBUS_CONTROL_MASK | IBUS_MOD1_MASK);
    if (state != 0) {
        if (sgpycc->preedit->len == 0 && sogoupycc_get_queued_request_count_by_callback_para_first(sgpycc) == 0)
            return FALSE; // process other events
        else
            return TRUE; // process nothing
    }

    // use for doublepinyin pre check
    GString *preview_preedit_string;
    // pending insert for doublepinyin
    static char pending_insert_char = 0;

    if (keyval >= 'A' && keyval <= 'Z') keyval += 'a' - 'z';
    if (keyval >= 'a' && keyval <= 'z') {
        if (sgpycc->temporary_disabled) {
            if (sgpycc->preedit->len > 0 || sogoupycc_get_queued_request_count_by_callback_para_first(sgpycc) > 0) {
                char buf[4];
                sprintf(buf, "%c", keyval);
                sogoupycc_commit_string(buf, &ibus_sgpycc_callback_commit_string_func, (void*) sgpycc, 1);
                ibus_sgpycc_engine_update(sgpycc);
                return TRUE;
            } else {
                return FALSE;
            }
        }
        int perform_flag;
        if (sgpycc->use_double_pinyin) {
            if (sgpycc->preedit->len / 2 == sgpycc->cursor_pos / 2) {
                // at end
                preview_preedit_string = g_string_new(sgpycc->preedit->str);
                g_string_insert_c(preview_preedit_string, sgpycc->cursor_pos, keyval);
                perform_flag = _is_valid_doublepinyin_string(preview_preedit_string);
                g_string_free(preview_preedit_string, TRUE);
                if (perform_flag) goto perform_insert_char;
            } else {
                // pending insert
                if (sgpycc->last_keyvalue >= 'a' && sgpycc->last_keyvalue <= 'z') {
                    preview_preedit_string = g_string_new(sgpycc->preedit->str);

                    g_string_insert_c(preview_preedit_string, sgpycc->cursor_pos, keyval);
                    g_string_insert_c(preview_preedit_string, sgpycc->cursor_pos, sgpycc->last_keyvalue);

                    perform_flag = _is_valid_doublepinyin_string(preview_preedit_string);
                    g_string_free(preview_preedit_string, TRUE);
                    if (perform_flag) {
                        g_string_insert_c(sgpycc->preedit, sgpycc->cursor_pos, keyval);
                        g_string_insert_c(sgpycc->preedit, sgpycc->cursor_pos, sgpycc->last_keyvalue);
                        sgpycc->cursor_pos += 2;
                        pending_insert_char = 0;
                        ibus_sgpycc_engine_update(sgpycc);
                    }
                } else {
                    sgpycc->last_keyvalue = keyval;
                }
            }
        } else {
perform_insert_char:
            g_string_insert_c(sgpycc->preedit, sgpycc->cursor_pos, keyval);

            sgpycc->cursor_pos++;
            if (sgpycc->use_double_pinyin) {
                if (sgpycc->cursor_pos == sgpycc->preedit->len - 1)
                    sgpycc->cursor_pos = sgpycc->preedit->len;
            }
            ibus_sgpycc_engine_update(sgpycc);
        }
        return TRUE;
    } else {
        sgpycc->last_keyvalue = keyval; // non-alpha keys
    }

    switch (keyval) {
        case ',':case '.':case ':':case '\n': case '\t': case '[': case ']': case '{':
        case '}':case ';': case '/': case '?': case '\\': case '`': case '-': case '+':
        case '=': case '_': case '\'': case '<': case '>': case '~': case '|':
        case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9': case '0':
        case IBUS_space: case IBUS_Return:
            if (sgpycc->preedit->len == 0 && sogoupycc_get_queued_request_count_by_callback_para_first(sgpycc) == 0)
                return FALSE;

            char buf[4];
            sprintf(buf, "%c", keyval);
            int preedit_have_content = (sgpycc->preedit->len > 0);

            if (sgpycc->preedit->len > 0) {
                ibus_sgpycc_engine_commit_preedit(sgpycc);
            }

            if (keyval != IBUS_space || !preedit_have_content) {
                sogoupycc_commit_string(buf, &ibus_sgpycc_callback_commit_string_func, (void*) sgpycc, 1);
            }

            ibus_sgpycc_engine_update(sgpycc);
            return TRUE;

        case IBUS_Escape:
            if (sgpycc->preedit->len == 0)
                return FALSE;

            // cancel current input
            g_string_assign(sgpycc->preedit, "");
            sgpycc->cursor_pos = 0;
            ibus_sgpycc_engine_update(sgpycc);
            return TRUE;

        case IBUS_Left:
            if (sgpycc->preedit->len == 0)
                return FALSE;
            if (sgpycc->cursor_pos > 0) {
                sgpycc->cursor_pos--;
                if (sgpycc->use_double_pinyin) {
                    if (((sgpycc->cursor_pos & 1) != 0) && (sgpycc->cursor_pos > 0)) sgpycc->cursor_pos--;
                }
                ibus_sgpycc_engine_update(sgpycc);
            }
            return TRUE;

        case IBUS_Right:
            if (sgpycc->preedit->len == 0)
                return FALSE;
            if (sgpycc->cursor_pos < sgpycc->preedit->len) {
                sgpycc->cursor_pos++;
                if (sgpycc->use_double_pinyin) {
                    if ((sgpycc->cursor_pos & 1) != 0 && (sgpycc->cursor_pos < sgpycc->preedit->len)) sgpycc->cursor_pos++;
                }
                ibus_sgpycc_engine_update(sgpycc);
            }
            return TRUE;

        case IBUS_Up:
            if (sgpycc->preedit->len == 0)
                return FALSE;
            if (sgpycc->cursor_pos != 0) {
                sgpycc->cursor_pos = 0;
                ibus_sgpycc_engine_update(sgpycc);
            }
            return TRUE;

        case IBUS_Down:
            if (sgpycc->preedit->len == 0)
                return FALSE;

            if (sgpycc->cursor_pos != sgpycc->preedit->len) {
                sgpycc->cursor_pos = sgpycc->preedit->len;
                ibus_sgpycc_engine_update(sgpycc);
            }

            return TRUE;

        case IBUS_BackSpace:
            // TODO: cancel requesting queue ?
            if (sgpycc->preedit->len == 0) {
                if (sogoupycc_get_queued_request_count_by_callback_para_first(sgpycc) == 0)
                    return FALSE;
                else {
                    // mute latest request
                    sogoupycc_mute_queued_requests_by_callback_para_first(sgpycc, 1);
                    ibus_sgpycc_engine_update(sgpycc);
                    return TRUE;
                }
            }

            if (sgpycc->cursor_pos > 0) {
                if (sgpycc->use_double_pinyin) {
                    preview_preedit_string = g_string_new(sgpycc->preedit->str);
                    int backspace_position = (sgpycc->cursor_pos - 1) / 2 * 2;
                    int erace_length = 2;
                    if (backspace_position + 2 > preview_preedit_string->len) erace_length = 1;
                    g_string_erase(preview_preedit_string, backspace_position, erace_length);
                    if (_is_valid_doublepinyin_string(preview_preedit_string)) {
                        sgpycc->cursor_pos = backspace_position;
                        g_string_erase(sgpycc->preedit, sgpycc->cursor_pos, erace_length);
                        ibus_sgpycc_engine_update(sgpycc);
                    }
                    g_string_free(preview_preedit_string, TRUE);
                } else {
                    sgpycc->cursor_pos--;
                    g_string_erase(sgpycc->preedit, sgpycc->cursor_pos, 1);
                    ibus_sgpycc_engine_update(sgpycc);
                }

            }
            return TRUE;

        case IBUS_Delete:
            if (sgpycc->preedit->len == 0) {
                if (sogoupycc_get_queued_request_count_by_callback_para_first(sgpycc) == 0) return FALSE;
                else return TRUE;
            }

            if (sgpycc->cursor_pos < sgpycc->preedit->len) {
                if (sgpycc->use_double_pinyin) {
                    preview_preedit_string = g_string_new(sgpycc->preedit->str);
                    int erace_length = 2;
                    if (sgpycc->cursor_pos == preview_preedit_string->len - 1 && (sgpycc->cursor_pos & 1)) erace_length = 1;

                    g_string_erase(preview_preedit_string, sgpycc->cursor_pos, erace_length);
                    if (_is_valid_doublepinyin_string(preview_preedit_string)) {
                        g_string_erase(sgpycc->preedit, sgpycc->cursor_pos, erace_length);
                        ibus_sgpycc_engine_update(sgpycc);
                    }
                    g_string_free(preview_preedit_string, TRUE);
                } else {
                    g_string_erase(sgpycc->preedit, sgpycc->cursor_pos, 1);
                    ibus_sgpycc_engine_update(sgpycc);
                }
            }
            return TRUE;
    }

    // let other program handle
    return FALSE;
}

static void ibus_sgpycc_callback_commit_string_func(void *sgpycc, const char *str) {
    ibus_sgpycc_engine_commit_string((IBusSgpyccEngine*) sgpycc, str);
    ibus_sgpycc_engine_update_preedit((IBusSgpyccEngine*) sgpycc);
}
