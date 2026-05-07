#include "Runtime/Utils/StringUtils.h"

#include <gtest/gtest.h>

#include <string>

using namespace DeltaEngine;

TEST(StringUtils, StringUtils_Utf8ToWString_EmptyInput_ReturnsEmpty)
{
    const std::wstring result = StringUtils::Utf8ToWString({});

    EXPECT_TRUE(result.empty());
}

TEST(StringUtils, StringUtils_WStringToUtf8_EmptyInput_ReturnsEmpty)
{
    const std::string result = StringUtils::WStringToUtf8({});

    EXPECT_TRUE(result.empty());
}

TEST(StringUtils, StringUtils_RoundTrip_AsciiPreserved)
{
    const std::string original = "Hello, World!";

    const std::wstring asWide = StringUtils::Utf8ToWString(original);
    const std::string back = StringUtils::WStringToUtf8(asWide);

    EXPECT_EQ(back, original);
}

TEST(StringUtils, StringUtils_RoundTrip_UnicodeSurrogatePairsPreserved)
{
    const char8_t* literal = u8"\U0001F600 - hosi - test";
    const std::string original(reinterpret_cast<const char*>(literal),
                               std::char_traits<char8_t>::length(literal));

    const std::wstring asWide = StringUtils::Utf8ToWString(original);
    const std::string back = StringUtils::WStringToUtf8(asWide);

    EXPECT_EQ(back, original);
}

TEST(StringUtils, StringUtils_PathToUtf8_RoundTripsUnicode)
{
    const char8_t* literal = u8"folder/星.txt";
    const std::string utf8(reinterpret_cast<const char*>(literal),
                           std::char_traits<char8_t>::length(literal));

    const std::filesystem::path p = StringUtils::Utf8ToPath(utf8);
    const std::string back = StringUtils::PathToUtf8(p);

    EXPECT_EQ(back, utf8);
}
