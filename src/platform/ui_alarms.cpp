#include "ui_render.hpp"
using namespace std::chrono;

// ─── Alarms ──────────────────────────────────────────────────────────────────

void render_alarms(App& app, UiState& ui) {
    if (!app.show_alarms) return;
    if (ImGui::BeginTable("##alarm_header", 2,
                          ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_NoSavedSettings)) {
        ImGui::TableSetupColumn("title", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("action", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::TextUnformatted("Alarms");
        ImGui::TableSetColumnIndex(1);
        if (ImGui::SmallButton("+ Add") && (int)app.alarms.size() < ALARM_MAX_COUNT) {
            memset(ui.alarm_name, 0, sizeof(ui.alarm_name));
            ui.alarm_hour = 8; ui.alarm_minute = 0;
            ui.alarm_days_mode = true;
            for (int d = 0; d < 7; ++d) ui.alarm_days[d] = true;
            {
                tm lt = local_tm(std::time(nullptr));
                ui.alarm_year = lt.tm_year + 1900;
                ui.alarm_month = lt.tm_mon + 1;
                ui.alarm_day = lt.tm_mday;
            }
            ui.show_add_alarm = true;
        }
        CHRONOS_DEBUG_ITEM("add alarm", IM_COL32(180, 120, 255, 255));
        ImGui::EndTable();
    }

    if (app.alarms.empty()) {
        ImGui::TextDisabled("No alarms set");
    } else if (ImGui::BeginTable("##alarm_rows", 4,
                                 ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_NoSavedSettings)) {
        ImGui::TableSetupColumn("enabled", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("name", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("time", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("action", ImGuiTableColumnFlags_WidthFixed);
        for (int i = 0; i < (int)app.alarms.size(); ++i) {
            auto& a = app.alarms[i];
            ImGui::PushID(i);
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            bool en = a.enabled;
            if (ImGui::Checkbox("##en", &en)) {
                auto r = dispatch_action(app, A_ALARM_TOGGLE + i, steady_clock::now(), {});
                if (r.save_config) ui.dirty = true;
            }
            CHRONOS_DEBUG_ITEM("alarm enabled", IM_COL32(180, 120, 255, 255));
            ImGui::TableSetColumnIndex(1);
            ImGui::TextUnformatted(a.name.empty() ? "(unnamed)" : a.name.c_str());
            ImGui::TableSetColumnIndex(2);
            ImGui::Text("%02d:%02d", a.hour, a.minute);
            ImGui::TableSetColumnIndex(3);
            if (ImGui::SmallButton("Del")) {
                auto r = dispatch_action(app, A_ALARM_DEL + i, steady_clock::now(), {});
                if (r.save_config) ui.dirty = true;
                ImGui::PopID();
                break;
            }
            CHRONOS_DEBUG_ITEM("alarm delete", IM_COL32(180, 120, 255, 255));
            ImGui::PopID();
        }
        ImGui::EndTable();
    }
}

// ─── Add-alarm window ────────────────────────────────────────────────────────

void render_add_alarm_window(App& app, UiState& ui) {
    if (!ui.show_add_alarm) return;

    ImGui::TextUnformatted("Add Alarm");
    ImGui::Separator();

    ImGui::InputText("Name", ui.alarm_name, sizeof(ui.alarm_name));
    CHRONOS_DEBUG_ITEM("alarm name", IM_COL32(180, 120, 255, 255));

    {
        const float btn  = ImGui::GetFrameHeight();
        const float sp   = ImGui::GetStyle().ItemSpacing.x;
        const float numw = ImGui::CalcTextSize("00").x + sp * 2.f;
        const float colw = ImGui::CalcTextSize(":").x;
        const float total = btn * 4.f + numw * 2.f + colw + sp * 6.f;
        const float avail = ImGui::GetContentRegionAvail().x;
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (avail - total) * 0.5f);

        if (ImGui::Button("-##h", {btn, btn})) ui.alarm_hour = (ui.alarm_hour + 23) % 24;
        ImGui::SameLine();
        ImGui::Text("%02d", ui.alarm_hour);
        ImGui::SameLine();
        if (ImGui::Button("+##h", {btn, btn})) ui.alarm_hour = (ui.alarm_hour + 1) % 24;
        ImGui::SameLine();
        ImGui::Text(":");
        ImGui::SameLine();
        if (ImGui::Button("-##m", {btn, btn})) ui.alarm_minute = (ui.alarm_minute + 59) % 60;
        ImGui::SameLine();
        ImGui::Text("%02d", ui.alarm_minute);
        ImGui::SameLine();
        if (ImGui::Button("+##m", {btn, btn})) ui.alarm_minute = (ui.alarm_minute + 1) % 60;
    }
    CHRONOS_DEBUG_ITEM("alarm time", IM_COL32(180, 120, 255, 255));

    {
        int mode = ui.alarm_days_mode ? 1 : 0;
        ImGui::RadioButton("Days of week", &mode, 1);
        CHRONOS_DEBUG_ITEM("days mode", IM_COL32(180, 120, 255, 255));
        ImGui::SameLine();
        ImGui::RadioButton("Specific date", &mode, 0);
        CHRONOS_DEBUG_ITEM("date mode", IM_COL32(180, 120, 255, 255));
        ui.alarm_days_mode = (mode != 0);
    }

    if (ui.alarm_days_mode) {
        const char* day_names[] = {"Mon","Tue","Wed","Thu","Fri","Sat","Sun"};
        for (int d = 0; d < 7; ++d) {
            ImGui::Checkbox(day_names[d], &ui.alarm_days[d]);
            CHRONOS_DEBUG_ITEM(day_names[d], IM_COL32(180, 120, 255, 255));
            if (d < 6) ImGui::SameLine();
        }
    } else {
        ImGui::SetNextItemWidth(70); ImGui::InputInt("Year",  &ui.alarm_year,  1);
        CHRONOS_DEBUG_ITEM("alarm year", IM_COL32(180, 120, 255, 255));
        ImGui::SameLine();
        ImGui::SetNextItemWidth(50); ImGui::InputInt("Month", &ui.alarm_month, 1);
        CHRONOS_DEBUG_ITEM("alarm month", IM_COL32(180, 120, 255, 255));
        ImGui::SameLine();
        ImGui::SetNextItemWidth(50); ImGui::InputInt("Day",   &ui.alarm_day,   1);
        CHRONOS_DEBUG_ITEM("alarm day", IM_COL32(180, 120, 255, 255));
        ui.alarm_month = std::clamp(ui.alarm_month, 1, 12);
        ui.alarm_day   = std::clamp(ui.alarm_day,   1, 31);
    }

    ImGui::Separator();
    if (ImGui::Button("OK")) {
        Alarm a;
        a.name    = ui.alarm_name;
        a.hour    = ui.alarm_hour;
        a.minute  = ui.alarm_minute;
        a.enabled = true;
        if (ui.alarm_days_mode) {
            a.schedule  = AlarmSchedule::Days;
            a.days_mask = 0;
            for (int d = 0; d < 7; ++d)
                if (ui.alarm_days[d]) a.days_mask |= (1 << d);
        } else {
            a.schedule    = AlarmSchedule::Date;
            a.date_year   = ui.alarm_year;
            a.date_month  = ui.alarm_month;
            a.date_day    = ui.alarm_day;
        }
        app.alarms.push_back(a);
        ui.dirty = true;
        ui.show_add_alarm = false;
    }
    CHRONOS_DEBUG_ITEM("alarm ok", IM_COL32(180, 120, 255, 255));
    ImGui::SameLine();
    if (ImGui::Button("Cancel")) ui.show_add_alarm = false;
    CHRONOS_DEBUG_ITEM("alarm cancel", IM_COL32(180, 120, 255, 255));
}
