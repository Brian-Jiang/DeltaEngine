#pragma once

#include "EngineIncludes.h"

#include <memory>
#include <string>

#include "Core/DObject.h"

#include "HeaderToolRegression.generated.h"

DELTA_ENGINE_NS_BEGIN

class UnreflectedBase
{
public:
    virtual ~UnreflectedBase() = default;
};

DCLASS()
class HeaderToolRegression : public UnreflectedBase, public DObject, public std::enable_shared_from_this<HeaderToolRegression>
{
    DGENERATED_BODY(HeaderToolRegression)

public:
    HeaderToolRegression() = default;
    HeaderToolRegression(const std::string& name, int score) = delete;

    DELTAENGINE_API void Initialize(const std::string& name, int score);

    DFUNCTION()
    int InlineReflected(int value) const;
    template <typename T>
    T TemplateReflected(T value) const { return value; }

private:
    DPROPERTY()
    std::basic_string<char, std::char_traits<char>, std::allocator<char>> m_fullString;

    DPROPERTY()
    long double m_unmapped;
};

DELTA_ENGINE_NS_END
