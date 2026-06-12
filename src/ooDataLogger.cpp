#include "ooDataLogger.h"

#include <wx/defs.h>

ooDataLogger::ooDataLogger()
    : m_loggerProjectIndex(wxNOT_FOUND),
      m_loggerProjectName(wxEmptyString),
      m_intervalSeconds(1800),
      m_captureDurationSeconds(0),
      m_running(false)
{
}

void ooDataLogger::ClearLoggerProject()
{
    m_loggerProjectIndex = wxNOT_FOUND;
    m_loggerProjectName.Clear();
    m_running = false;
}

void ooDataLogger::SetLoggerProject(int projectIndex, const wxString& projectName)
{
    if (m_loggerProjectIndex != projectIndex) {
        m_running = false;
    }

    m_loggerProjectIndex = projectIndex;
    m_loggerProjectName = projectName;
}

bool ooDataLogger::HasLoggerProject() const
{
    return m_loggerProjectIndex != wxNOT_FOUND && !m_loggerProjectName.IsEmpty();
}

bool ooDataLogger::IsLoggerProject(int projectIndex) const
{
    return HasLoggerProject() && m_loggerProjectIndex == projectIndex;
}

int ooDataLogger::GetLoggerProjectIndex() const
{
    return m_loggerProjectIndex;
}

const wxString& ooDataLogger::GetLoggerProjectName() const
{
    return m_loggerProjectName;
}

void ooDataLogger::SetIntervalSeconds(long seconds)
{
    m_intervalSeconds = seconds < 0 ? 0 : seconds;
}

long ooDataLogger::GetIntervalSeconds() const
{
    return m_intervalSeconds;
}

void ooDataLogger::SetCaptureDurationSeconds(long seconds)
{
    m_captureDurationSeconds = seconds < 0 ? 0 : seconds;
}

long ooDataLogger::GetCaptureDurationSeconds() const
{
    return m_captureDurationSeconds;
}

void ooDataLogger::Start()
{
    if (HasLoggerProject()) {
        m_running = true;
    }
}

void ooDataLogger::Stop()
{
    m_running = false;
}

void ooDataLogger::Toggle()
{
    if (m_running) {
        Stop();
    } else {
        Start();
    }
}

bool ooDataLogger::IsRunning() const
{
    return m_running;
}

wxString ooDataLogger::GetStatusText() const
{
    if (!IsRunning() || !HasLoggerProject()) {
        return wxEmptyString;
    }

    return wxString::Format("Logger ON: %s", m_loggerProjectName);
}
