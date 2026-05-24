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
    : m_currentFilePath(wxEmptyString),
      m_isRecording(false)
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

    wxString candidatePath = dayFolder + fileBaseName + ".nmea";

    int suffix = 1;
    while (wxFileExists(candidatePath)) {
        candidatePath = dayFolder + fileBaseName +
                        wxString::Format("_%d.nmea", suffix);
        suffix++;
    }

    if (!m_file.Open(candidatePath, "w")) {
        return false;
    }

    m_currentFilePath = candidatePath;
    m_isRecording = true;

    return true;
}

void ooNmeaRecorder::StopRecording()
{
    if (m_file.IsOpened()) {
        m_file.Flush();
        m_file.Close();
    }

    m_isRecording = false;
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
}

bool ooNmeaRecorder::IsRecording() const
{
    return m_isRecording;
}

wxString ooNmeaRecorder::GetCurrentFilePath() const
{
    return m_currentFilePath;
}