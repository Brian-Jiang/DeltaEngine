namespace DeltaEngine {

DCLASS(meta=(Category="Gameplay", Tooltip="A reflected thing"))
class MetaTaggedClass : public DObject
{
    DGENERATED_BODY(MetaTaggedClass)
public:
    DPROPERTY(meta=(UIType="Color"))
    float m_value;
};

DSTRUCT(meta=(Category="Math"))
struct MetaTaggedStruct
{
    DPROPERTY()
    float m_x;
};

}
