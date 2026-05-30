#include "FieldRegistry.h"

#include "common.h"
#include "CPlayerPed.h"
#include "CVector.h"

#include <functional>
#include <unordered_map>

namespace {

using FieldFn = std::function<nlohmann::json()>;
std::unordered_map<std::string, FieldFn> g_fields;

static nlohmann::json readPosition()
{
    CPlayerPed* ped = FindPlayerPed(-1);
    if (!ped) return nullptr;
    const CVector& pos = ped->GetPosition();
    return nlohmann::json{{"x", pos.x}, {"y", pos.y}, {"z", pos.z}};
}

} // anonymous namespace

namespace FieldRegistry {

    void init()
    {
        g_fields["position"] = readPosition;
    }

    bool has(const std::string& name)
    {
        return g_fields.count(name) != 0;
    }

    nlohmann::json get(const std::string& name)
    {
        auto it = g_fields.find(name);
        if (it == g_fields.end()) return nullptr;
        return it->second();
    }

} // namespace FieldRegistry
