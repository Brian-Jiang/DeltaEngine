#include "UIComponents/EditorInlineRename.h"

#include "UIComponents/UIComponentsEditorTheme.h"

#include "imgui.h"

#include <cctype>
#include <cstring>

using namespace DeltaEngine;

namespace
{
bool IsBlank(const char* s)
{
    if (!s || !*s)
        return true;
    for (const char* p = s; *p; ++p)
    {
        if (!std::isspace(static_cast<unsigned char>(*p)))
            return false;
    }
    return true;
}
}

void EditorInlineRename::Begin(const std::string& initialName)
{
    m_active       = true;
    m_requestFocus = true;
    const size_t n = std::min(initialName.size(), sizeof(m_buf) - 1u);
    std::memcpy(m_buf, initialName.data(), n);
    m_buf[n] = '\0';
}

void EditorInlineRename::Clear()
{
    m_active       = false;
    m_requestFocus = false;
    m_buf[0]       = '\0';
}

EditorInlineRename::Result EditorInlineRename::Draw()
{
    if (!m_active)
        return Result::None;

    if (!ImGui::GetCurrentContext())
    {
        DLOG(LogUIComponents, ELogLevel::Warning,
            "EditorInlineRename::Draw: no active ImGui context (call after CreateContext/NewFrame)");
        return Result::None;
    }

    if (ImGui::IsKeyPressed(ImGuiKey_Escape))
    {
        Clear();
        return Result::Cancelled;
    }

    if (m_requestFocus)
    {
        ImGui::SetKeyboardFocusHere();
        m_requestFocus = false;
    }

    const bool enter = ImGui::InputText(
        "##inlineRename",
        m_buf,
        sizeof(m_buf),
        ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_EnterReturnsTrue);

    if (enter)
    {
        if (IsBlank(m_buf))
        {
            Clear();
            return Result::Cancelled;
        }
        m_active       = false;
        m_requestFocus = false;
        return Result::Committed;
    }

    if (ImGui::IsItemDeactivated())
    {
        if (IsBlank(m_buf))
        {
            Clear();
            return Result::Cancelled;
        }
        m_active       = false;
        m_requestFocus = false;
        return Result::Committed;
    }

    return Result::None;
}
