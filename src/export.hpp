#pragma once
#include <string>

#include "analyzer.hpp"
#include "ui.hpp"

// Asks for a file name with the standard Windows "Save as" dialog.
// Returns an empty string if the user cancels.
std::wstring AskSavePath(const wchar_t* defaultName, const wchar_t* filter, const wchar_t* defaultExt);

// Text report with the results of every tab.
bool ExportReport(const Analyzer& a, const UiState& s, const std::wstring& path);

// Raw polling intervals as CSV (index, interval in µs), for comparison with other tools.
bool ExportIntervalsCsv(const Analyzer& a, const std::wstring& path);
