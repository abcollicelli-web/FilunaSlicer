#!/usr/bin/env python3
"""Source integration guard, not a substitute for full Windows GUI build."""
from pathlib import Path
root = Path(__file__).resolve().parents[2]
main = (root / "src/slic3r/GUI/MainFrame.cpp").read_text(encoding="utf-8")
app = (root / "src/slic3r/GUI/GUI_App.cpp").read_text(encoding="utf-8")
start = main.index("wxMenu* MainFrame::generate_help_menu()")
end = main.index("return helpMenu;", start)
menu = main[start:end]
assert 'wxString::FromUTF8("Atualizações Filuna")' in menu
assert "check_new_version_sf(true, 1)" not in menu
assert "manifesto" in menu and "assinatura" in menu
startup = app[app.index("if (this->preset_updater)"):]
startup = startup[:startup.index("if (!cw_showed)")]
assert "this->check_new_version_sf();" not in startup
assert "filuna" in startup.lower()
assert "void GUI_App::check_new_version_sf" in app
print("FILUNA_GUI_UPDATE_SOURCE_GUARD_PASS")
