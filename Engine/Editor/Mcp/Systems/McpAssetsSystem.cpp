#include "McpAssetsSystem.h"

#include "Editor/Assets/EditorAssetDatabase.h"
#include "Editor/Commands/PropertyValueIO.h"
#include "Editor/EditorCore.h"
#include "Mcp/McpRegistry.h"
#include "Runtime/Assets/DPrimaryAsset.h"
#include "Runtime/Core/DComponent.h"
#include "Runtime/Core/DWorld.h"
#include "Runtime/Core/GameObject.h"
#include "Runtime/Core/SceneComponent.h"
#include "Runtime/IO/IOManager.h"
#include "Runtime/Reflection/DClass.h"
#include "Runtime/Reflection/DProperty.h"
#include "Runtime/Reflection/DVectorProperty.h"
#include "Runtime/Serialization/ScriptPointer.h"

#include <algorithm>
#include <chrono>
#include <cctype>
#include <filesystem>
#include <map>
#include <set>
#include <unordered_set>

using namespace DeltaEngine;

namespace
{

nlohmann::json MakeError(const std::string& msg)
{
    return { {"ok", false}, {"error", msg} };
}

std::string TrimSlashes(std::string s)
{
    while (!s.empty() && (s.front() == '/' || s.front() == '\\'))
        s.erase(0, 1);
    while (!s.empty() && (s.back() == '/' || s.back() == '\\'))
        s.pop_back();
    return s;
}

std::string ToLower(std::string s)
{
    for (char& c : s)
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}

std::string FileTimeToIsoUtc(std::filesystem::file_time_type ft)
{
    using namespace std::chrono;
    const auto sctp = time_point_cast<system_clock::duration>(
        ft - std::filesystem::file_time_type::clock::now() + system_clock::now());
    const std::time_t t = system_clock::to_time_t(sctp);
    std::tm tm{};
#if defined(_WIN32)
    gmtime_s(&tm, &t);
#else
    gmtime_r(&t, &tm);
#endif
    char buf[32]{};
    std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &tm);
    return buf;
}

const char* ClassNameToAssetType(const std::string& className)
{
    if (className == "PA_StaticMesh")
        return "mesh";
    if (className == "PA_Material")
        return "material";
    if (className == "PA_Texture")
        return "texture";
    if (className == "PA_Shader")
        return "shader";
    if (className == "PA_DScene")
        return "scene";
    return "other";
}

bool TypeFilterMatches(const std::string& className, const std::string& typeFilter)
{
    if (typeFilter == "any")
        return true;
    return ClassNameToAssetType(className) == typeFilter;
}

std::filesystem::path GetAssetRootPath()
{
    return std::filesystem::weakly_canonical(
        std::filesystem::path(IOManager::GetEngineImportedAssetsFolder()));
}

std::optional<std::filesystem::path> TryRelativeToAssetRoot(const std::filesystem::path& filePath)
{
    const std::filesystem::path root = GetAssetRootPath();
    try
    {
        return std::filesystem::relative(filePath, root);
    }
    catch (...)
    {
        return std::nullopt;
    }
}

std::string ParentPathString(const std::filesystem::path& rel)
{
    const std::filesystem::path p = rel.parent_path();
    return p.empty() ? std::string{} : p.generic_string();
}

bool AssetMatchesFolder(
    const std::string& parentPathStr,
    const std::string& folderNormalized,
    bool recursive)
{
    if (folderNormalized.empty())
    {
        if (!recursive)
            return parentPathStr.empty();
        return true;
    }
    if (recursive)
        return parentPathStr == folderNormalized
            || parentPathStr.starts_with(folderNormalized + "/");
    return parentPathStr == folderNormalized;
}

nlohmann::json SerializeProperties(
    const DObject* obj,
    const DClass* dclass,
    const std::unordered_set<std::string>& includeFields)
{
    nlohmann::json props = nlohmann::json::object();
    for (const DProperty* p = dclass->GetProperties(); p; p = p->GetHierarchyNext())
    {
        if (!includeFields.empty() && !includeFields.contains(p->GetName()))
            continue;
        nlohmann::json val = PropertyToJson(obj, p);
        if (!val.is_null())
            props[p->GetName()] = std::move(val);
    }
    return props;
}

