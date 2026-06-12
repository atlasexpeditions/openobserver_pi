#pragma once

#include <wx/string.h>

class ooDataLogger
{
public:
    ooDataLogger();

    void ClearLoggerProject();
    void SetLoggerProject(int projectIndex, const wxString& projectName);

    bool HasLoggerProject() const;
    bool IsLoggerProject(int projectIndex) const;

    int GetLoggerProjectIndex() const;
    const wxString& GetLoggerProjectName() const;

    void SetIntervalSeconds(long seconds);
    long GetIntervalSeconds() const;

    void SetCaptureDurationSeconds(long seconds);
    long GetCaptureDurationSeconds() const;

    void Start();
    void Stop();
    void Toggle();

    bool IsRunning() const;
    wxString GetStatusText() const;

private:
    int m_loggerProjectIndex;
    wxString m_loggerProjectName;
    long m_intervalSeconds;
    long m_captureDurationSeconds;
    bool m_running;
};
