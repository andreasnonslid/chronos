#include "ui.hpp"

#ifdef _WIN32
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int nShow) {
    return ui::run_app(hInst, nShow);
}
#else
int main(int argc, char** argv) {
    return ui::run_app(ui::InstanceHandle{.argc = argc, .argv = argv}, 0);
}
#endif