template <typename Fn>
void VisitResolvedObjectReferencesInProperty(DProperty* prop, void* containerPtr, Fn&& fn)
{
    if (!prop || !containerPtr)
        return;

    if (dynamic_cast<DObjectPtrPropertyBase*>(prop))
    {
        if (DObject* target = prop->GetObjectPointer(containerPtr))
            fn(prop, containerPtr, target);
        return;
    }

    auto* vectorProp = dynamic_cast<DVectorPropertyBase*>(prop);
    if (!vectorProp)
        return;

    DProperty* innerProp = const_cast<DProperty*>(vectorProp->GetInnerProperty());
    if (!innerProp)
        return;

    const size_t elementCount = vectorProp->GetSize(containerPtr);
    for (size_t index = 0; index < elementCount; ++index)
    {
        void* elementAddress = vectorProp->GetElementAddress(containerPtr, index);
        if (!elementAddress)
            continue;
        VisitResolvedObjectReferencesInProperty(innerProp, elementAddress, std::forward<Fn>(fn));
    }
}

template <typename Fn>
void VisitResolvedObjectReferencesInStruct(DStruct* ds, void* basePtr, Fn&& fn)
{
    if (!ds || !basePtr)
        return;

    if (DStruct* parent = ds->GetSuper())
        VisitResolvedObjectReferencesInStruct(parent, basePtr, std::forward<Fn>(fn));

    for (DProperty* prop = ds->GetOwnProperties(); prop; prop = prop->GetNext())
    {
        if (auto* dsp = dynamic_cast<DStructProperty*>(prop))
        {
            void* nested = static_cast<uint8_t*>(basePtr) + dsp->GetOffset();
            if (DStruct* inner = dsp->GetSchema())
                VisitResolvedObjectReferencesInStruct(inner, nested, std::forward<Fn>(fn));
            continue;
        }
        void* slot = static_cast<uint8_t*>(basePtr) + prop->GetOffset();
        VisitResolvedObjectReferencesInProperty(prop, slot, std::forward<Fn>(fn));
    }
}

template <typename Fn>
void VisitResolvedObjectPointersOnObject(DObject* obj, Fn&& fn)
{
    if (!obj || !obj->GetClass())
        return;
    VisitResolvedObjectReferencesInStruct(obj->GetClass(), obj, std::forward<Fn>(fn));
}

std::string GameObjectNameFor(DObject* referrer)
{
    if (auto* go = dynamic_cast<GameObject*>(referrer))
        return go->GetName();
    if (auto* comp = dynamic_cast<DComponent*>(referrer))
    {
        if (GameObject* g = comp->GetGameObject())
            return g->GetName();
    }
    return {};
}

void VisitSceneComponentTree(SceneComponent* sc, auto&& visitor)
{
    if (!sc)
        return;
    visitor(sc);
    for (SceneComponent* ch : sc->GetChildren())
        VisitSceneComponentTree(ch, std::forward<decltype(visitor)>(visitor));
}

struct FolderNode
{
    std::map<std::string, FolderNode> m_children;
};

void InsertDirPath(FolderNode& root, const std::filesystem::path& relDir)
{
    if (relDir.empty())
        return;
    FolderNode* cur = &root;
    for (const auto& part : relDir)
    {
        const std::string seg = part.string();
        if (seg.empty() || seg == ".")
            continue;
        cur = &cur->m_children[seg];
    }
}

nlohmann::json FolderNodeToJson(const FolderNode& node, const std::string& virtualPathPrefix)
{
    nlohmann::json children = nlohmann::json::array();
    for (const auto& [name, child] : node.m_children)
    {
        const std::string childPath = (virtualPathPrefix.empty() || virtualPathPrefix == "/")
            ? (std::string("/") + name)
            : (virtualPathPrefix + "/" + name);
        nlohmann::json j;
        j["name"] = name;
        j["path"] = childPath;
        j["children"] = FolderNodeToJson(child, childPath);
        children.push_back(std::move(j));
    }
    return children;
}

} // namespace

