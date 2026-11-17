#define main zero_watt_main
#include "../src/main.cpp"
#undef main
#include <SDL.h>
#include <atomic>
int main(int argc,char **) {
    std::atomic<bool> done{false},early{false};
    std::thread events([&] {
        std::this_thread::sleep_for(std::chrono::milliseconds(250));
        if(argc>1) {
            SDL_Event event{}; event.type=SDL_QUIT; SDL_PushEvent(&event); return;
        }
        auto key=[](Uint32 type,SDL_Keycode code) {
            SDL_Event event{}; event.type=type; event.key.keysym.sym=code; SDL_PushEvent(&event);
        };
        key(SDL_KEYDOWN,SDLK_ESCAPE); key(SDL_KEYUP,SDLK_ESCAPE);
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        early=done.load();
        key(SDL_KEYDOWN,SDLK_ESCAPE);
        std::this_thread::sleep_for(std::chrono::milliseconds(1300));
        if(!done) { SDL_Event event{}; event.type=SDL_QUIT; SDL_PushEvent(&event); }
    });
    char name[]="zero-watt", demo[]="--demo"; char *args[]={name,demo};
    auto begin=Clock::now();
    int result=zero_watt_main(2,args); done=true;
    auto elapsed=std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now()-begin).count();
    events.join();
    if(result || early || (argc==1 && (elapsed<1300 || elapsed>1700))) return 1;
    return 0;
}
