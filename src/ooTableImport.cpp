#include "ooTableImport.h"

#include "ooObservations.h"
#include "ooProjectFieldMapper.h"

#include <algorithm>
#include <set>
#include <vector>
#include <wx/textfile.h>

static wxString NormalizeHeaderName(wxString value)
{
    value.MakeLower();
    value.Trim(true);
    value.Trim(false);

    value.Replace("_", " ");
    value.Replace("-", " ");
    value.Replace(".", " ");
    value.Replace("/", " ");

    while (value.Contains("  ")) {
        value.Replace("  ", " ");
    }

    return value;
}

static int FindProjectColumnByLabel(
    const ooProject& project,
    const wxString& header,
    const std::set<int>& usedColumns)
{
    const wxString normalizedHeader = NormalizeHeaderName(header);

    for (int c = 0; c < project.GetColCount(); ++c) {
        if (usedColumns.find(c) != usedColumns.end()) {
            continue;
        }

        if (NormalizeHeaderName(project.GetColLabels()[c]).IsSameAs(normalizedHeader)) {
            return c;
        }
    }

    return wxNOT_FOUND;
}

static int FindProjectColumnByFieldType(
    const ooProject& project,
    const wxString& fieldType,
    const std::set<int>& usedColumns)
{
    if (ooObservations::IsInternalObservationFieldType(fieldType)) {
        return wxNOT_FOUND;
    }

    for (int c = 0; c < project.GetColCount(); ++c) {
        if (usedColumns.find(c) != usedColumns.end()) {
            continue;
        }

        if (project.GetColFieldTypes()[c].IsSameAs(fieldType)) {
            return c;
        }
    }

    return wxNOT_FOUND;
}

bool ooTableImport::ReadCsvHeaders(
    const wxString& filePath,
    ooImportedTable& table,
    wxString& errorMessage)
{
    table.headers.Clear();
    table.rowCount = 0;

    if (filePath.IsEmpty()) {
        errorMessage = "No CSV file selected.";
        return false;
    }

    wxTextFile file(filePath);
    if (!file.Open()) {
        errorMessage = "Unable to open CSV file.";
        return false;
    }

    if (file.GetLineCount() == 0) {
        errorMessage = "CSV file is empty.";
        file.Close();
        return false;
    }

    table.headers = ParseCsvLine(file.GetLine(0));
    table.rowCount = static_cast<int>(file.GetLineCount()) - 1;

    file.Close();

    if (table.headers.IsEmpty()) {
        errorMessage = "CSV file does not contain headers.";
        return false;
    }

    return true;
}

bool ooTableImport::ImportCsvRowsByHeaderMapping(
    const wxString& filePath,
    ooObservations* observations,
    const wxArrayString& availableFieldTypes,
    int& rowsImported,
    wxString& errorMessage)
{
    rowsImported = 0;

    if (!observations) {
        errorMessage = "No active observations table.";
        return false;
    }

    wxTextFile file(filePath);
    if (!file.Open()) {
        errorMessage = "Unable to open CSV file.";
        return false;
    }

    if (file.GetLineCount() < 2) {
        file.Close();
        errorMessage = "CSV file does not contain data rows.";
        return false;
    }

    const wxArrayString headers = ParseCsvLine(file.GetLine(0));
    const ooProject& project = observations->GetProject();

    std::set<int> usedDestinationColumns;
    std::vector<int> sourceToDestination(headers.GetCount(), wxNOT_FOUND);

    for (size_t sourceCol = 0; sourceCol < headers.GetCount(); ++sourceCol) {
        wxString header = headers[sourceCol];
        header.Trim(true);
        header.Trim(false);

        if (header.IsEmpty()) {
            continue;
        }

        int destinationCol =
            FindProjectColumnByLabel(project, header, usedDestinationColumns);

        if (destinationCol == wxNOT_FOUND) {
            const wxString suggestedFieldType =
                ooProjectFieldMapper::SuggestFieldTypeForColumn(header, availableFieldTypes);

            destinationCol =
                FindProjectColumnByFieldType(project, suggestedFieldType, usedDestinationColumns);
        }

        if (destinationCol != wxNOT_FOUND) {
            sourceToDestination[sourceCol] = destinationCol;
            usedDestinationColumns.insert(destinationCol);
        }
    }

    if (usedDestinationColumns.empty()) {
        file.Close();
        errorMessage = "No CSV columns could be matched to the current project.";
        return false;
    }

    for (size_t lineIndex = 1; lineIndex < file.GetLineCount(); ++lineIndex) {
        const wxArrayString cells = ParseCsvLine(file.GetLine(lineIndex));

        observations->AppendRows(1);
        const int row = observations->GetNumberRows() - 1;

        const size_t count = std::min(cells.GetCount(), sourceToDestination.size());

        for (size_t sourceCol = 0; sourceCol < count; ++sourceCol) {
            const int destinationCol = sourceToDestination[sourceCol];

            if (destinationCol == wxNOT_FOUND) {
                continue;
            }

            observations->SetValue(row, destinationCol, cells[sourceCol]);
        }

        ++rowsImported;
    }

    file.Close();
    return true;
}

wxArrayString ooTableImport::ParseCsvLine(const wxString& line)
{
    wxArrayString cells;
    wxString current;
    bool inQuotes = false;

    for (size_t i = 0; i < line.Length(); ++i) {
        const wxChar ch = line[i];

        if (ch == '"') {
            if (inQuotes && i + 1 < line.Length() && line[i + 1] == '"') {
                current += '"';
                ++i;
            } else {
                inQuotes = !inQuotes;
            }
        } else if (ch == ',' && !inQuotes) {
            current.Trim(true);
            current.Trim(false);
            cells.Add(current);
            current.Clear();
        } else {
            current += ch;
        }
    }

    current.Trim(true);
    current.Trim(false);
    cells.Add(current);

    return cells;
}