void McpAssetsSystem::RegisterTools(McpRegistry& registry)
{
    registry.RegisterOperation("assets", "list",
        [this](EditorCore& c, const nlohmann::json& p) { return QueryList(c, p); });
    registry.RegisterOperation("assets", "get",
        [this](EditorCore& c, const nlohmann::json& p) { return QueryGet(c, p); });
    registry.RegisterOperation("assets", "search",
        [this](EditorCore& c, const nlohmann::json& p) { return QuerySearch(c, p); });
    registry.RegisterOperation("assets", "folder_tree",
        [this](EditorCore& c, const nlohmann::json& p) { return QueryFolderTree(c, p); });
    registry.RegisterOperation("assets", "usages",
        [this](EditorCore& c, const nlohmann::json& p) { return QueryUsages(c, p); });
}

nlohmann::json McpAssetsSystem::QueryList(EditorCore& core, const nlohmann::json& params)
{
    EditorAssetDatabase* db = core.GetAssetDatabase();
    if (!db)
        return MakeError("no asset database");

    const std::string folderNorm = TrimSlashes(params.value("folder", "/"));
    const std::string typeFilter  = params.value("type_filter", "any");
    const bool recursive          = params.value("recursive", false);

    std::unordered_set<std::string> includeFields;
    if (params.contains("include_fields") && params["include_fields"].is_array())
    {
        for (const auto& f : params["include_fields"])
            if (f.is_string())
                includeFields.insert(f.get<std::string>());
    }
    else
    {
        includeFields.insert("path");
        includeFields.insert("type");
    }

    nlohmann::json assets = nlohmann::json::array();

    for (const auto& [id, entry] : db->GetAllAssets())
    {
        if (entry.m_filePath.empty())
            continue;
        if (!TypeFilterMatches(entry.m_header.m_className, typeFilter))
            continue;

        const auto relOpt = TryRelativeToAssetRoot(entry.m_filePath);
        if (!relOpt.has_value())
            continue;

        const std::string parentStr = ParentPathString(*relOpt);
        if (!AssetMatchesFolder(parentStr, folderNorm, recursive))
            continue;

        nlohmann::json row;
        row["asset_id"] = id.ToString();

        if (includeFields.contains("path"))
            row["path"] = relOpt->generic_string();
        if (includeFields.contains("type"))
            row["type"] = ClassNameToAssetType(entry.m_header.m_className);

        if (includeFields.contains("size"))
        {
            std::error_code ec;
            if (std::filesystem::is_regular_file(entry.m_filePath, ec))
            {
                const auto sz = std::filesystem::file_size(entry.m_filePath, ec);
                if (!ec)
                    row["size"] = sz;
            }
        }

        if (includeFields.contains("import_date"))
        {
            std::error_code ec;
            const auto ft = std::filesystem::last_write_time(entry.m_filePath, ec);
            if (!ec)
                row["import_date"] = FileTimeToIsoUtc(ft);
        }

        if (includeFields.contains("dependencies"))
        {
            DPrimaryAsset* loaded = db->LoadAsset(id);
            std::set<std::string> depIds;
            if (loaded)
            {
                for (const ScriptPointer& sp : loaded->CollectExternalReferences())
                {
                    if (!sp.m_assetId.IsNull())
                        depIds.insert(sp.m_assetId.ToString());
                }
            }
            row["dependencies"] = nlohmann::json::array();
            for (const auto& s : depIds)
                row["dependencies"].push_back(s);
        }

        if (includeFields.contains("thumbnail_available"))
            row["thumbnail_available"] = false;

        assets.push_back(std::move(row));
    }

    return { {"ok", true}, {"assets", std::move(assets)} };
}

