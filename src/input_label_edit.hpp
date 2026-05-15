#pragma once
#include <windows.h>
#include "wndstate.hpp"

constexpr int EDIT_ID_BASE = 9000;

LRESULT CALLBACK EditSubclassProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
void try_start_label_edit(HWND hwnd, POINT pt, WndState& s);
bool handle_label_edit_command(HWND hwnd, WPARAM wp, LPARAM lp, WndState& s);
