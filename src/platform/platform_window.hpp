#pragma once

struct SDL_Window;

void platform_window_init(SDL_Window* window);
void platform_window_shutdown(SDL_Window* window);
void platform_set_always_on_top(SDL_Window* window, bool topmost);
void platform_begin_window_drag(SDL_Window* window);