nlohmann::json McpAssetsSystem::QueryGet(EditorCore& core, const nlohmann::json& params)
{
    if (!params.contains("asset_id") || !params["asset_id"].is_string())
        return MakeError("missing required param: asset_id");

    const AssetId assetId = UUID::FromString(params["asset_id"].get<std::string>());
    if (assetId.IsNull())
        return MakeError("invalid asset_id");

    EditorAssetDatabase* db = core.GetAssetDatabase();
    if (!db)
        return MakeError("no asset database");

    DPrimaryAsset* asset = db->LoadAsset(assetId);
    if (!asset)
        return MakeError("asset not found or failed to load");

    std::unordered_set<std::string> includeFields;
    if (params.contains("include_fields") && params["include_fields"].is_array())
    {
        for (const auto& f : params["include_fields"])
            if (f.is_string())
                includeFields.insert(f.get<std::string>());
    }
    else
    {
        includeFields.insert("properties");
        includeFields.insert("dependencies");
    }

    nlohmann::json result;
    result["ok"]        = true;
    result["asset_id"]  = assetId.ToString();
    result["class"]     = asset->GetHeader().m_className;
    result["type"]      = ClassNameToAssetType(asset->GetHeader().m_className);

    if (includeFields.contains("properties"))
    {
        nlohmann::json objects = nlohmann::json::array();
        const std::unordered_set<std::string> allProps;
        for (DObject* obj : asset->GetObjects())
        {
            if (!obj || !obj->GetClass())
                continue;
            nlohmann::json o;
            o["object_id"] = obj->GetObjectId().ToString();
            o["class"]     = obj->GetClass()->GetName();
            o["properties"] = SerializeProperties(obj, obj->GetClass(), allProps);
            objects.push_back(std::move(o));
        }
        result["properties"] = std::move(objects);
    }

    if (includeFields.contains("dependencies"))
    {
        nlohmann::json deps = nlohmann::json::array();
        for (const ScriptPointer& sp : asset->CollectExternalReferences())
        {
            if (sp.m_assetId.IsNull())
                continue;
            deps.push_back({
                {"asset_id", sp.m_assetId.ToString()},
                {"object_id", sp.m_objectId.ToString()},
            });
        }
        result["dependencies"] = std::move(deps);
    }

    if (includeFields.contains("dependents"))
    {
        nlohmann::json depList = nlohmann::json::array();
        std::set<std::string> seen;
        for (const auto& [otherId, otherEntry] : db->GetAllAssets())
        {
            if (otherId == assetId)
                continue;
            DPrimaryAsset* other = db->LoadAsset(otherId);
            if (!other)
                continue;
            bool isDep = false;
            for (const ScriptPointer& sp : other->CollectExternalReferences())
            {
                if (sp.m_assetId == assetId)
                {
                    isDep = true;
                    break;
                }
            }
            if (isDep && seen.insert(otherId.ToString()).second)
                depList.push_back(otherId.ToString());
        }
        result["dependents"] = std::move(depList);
    }

    if (includeFields.contains("source_path"))
        result["source_path"] = db->GetAssetPath(assetId).string();

    if (includeFields.contains("import_settings"))
        result["import_settings"] = nlohmann::json::object();

    return result;
}

nlohmann::json McpAssetsSystem::QuerySearch(EditorCore& core, const nlohmann::json& params)
{
    if (!params.contains("query") || !params["query"].is_string())
        return MakeError("missing required param: query");
    const std::string q = params["query"].get<std::string>();
    if (q.empty())
        return MakeError("query must be non-empty");

    const std::string typeFilter = params.value("type_filter", "any");
    int limit                    = params.value("limit", 20);
    limit                        = std::clamp(limit, 1, 500);

    EditorAssetDatabase* db = core.GetAssetDatabase();
    if (!db)
        return MakeError("no asset database");

    const std::string qLower = ToLower(q);

    struct Hit
    {
        std::string path;
        AssetId     id;
        std::string className;
    };
    std::vector<Hit> hits;

    for (const auto& [id, entry] : db->GetAllAssets())
    {
        if (entry.m_filePath.empty())
            continue;
        if (!TypeFilterMatches(entry.m_header.m_className, typeFilter))
            continue;

        const auto relOpt = TryRelativeToAssetRoot(entry.m_filePath);
        if (!relOpt.has_value())
            continue;

        const std::filesystem::path& rel = *relOpt;
        std::string display                = rel.stem().stem().string();
        const std::string pathLower        = ToLower(rel.generic_string());
        const std::string displayLower     = ToLower(display);

        if (pathLower.find(qLower) == std::string::npos
            && displayLower.find(qLower) == std::string::npos)
            continue;

        hits.push_back(Hit{ rel.generic_string(), id, entry.m_header.m_className });
    }

    std::sort(hits.begin(), hits.end(), [](const Hit& a, const Hit& b) { return a.path < b.path; });
    if (static_cast<int>(hits.size()) > limit)
        hits.resize(static_cast<size_t>(limit));

    nlohmann::json results = nlohmann::json::array();
    for (const Hit& h : hits)
    {
        results.push_back({
            {"asset_id", h.id.ToString()},
            {"path", h.path},
            {"type", ClassNameToAssetType(h.className)},
        });
    }

    return { {"ok", true}, {"results", std::move(results)} };
}

