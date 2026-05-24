#include "ooNmeaRecorder.h"

#include <wx/datetime.h>
#include <wx/filefn.h>
#include <wx/filename.h>

static wxString EnsureTrailingPathSeparator(const wxString& path)
{
    if (path.IsEmpty()) return path;

    wxString result = path;
    const wxChar sep = wxFileName::GetPathSeparator();

    if (result.Last() != sep) {
        result.Append(sep);
    }

    return result;
}

ooNmeaRecorder::ooNmeaRecorder()
    : m_tempFilePath(wxEmptyString),
      m_finalFilePath(wxEmptyString),
      m_isRecording(false),
      m_hasRecordedData(false)
{
}

ooNmeaRecorder::~ooNmeaRecorder()
{
    StopRecording();
}

bool ooNmeaRecorder::StartRecording(const wxString& openObserverPrivateDataDir)
{
    if (m_isRecording) {
        StopRecording();
    }

    if (openObserverPrivateDataDir.IsEmpty()) {
        return false;
    }

    const wxDateTime nowUtc = wxDateTime::UNow().ToUTC();

    const wxString dayFolderName = nowUtc.Format("%Y-%m-%d");
    const wxString fileBaseName = nowUtc.Format("NMEA-%Y%m%d-%H%M%S");

    wxString recordingsRoot = EnsureTrailingPathSeparator(openObserverPrivateDataDir);
    recordingsRoot.Append("NMEArecordings");
    recordingsRoot = EnsureTrailingPathSeparator(recordingsRoot);

    wxString dayFolder = recordingsRoot;
    dayFolder.Append(dayFolderName);
    dayFolder = EnsureTrailingPathSeparator(dayFolder);

    if (!wxDirExists(dayFolder)) {
        if (!wxFileName::Mkdir(dayFolder, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL)) {
            return false;
        }
    }

    wxString finalPath = dayFolder + fileBaseName + ".nmea";
    wxString tempPath = dayFolder + fileBaseName + ".recording";

    int suffix = 1;
    while (wxFileExists(finalPath) || wxFileExists(tempPath)) {
        finalPath = dayFolder + fileBaseName +
                    wxString::Format("_%d.nmea", suffix);
        tempPath = dayFolder + fileBaseName +
                   wxString::Format("_%d.recording", suffix);
        suffix++;
    }

    if (!m_file.Open(tempPath, "w")) {
        return false;
    }

    m_tempFilePath = tempPath;
    m_finalFilePath = finalPath;
    m_isRecording = true;
    m_hasRecordedData = false;

    return true;
}

wxString ooNmeaRecorder::StopRecording()
{
    const wxString tempPath = m_tempFilePath;
    const wxString finalPath = m_finalFilePath;

    if (m_file.IsOpened()) {
        m_file.Flush();
        m_file.Close();
    }

    m_isRecording = false;

    if (!m_hasRecordedData) {
        if (!tempPath.IsEmpty() && wxFileExists(tempPath)) {
            wxRemoveFile(tempPath);
        }

        m_tempFilePath = wxEmptyString;
        m_finalFilePath = wxEmptyString;
        return wxEmptyString;
    }

    if (!tempPath.IsEmpty() && !finalPath.IsEmpty()) {
        if (wxFileExists(finalPath)) {
            wxRemoveFile(finalPath);
        }

        if (wxRenameFile(tempPath, finalPath)) {
            m_tempFilePath = wxEmptyString;
            m_finalFilePath = finalPath;
            return finalPath;
        }
    }

    return wxEmptyString;
}

void ooNmeaRecorder::WriteSentence(const wxString& sentence)
{
    if (!m_isRecording || !m_file.IsOpened()) {
        return;
    }

    if (sentence.IsEmpty()) {
        return;
    }

    wxString line = sentence;

    if (!line.EndsWith("\n")) {
        line.Append("\n");
    }

    m_file.Write(line, wxConvUTF8);
    m_hasRecordedData = true;
}

bool ooNmeaRecorder::IsRecording() const
{
    return m_isRecording;
}

bool ooNmeaRecorder::HasRecordedData() const
{
    return m_hasRecordedData;
}

wxString ooNmeaRecorder::GetCurrentFilePath() const
{
    if (m_isRecording) {
        return m_tempFilePath;
    }

    return m_finalFilePath;
}