namespace DeltaEngine {

DCLASS()
class HideDetailsTestClass : public DObject
{
    DGENERATED_BODY(HideDetailsTestClass)
public:
    DPROPERTY()
    float m_visible;

    DPROPERTY(HideInDetails)
    float m_hidden;

    DPROPERTY(EditorOnly, HideInDetails)
    float m_editorHidden;
};

}
