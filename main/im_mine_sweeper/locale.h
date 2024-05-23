#ifndef __locale_h_
#define __locale_h_

namespace lang {
    enum LOCALE {
        LC_KOR,
        LC_ENG,
    };
    void set(LOCALE id);

    extern const char* locale;
    extern const char* locale_strs;
    extern const char* title;
    extern const char* game_disc;
    extern const char* over;
    extern const char* clear;
    extern const char* auto_btn1;
    extern const char* auto_btn1_disc;
    extern const char* auto_btn2;
    extern const char* auto_btn2_disc;
    extern const char* retry;
    extern const char* levels;
    extern const char* level;
    extern const char* thanks;
    extern const char* programmer;
    extern const char* setting;
    extern const char* board;
    extern const char* state;
    extern const char* sec_count;
}

#endif