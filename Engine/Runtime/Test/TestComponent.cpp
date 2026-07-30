#include "Runtime/Test/TestComponent.h"
#include "Runtime/Logging/LogChannels.h"

using namespace DeltaEngine;

int TestComponent::s_lastReceivedValue = 0;
int TestComponent::s_receiveCount = 0;

void TestComponent::ResetEventReceiveTracking()
{
    s_lastReceivedValue = 0;
    s_receiveCount = 0;
}

void TestComponent::TestFunction()
{
    ++m_testInt;
    DLOG(LogCore, ELogLevel::Log, "Test component log");
}

int TestComponent::TestAdd(int a, int b)
{
    return a + b;
}

float TestComponent::TestMultiply(float x, bool negate)
{
    return negate ? -x : x;
}

void TestComponent::OnIntEvent(int value)
{
    m_testInt = value;
}

void TestComponent::OnTwoArgEvent(int a, float b)
{
    m_testInt = a;
    m_testFloat = b;
}

void TestComponent::OnTestEventReceived(int value)
{
    s_lastReceivedValue = value;
    ++s_receiveCount;
}

void TestComponent::BroadcastTestEvent(int value)
{
    OnTestEvent.Broadcast(value);
}
