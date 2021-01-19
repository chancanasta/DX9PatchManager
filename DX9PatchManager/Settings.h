#ifndef __DX9SETTINGS_H
#define __DX9SETTINGS_H

void SetUpConfig();
void LoadSettings();
void SaveSettings();
void ShowSettings(HWND hMainWindow);
void SaveSettingValues(HWND hDLg);
void SettingsCheckBox(HWND item, BOOL value);
BOOL RetValCheckBox(HWND item);
BOOL ReadBoolSetting(LPCWSTR section, LPCWSTR item, BOOL default);
void WriteBoolSetting(BOOL value, LPCWSTR section, LPCWSTR item);

void WriteIntSetting(int value, LPCWSTR section, LPCWSTR item);

INT_PTR CALLBACK SettingsDialog(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);


#endif