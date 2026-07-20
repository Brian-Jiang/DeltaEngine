#include "McpAssetsSystem.h"

#include "Editor/Assets/EditorAssetDatabase.h"
#include "Editor/Commands/EditorCommand_SetAssetDynamicMeta.h"
#include "Editor/Commands/PropertyValueIO.h"
#include "Editor/EditorCore.h"
#include "Mcp/McpProtocol.h"
#include "Mcp/McpRegistry.h"
#include "Runtime/Assets/DPrimaryAsset.h"
#include "Runtime/Assets/PA_CommonAssets.h"
#include "Runtime/Core/DComponent.h"
#include "Runtime/Core/DShader.h"
#include "Runtime/Core/DWorld.h"
#include "Runtime/Core/GameObject.h"
#include "Runtime/Core/SceneComponent.h"
#include "Runtime/IO/IOManager.h"
#include "Runtime/Logging/LogCategory.h"
#include "Runtime/Reflection/DClass.h"
#include "Runtime/Reflection/DProperty.h"
#include "Runtime/Reflection/DVectorProperty.h"
#include "Runtime/Reflection/ReflectionRegistry.h"
#include "Runtime/Serialization/ScriptPointer.h"

#include <algorithm>
#include <chrono>
#include <cctype>
#include <filesystem>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <system_error>
#include <unordered_set>

using namespace DeltaEngine;

DEFINE_LOG_CATEGORY_STATIC(LogMcpAssets);

