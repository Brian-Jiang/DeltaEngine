#include "Editor/EditorWindows/EditorDetailsPropertyFormatting.h"

#include <cctype>

DELTA_ENGINE_NS_BEGIN

std::string FormatPropertyInspectorLabel(const std::string& propName)
{
    if (propName.empty())
        return {};

    std::string name = propName;
    if (name.size() >= 2 && name[0] == 'm' && name[1] == '_')
        name.erase(0, 2);

    if (name.empty())
        return {};

    std::string result;
    result.reserve(name.size() + 8);
    bool prevUpper = false;
    bool prevLower = false;

    for (size_t i = 0; i < name.size(); ++i)
    {
        const char c = name[i];
        const bool isUpper = std::isupper(static_cast<unsigned char>(c));
        const bool isLower = std::islower(static_cast<unsigned char>(c));

        if (i == 0)
        {
            result += static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        }
        else if (isUpper)
        {
            if (prevLower || (!prevUpper && std::isdigit(static_cast<unsigned char>(name[i - 1]))))
                result += ' ';
            result += c;
        }
        else
        {
            result += c;
        }

        prevUpper = isUpper;
        prevLower = isLower;
    }

    return result;
}

bool IsUndoableInspectorPropertyType(EPropertyType type)
{
    switch (type)
    {
    case EPropertyType::Float:
    case EPropertyType::Int:
    case EPropertyType::Bool:
    case EPropertyType::Double:
    case EPropertyType::String:
    case EPropertyType::FilesystemPath:
    case EPropertyType::Vector3:
    case EPropertyType::Quaternion:
    case EPropertyType::Float4:
    case EPropertyType::Float4x4:
    case EPropertyType::Struct:
        return true;
    case EPropertyType::WString:
    case EPropertyType::ObjectPtr:
    case EPropertyType::BulkData:
    case EPropertyType::Vector:
        return false;
    default:
        DELTA_CHECK_MSG(false,
            "EPropertyType {} not enumerated for undo inspector (compiler should catch stale switch)",
            static_cast<int>(type));
        return false;
    }
}

DELTA_ENGINE_NS_END
