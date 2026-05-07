#include "Editor/UIComponents/UIComponentsImGuiFixture.h"

#include "Editor/UIComponents/PropertyWidgets/ColorField.h"
#include "Editor/UIComponents/PropertyWidgets/ReferenceField.h"
#include "Editor/UIComponents/PropertyWidgets/ScalarField.h"
#include "Editor/UIComponents/PropertyWidgets/StringField.h"
#include "Editor/UIComponents/PropertyWidgets/Vec3Field.h"

#include <gtest/gtest.h>

using namespace DeltaEngine;
using namespace DeltaEngine::Tests;

TEST_F(UIComponentsImGuiFixture, ScalarField_Draw_NullLabel_ReturnsEmptyEvent)
{
    float       v  = 1.f;
    ScalarField sf;
    WidgetEditEvent evt = sf.Draw(nullptr, &v);
    EXPECT_FALSE(evt.valueChanged);
}

TEST_F(UIComponentsImGuiFixture, ScalarField_Draw_NullValue_ReturnsEmptyEvent)
{
    ScalarField sf;
    WidgetEditEvent evt = sf.Draw("V", nullptr);
    EXPECT_FALSE(evt.valueChanged);
}

TEST_F(UIComponentsImGuiFixture, StringField_Draw_NullBuf_ReturnsEmptyEvent)
{
    StringField sf;
    char        buf[] = "a";
    WidgetEditEvent e1 = sf.Draw("L", nullptr, 4);
    WidgetEditEvent e2 = sf.Draw("L", buf, 0);
    EXPECT_FALSE(e1.valueChanged);
    EXPECT_FALSE(e2.valueChanged);
}

TEST_F(UIComponentsImGuiFixture, Vec3Field_Draw_NullValues_ReturnsEmptyEvent)
{
    Vec3Field vf;
    WidgetEditEvent evt = vf.Draw("P", nullptr);
    EXPECT_FALSE(evt.valueChanged);
}

TEST_F(UIComponentsImGuiFixture, ColorField_Draw_NullValues_ReturnsEmptyEvent)
{
    ColorField cf;
    WidgetEditEvent evt = cf.Draw("C", nullptr);
    EXPECT_FALSE(evt.valueChanged);
}

TEST_F(UIComponentsImGuiFixture, ReferenceField_Draw_NullLabel_ReturnsEarly)
{
    ReferenceField rf;
    rf.Draw(nullptr, "x");
}
