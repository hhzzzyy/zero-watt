#define main zero_watt_main
#include "../src/main.cpp"
#undef main
#include <cassert>
int main(){
 InputState s;
 auto press=[&](int k){command(s,k);process_commands(s,nullptr,10000,8000);};
 press(ShowHelp);assert(s.help);press(ShowHelp);assert(!s.help);
 press(Enter);assert(!s.help);
 press(ToggleBackground);assert(s.settings.background);
 press(Escape);assert(s.settings.background && !s.help);
 press(SetTime);assert(s.editing);
 s.entry="";press(Enter);assert(s.editing && !s.error.empty());
 press('6');press('0');press(Enter);assert(!s.editing && s.settings.minutes==60);
 press(SetTime);press(Backspace);press(Escape);assert(s.settings.minutes==60);
 press(ShowGraph);assert(s.history_page && s.follow);
 s.view_end=10000;press(Up);assert(s.span==1800 && !s.follow);
 press(Left);assert(s.view_end<10000);
 press(Enter);assert(s.follow);
 press(Escape);assert(!s.history_page);
 press(ToggleBackground);assert(!s.settings.background);
 return 0;
}
