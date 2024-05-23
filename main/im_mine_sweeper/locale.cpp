#include "locale.h"
namespace lang {
    const char* locale;
    const char* locale_strs;
    const char* title;
    const char* game_disc;
    const char* over;
    const char* clear;
    const char* auto_btn1;
    const char* auto_btn1_disc;
    const char* auto_btn2;
    const char* auto_btn2_disc;
    const char* retry;
    const char* levels;
    const char* level;
    const char* thanks;
    const char* programmer;
    const char* setting;
    const char* board;
    const char* state;
    const char* sec_count;
}

void lang::set(LOCALE id) {
    switch( id ) {
    case LC_KOR:
        locale          = "언어";
        locale_strs     = "한국어\0영어\0";
        title           = "아임마인즈";
        game_disc       = "좌클릭: 열기, 우클릭: 깃발\n하나 열어서 시작\n칸의 숫자는 주변 지뢰 개수\n지뢰를 제외한 모든 칸을 열면 클리어\n우클릭으로 추가 설정 열기\n";
        over            = "게임 오버";
        clear           = "클리어!";
        auto_btn1       = "자동1";
        auto_btn1_disc  = "열린칸마다 주변 지뢰개수와 깃발개수를 비교해서 주변 닫힌칸을 모두 열거나 깃발을 세운다. 정확성 보장됨.";
        auto_btn2       = "자동2";
        auto_btn2_disc  = "열린칸마다 닫힌칸에 지뢰일 확률을 더하고 가장 높은 확률의 닫힌칸 하나를 열거나 깃발을 세운다. 정확성 보장되지 않음.";
        retry           = "\uE810 다시시작";
        levels          = "초급\0중급\0고급\0커스텀\0";
        level           = "레벨";
        thanks          = "플레이해 주셔서 감사합니다.";
        programmer      = "프로그래머: 임동예 ( imdongye@naver.com )";
        setting         = "설정##mines";
        board           = "지뢰밭##mines";
        state           = "게임중##mines";
        sec_count       = "%.3f 초";
        break;
    case LC_ENG:
        locale          = "Language";
        locale_strs     = "korean\0english\0";
        title           = "immines";
        game_disc       = "Left-click: open, right-click: flag\nOpen one to start\nNumber in cell is the number of nearby mines\nOpen all squares except mines to clear\nRight-click to open more settings\n";
        over            = "Game Over";
        clear           = "Clear!";
        auto_btn1       = "Auto1";
        auto_btn1_disc  = "For each open cell, compare the number of nearby mines to the number of flags and open or flag all nearby closed cells. Accuracy guaranteed.";
        auto_btn2       = "Auto2";
        auto_btn2_disc  = "For each open cell, add the probability that the closed cell is a mine and open or flag the one closed square with the highest probability. Accuracy not guaranteed.";
        retry           = "\uE810 Retry";
        levels          = "Beginner\0Intermediate\0Advanced\0Custom\0";
        level           = "Level";
        thanks          = "Thank you for playing";
        programmer      = "programmer: 임동예 ( imdongye@naver.com )";
        setting         = "settings##mines";
        board           = "board##mines";
        state           = "state##mines";
        sec_count       = "%.3f sec";
        break;
    }
}