nlohmann::json McpAssetsSystem::QueryFolderTree(EditorCore& core, const nlohmann::json& params)
{
    EditorAssetDatabase* db = core.GetAssetDatabase();
    if (!db)
        return MakeError("no asset database");
    (void)db;

    const std::string rootNorm = TrimSlashes(params.value("root", "/"));

    const std::filesystem::path assetRoot = GetAssetRootPath();
    if (!std::filesystem::exists(assetRoot) || !std::filesystem::is_directory(assetRoot))
        return MakeError("asset root is not a directory");

    std::filesystem::path scanRoot = assetRoot;
    if (!rootNorm.empty())
    {
        scanRoot /= std::filesystem::path(rootNorm);
        std::error_code ec;
        if (!std::filesystem::exists(scanRoot, ec) || !std::filesystem::is_directory(scanRoot))
            return MakeError("root folder does not exist");
    }

    FolderNode rootNode;
    std::error_code ec;
    std::filesystem::recursive_directory_iterator it(scanRoot, std::filesystem::directory_options::skip_permission_denied, ec);
    if (ec)
        return MakeError("failed to scan asset folders");
    const std::filesystem::recursive_directory_iterator end;
    for (; !ec && it != end; it.increment(ec))
    {
        if (ec)
            break;
        const std::filesystem::directory_entry& e = *it;
        if (!e.is_directory())
            continue;
        auto rel = std::filesystem::relative(e.path(), scanRoot, ec);
        if (ec)
            continue;
        InsertDirPath(rootNode, rel);
    }

    const std::string virtPath = rootNorm.empty() ? "/" : ("/" + rootNorm);
    const std::string nodeName =
        rootNorm.empty() ? "." : std::filesystem::path(rootNorm).filename().string();

    nlohmann::json tree;
    tree["name"]     = nodeName;
    tree["path"]     = virtPath;
    tree["children"] = FolderNodeToJson(rootNode, virtPath);
    return { {"ok", true}, {"tree", std::move(tree)} };
}

nlohmann::json McpAssetsSystem::QueryUsages(EditorCore& core, const nlohmann::json& params)
{
    if (!params.contains("asset_id") || !params["asset_id"].is_string())
        return MakeError("missing required param: asset_id");

    const AssetId targetAssetId = UUID::FromString(params["asset_id"].get<std::string>());
    if (targetAssetId.IsNull())
        return MakeError("invalid asset_id");

    DWorld* world = core.GetWorld();
    if (!world)
        return { {"ok", true}, {"usages", nlohmann::json::array()} };

    nlohmann::json usages = nlohmann::json::array();

    for (GameObject* go : world->GetGameObjects())
    {
        auto emitFor = [&](DObject* obj)
        {
            if (!obj || !obj->GetClass())
                return;
            VisitResolvedObjectPointersOnObject(obj,
                [&](DProperty* prop, void*, DObject* target)
                {
                    if (!target)
                        return;
                    DPrimaryAsset* own = target->GetOwningAsset();
                    if (!own || own->GetAssetId() != targetAssetId)
                        return;

                    nlohmann::json row;
                    row["property"] = prop->GetName();
                    if (DClass* cls = obj->GetClass())
                        row["referrer_class"] = cls->GetName();
                    row["referrer_object_id"] = obj->GetObjectId().ToString();
                    row["game_object_name"]   = GameObjectNameFor(obj);
                    usages.push_back(std::move(row));
                });
        };

        emitFor(go);
        for (DComponent* c : go->GetComponents())
            emitFor(c);
        for (SceneComponent* sc : go->GetSceneComponents())
            VisitSceneComponentTree(sc, [&](SceneComponent* node) { emitFor(node); });
    }

    return { {"ok", true}, {"usages", std::move(usages)} };
}