namespace
{

nlohmann::json MakeError(const std::string& msg)
{
    return MakeMcpError(msg);
}

std::string TrimSlashes(std::string s)
{
    while (!s.empty() && (s.front() == '/' || s.front() == '\\'))
        s.erase(0, 1);
    while (!s.empty() && (s.back() == '/' || s.back() == '\\'))
        s.pop_back();
    return s;
}

bool IsPathUnderRoot(const std::filesystem::path& root, const std::filesystem::path& candidate)
{
    std::error_code ec;
    const auto rel = std::filesystem::relative(candidate, root, ec);
    if (ec || rel.empty())
        return !ec && candidate == root;
    const std::string relStr = rel.generic_string();
    return !relStr.starts_with("..");
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

bool ClassHasStaticMetaSchema(const std::string& className)
{
    if (className.empty())
        return false;

    DClass* dclass = GetReflectionRegistry().FindClassByName(className);
    if (!dclass)
        return false;

    DObject* obj = GetReflectionRegistry().CreateObject(className);
    if (!obj)
        return false;

    bool result = false;
    if (auto* asset = dynamic_cast<DPrimaryAsset*>(obj))
    {
        auto [schema, instance] = asset->GetStaticMetaSchema();
        result = (schema != nullptr);
    }
    GetReflectionRegistry().DestroyObject(obj);
    return result;
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
    EditorCore& core,
    const DObject* obj,
    const DClass* dclass,
    const std::unordered_set<std::string>& includeFields)
{
    nlohmann::json props = nlohmann::json::object();
    for (const DProperty* p = dclass->GetProperties(); p; p = p->GetHierarchyNext())
    {
        if (!includeFields.empty() && !includeFields.contains(p->GetName()))
            continue;
        nlohmann::json val = PropertyToJson(obj, p, core);
        if (!val.is_null())
            props[p->GetName()] = std::move(val);
    }
    return props;
}

nlohmann::json CollectAssetObjects(const DPrimaryAsset* asset)
{
    nlohmann::json objects = nlohmann::json::array();
    if (!asset)
        return objects;

    for (DObject* obj : asset->GetObjects())
    {
        if (!obj || !obj->GetClass())
            continue;
        objects.push_back({
            {"object_id", obj->GetObjectId().ToString()},
            {"class", obj->GetClass()->GetName()},
        });
    }
    return objects;
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
        if (dynamic_cast<DObjectPtrPropertyBase*>(prop))
            VisitResolvedObjectReferencesInProperty(prop, basePtr, std::forward<Fn>(fn));
        else
        {
            void* slot = static_cast<uint8_t*>(basePtr) + prop->GetOffset();
            VisitResolvedObjectReferencesInProperty(prop, slot, std::forward<Fn>(fn));
        }
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
    registry.RegisterQuery("assets", "list",
        [this](EditorCore& c, const nlohmann::json& p) { return QueryList(c, p); });
    registry.RegisterQuery("assets", "get",
        [this](EditorCore& c, const nlohmann::json& p) { return QueryGet(c, p); });
    registry.RegisterQuery("assets", "search",
        [this](EditorCore& c, const nlohmann::json& p) { return QuerySearch(c, p); });
    registry.RegisterQuery("assets", "folder_tree",
        [this](EditorCore& c, const nlohmann::json& p) { return QueryFolderTree(c, p); });
    registry.RegisterQuery("assets", "usages",
        [this](EditorCore& c, const nlohmann::json& p) { return QueryUsages(c, p); });
    registry.RegisterQuery("assets", "get_asset_metadata",
        [this](EditorCore& c, const nlohmann::json& p) { return QueryGetAssetMetadata(c, p); });
    registry.RegisterQuery("assets", "get_assets_metadata",
        [this](EditorCore& c, const nlohmann::json& p) { return QueryGetAssetsMetadata(c, p); });
    registry.RegisterQuery("assets", "has_static_meta_schema",
        [this](EditorCore& c, const nlohmann::json& p) { return QueryHasStaticMetaSchema(c, p); });

    registry.RegisterCommand("assets", "set_asset_dynamic_metadata",
        [this](EditorCore& c, const nlohmann::json& p) { return CommandSetAssetDynamicMetadata(c, p); });
    registry.RegisterCommand("assets", "reimport_assets",
        [this](EditorCore& c, const nlohmann::json& p) { return CommandReimportAssets(c, p); });
    registry.RegisterCommand("assets", "duplicate_asset",
        [this](EditorCore& c, const nlohmann::json& p) { return CommandDuplicateAsset(c, p); });
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
    result["objects"]   = CollectAssetObjects(asset);

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
            o["properties"] = SerializeProperties(core, obj, obj->GetClass(), allProps);
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

nlohmann::json McpAssetsSystem::QueryGetAssetMetadata(EditorCore& core, const nlohmann::json& params)
{
    if (!params.contains("asset_id") || !params["asset_id"].is_string())
        return MakeError("missing required param: asset_id");

    const AssetId assetId = UUID::FromString(params["asset_id"].get<std::string>());
    if (assetId.IsNull())
        return MakeError("invalid asset_id");

    EditorAssetDatabase* db = core.GetAssetDatabase();
    if (!db)
        return MakeError("no asset database");

    const auto& allAssets = db->GetAllAssets();
    auto it = allAssets.find(assetId);
    if (it == allAssets.end())
        return MakeError("asset not found");

    nlohmann::json result;
    result["ok"]                      = true;
    result["asset_id"]                = assetId.ToString();
    result["class"]                   = it->second.m_header.m_className;
    result["meta"]                    = it->second.m_meta;
    result["has_static_meta_schema"]  = ClassHasStaticMetaSchema(it->second.m_header.m_className);
    return result;
}

nlohmann::json McpAssetsSystem::QueryGetAssetsMetadata(EditorCore& core, const nlohmann::json& params)
{
    EditorAssetDatabase* db = core.GetAssetDatabase();
    if (!db)
        return MakeError("no asset database");

    const std::string typeFilter = params.value("type_filter", "any");

    std::vector<AssetId> requested;
    bool useRequested = false;
    if (params.contains("asset_ids") && params["asset_ids"].is_array())
    {
        useRequested = true;
        for (const auto& idJson : params["asset_ids"])
        {
            if (!idJson.is_string())
                continue;
            AssetId aid = UUID::FromString(idJson.get<std::string>());
            if (!aid.IsNull())
                requested.push_back(aid);
        }
    }

    std::unordered_map<std::string, bool> staticSchemaCache;
    auto staticSchemaFor = [&](const std::string& className) -> bool
    {
        auto cacheIt = staticSchemaCache.find(className);
        if (cacheIt != staticSchemaCache.end())
            return cacheIt->second;
        const bool present = ClassHasStaticMetaSchema(className);
        staticSchemaCache.emplace(className, present);
        return present;
    };

    nlohmann::json items = nlohmann::json::array();

    auto emit = [&](const AssetId& id, const EditorAssetDatabase::AssetEntry& entry)
    {
        if (!TypeFilterMatches(entry.m_header.m_className, typeFilter))
            return;
        nlohmann::json row;
        row["asset_id"]                 = id.ToString();
        row["class"]                    = entry.m_header.m_className;
        row["type"]                     = ClassNameToAssetType(entry.m_header.m_className);
        row["meta"]                     = entry.m_meta;
        row["has_static_meta_schema"]   = staticSchemaFor(entry.m_header.m_className);
        items.push_back(std::move(row));
    };

    const auto& allAssets = db->GetAllAssets();
    if (useRequested)
    {
        for (const AssetId& id : requested)
        {
            auto it = allAssets.find(id);
            if (it == allAssets.end())
                continue;
            emit(id, it->second);
        }
    }
    else
    {
        for (const auto& [id, entry] : allAssets)
            emit(id, entry);
    }

    return { {"ok", true}, {"items", std::move(items)} };
}

nlohmann::json McpAssetsSystem::CommandSetAssetDynamicMetadata(EditorCore& core, const nlohmann::json& params)
{
    if (!params.contains("asset_id") || !params["asset_id"].is_string())
        return MakeError("missing required param: asset_id");
    if (!params.contains("json_path") || !params["json_path"].is_string())
        return MakeError("missing required param: json_path");
    if (!params.contains("new_value"))
        return MakeError("missing required param: new_value");

    const std::string assetIdStr = params["asset_id"].get<std::string>();
    const AssetId assetId = UUID::FromString(assetIdStr);
    if (assetId.IsNull())
        return MakeError("invalid asset_id");

    const std::string jsonPath = params["json_path"].get<std::string>();

    if (jsonPath == "/static" || jsonPath.starts_with("/static/"))
        return MakeError("json_path points to static metadata (read-only): '" + jsonPath + "'");

    const bool isDynamicRoot  = jsonPath.empty() || jsonPath == "/dynamic";
    const bool isDynamicChild = jsonPath.starts_with("/dynamic/");
    if (!isDynamicRoot && !isDynamicChild)
        return MakeError("invalid json_path '" + jsonPath + "' (must be '', '/dynamic', or '/dynamic/...')");

    if (isDynamicChild)
    {
        try
        {
            (void)nlohmann::json::json_pointer{jsonPath.substr(std::string_view("/dynamic").size())};
        }
        catch (const std::exception& e)
        {
            return MakeError("invalid json_path '" + jsonPath + "': " + e.what());
        }
    }

    EditorAssetDatabase* db = core.GetAssetDatabase();
    if (!db)
        return MakeError("no asset database");

    if (!db->LoadAsset(assetId))
        return MakeError("asset not found or failed to load");

    nlohmann::json err;
    if (!ExecuteMcpCommand(core, std::make_unique<EditorCommand_SetAssetDynamicMeta>(
            assetId, jsonPath, params["new_value"]), err))
        return err;

    return MakeMcpOk();
}

nlohmann::json McpAssetsSystem::CommandReimportAssets(EditorCore& core, const nlohmann::json& params)
{
    if (!params.contains("asset_ids") || !params["asset_ids"].is_array())
        return MakeError("missing required param: asset_ids (array of UUID strings)");

    nlohmann::json assetIds = nlohmann::json::array();
    for (const auto& v : params["asset_ids"])
    {
        if (!v.is_string())
            return MakeError("asset_ids must contain only strings");
        const std::string idStr = v.get<std::string>();
        if (UUID::FromString(idStr).IsNull())
            return MakeError("invalid asset_id: " + idStr);
        assetIds.push_back(idStr);
    }

    std::vector<AssetId> ids;
    for (const auto& v : assetIds)
        ids.push_back(UUID::FromString(v.get<std::string>()));

    if (ids.empty())
        return MakeError("asset_ids is empty");

    EditorAssetDatabase* db = core.GetAssetDatabase();
    if (!db)
        return MakeError("no asset database");

    // Runs on the main thread: shader recompile swaps GPU blobs the render thread reads.
    nlohmann::json reimported = nlohmann::json::array();
    nlohmann::json skipped    = nlohmann::json::array();

    for (const AssetId& id : ids)
    {
        DPrimaryAsset* asset = db->LoadAsset(id);
        if (!asset)
        {
            skipped.push_back({{"asset_id", id.ToString()}, {"reason", "not found or failed to load"}});
            continue;
        }

        if (auto* shaderAsset = dynamic_cast<PA_Shader*>(asset))
        {
            DShader* shader = shaderAsset->GetShader();
            if (!shader)
            {
                skipped.push_back({{"asset_id", id.ToString()}, {"reason", "shader asset has no shader"}});
                continue;
            }

            DLOG(LogMcpAssets, ELogLevel::Verbose, "reimport_assets: reimporting shader '{}'", id.ToString());
            shader->Reimport();
            reimported.push_back(id.ToString());
            continue;
        }

        skipped.push_back({{"asset_id", id.ToString()}, {"reason", "not a shader asset"}});
    }

    return { {"ok", true}, {"reimported", std::move(reimported)}, {"skipped", std::move(skipped)} };
}

nlohmann::json McpAssetsSystem::CommandDuplicateAsset(EditorCore& core, const nlohmann::json& params)
{
    if (!params.contains("asset_id") || !params["asset_id"].is_string())
        return MakeError("missing required param: asset_id");

    const std::string assetIdStr = params["asset_id"].get<std::string>();
    const AssetId assetId = UUID::FromString(assetIdStr);
    if (assetId.IsNull())
        return MakeError("invalid asset_id");

    if (params.contains("new_name"))
    {
        if (!params["new_name"].is_string())
            return MakeError("new_name must be a string");
        const std::string newName = params["new_name"].get<std::string>();
        if (newName.empty())
            return MakeError("new_name must be non-empty");
        if (newName.find('/') != std::string::npos || newName.find('\\') != std::string::npos)
            return MakeError("new_name must not contain path separators");
    }

    if (params.contains("new_path") && !params["new_path"].is_string())
        return MakeError("new_path must be a string");

    std::optional<std::string> newName;
    if (params.contains("new_name") && params["new_name"].is_string())
        newName = params["new_name"].get<std::string>();

    std::optional<std::string> newPathVirtual;
    if (params.contains("new_path") && params["new_path"].is_string())
        newPathVirtual = params["new_path"].get<std::string>();

    EditorAssetDatabase* db = core.GetAssetDatabase();
    if (!db)
        return MakeError("no asset database");

    if (!db->LoadAsset(assetId))
        return MakeError("asset not found or failed to load");

    const AssetId duplicatedId = db->DuplicateAsset(assetId);
    if (duplicatedId.IsNull())
        return MakeError("duplicate failed");

    auto cleanupOnFailure = [&]() { db->DeleteAsset(duplicatedId); };

    if (newName.has_value())
    {
        if (!db->RenameAssetToExactStem(duplicatedId, *newName))
        {
            cleanupOnFailure();
            return MakeError("rename failed: target stem already exists or filesystem error");
        }
    }

    if (newPathVirtual.has_value())
    {
        const std::string folderNorm = TrimSlashes(*newPathVirtual);
        const std::filesystem::path assetRoot = core.GetAssetRoot();
        std::filesystem::path targetFolder = folderNorm.empty()
            ? assetRoot
            : assetRoot / std::filesystem::path(folderNorm).generic_string();

        std::error_code ec;
        targetFolder = std::filesystem::weakly_canonical(targetFolder, ec);
        if (ec || !IsPathUnderRoot(assetRoot, targetFolder))
        {
            cleanupOnFailure();
            return MakeError("new_path is outside the imported-assets root");
        }

        std::filesystem::create_directories(targetFolder, ec);
        if (ec)
        {
            cleanupOnFailure();
            return MakeError("failed to create target folder: " + ec.message());
        }

        if (!db->MoveAsset(duplicatedId, targetFolder))
        {
            cleanupOnFailure();
            return MakeError("move failed: target path may already exist or filesystem error");
        }
    }

    const std::filesystem::path absPath = db->GetAssetPath(duplicatedId);
    std::string relPath = absPath.generic_string();
    std::error_code relEc;
    const auto relative = std::filesystem::relative(absPath, core.GetAssetRoot(), relEc);
    if (!relEc)
        relPath = relative.generic_string();

    return {
        {"ok", true},
        {"asset_id", duplicatedId.ToString()},
        {"path", std::move(relPath)},
    };
}

nlohmann::json McpAssetsSystem::QueryHasStaticMetaSchema(EditorCore& core, const nlohmann::json& params)
{
    std::string className;

    if (params.contains("asset_id") && params["asset_id"].is_string())
    {
        const AssetId assetId = UUID::FromString(params["asset_id"].get<std::string>());
        if (assetId.IsNull())
            return MakeError("invalid asset_id");

        EditorAssetDatabase* db = core.GetAssetDatabase();
        if (!db)
            return MakeError("no asset database");

        const auto& allAssets = db->GetAllAssets();
        auto it = allAssets.find(assetId);
        if (it == allAssets.end())
            return MakeError("asset not found");

        className = it->second.m_header.m_className;
    }
    else if (params.contains("class") && params["class"].is_string())
    {
        className = params["class"].get<std::string>();
    }
    else
    {
        return MakeError("missing required param: asset_id or class");
    }

    return {
        {"ok", true},
        {"class", className},
        {"has_static_meta_schema", ClassHasStaticMetaSchema(className)}
    };
}
