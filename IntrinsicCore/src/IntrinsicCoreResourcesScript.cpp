// Intrinsic
// Copyright (c) 2016 Benjamin Glatzel
//
// This program is free software : you can redistribute it and / or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

// Precompiled header file
#include "stdafx.h"

#if defined(_INTR_FINAL_BUILD)
#define SOL_PROTECTED_EXEC(_call, _result)                                     \
  _call;                                                                       \
  _result = true
#else
#define SOL_PROTECTED_EXEC(_call, _result)                                     \
  try                                                                          \
  {                                                                            \
    _call;                                                                     \
    _result = true;                                                            \
  }                                                                            \
  catch (...)                                                                  \
  {                                                                            \
    _INTR_LOG_WARNING(lua_tostring(_luaState.lua_state(), -1));                \
  }
#endif // _INTR_FINAL_BUILD

namespace Intrinsic
{
namespace Core
{
namespace Resources
{
// Global variables
_INTR_STRING _scriptPath = "scripts/";
sol::state _luaState;

void setupLuaState(sol::state* p_State)
{
  _luaState = sol::state();

  p_State->open_libraries(sol::lib::base, sol::lib::math, sol::lib::table);

  p_State->create_named_table("methods", "tick", p_State->create_table(),
                              "onCreate", p_State->create_table(), "onDestroy",
                              p_State->create_table());

  p_State->script("function registerTickFunction(id, func)\n"
                  "methods[\"tick\"][id] = func\n"
                  "end\n");
  p_State->script("function registerOnCreateFunction(id, func)\n"
                  "methods[\"onCreate\"][id] = func\n"
                  "end\n");
  p_State->script("function registerOnDestroyFunction(id, func)\n"
                  "methods[\"onDestroy\"][id] = func\n"
                  "end\n");

  sol::table glmTable = p_State->create_named_table("glm");

  sol::table nodeComponentTable = p_State->create_named_table("nodeComponent");
  sol::table meshComponentTable = p_State->create_named_table("meshComponent");

  sol::table worldTable = p_State->create_named_table("world");

  sol::table entityTable = p_State->create_named_table("entity");
  sol::table inputSystemTable = p_State->create_named_table("inputSystem");

  {
    inputSystemTable["keyState"] = &Input::System::getKeyState;
    inputSystemTable["lastMousePos"] = &Input::System::getLastMousePos;
    inputSystemTable["lastMousePosViewport"] =
        &Input::System::getLastMousePosViewport;
    inputSystemTable["lastMousePosRel"] = &Input::System::getLastMousePosRel;
  }

  {
    p_State->new_usertype<glm::vec2, float, float>("Vec2", "x", &glm::vec2::x,
                                                   "y", &glm::vec2::y);
    p_State->new_usertype<glm::vec3, float, float, float>(
        "Vec3", "x", &glm::vec3::x, "y", &glm::vec3::y, "z", &glm::vec3::z);
    p_State->new_usertype<glm::vec4, float, float, float, float>(
        "Vec4", "x", &glm::vec4::x, "y", &glm::vec4::y, "z", &glm::vec4::z, "w",
        &glm::vec4::w);

    sol::constructors<sol::types<float, float, float, float>,
                      sol::types<const glm::vec3&>>
        quatConstructors;
    sol::usertype<glm::quat> quatUserType(quatConstructors, "x", &glm::quat::x,
                                          "y", &glm::quat::y, "z",
                                          &glm::quat::z, "w", &glm::quat::w);

    p_State->set_usertype("Quat", quatUserType);

    glmTable["rotate"] =
        (glm::vec3(*)(const glm::quat&, const glm::vec3&)) & glm::rotate;
    glmTable["rotate"] =
        (glm::vec4(*)(const glm::quat&, const glm::vec4&)) & glm::rotate;
    glmTable["rotate"] =
        (glm::quat(*)(const glm::quat&, const glm::quat&)) & glm::operator*;
  }

  {
    p_State->new_usertype<Name, const char*>("Name");
    p_State->new_usertype<Dod::Ref>("Ref", "isValid", &Dod::Ref::isValid);
  }

  {// Entity API
   {entityTable["getEntityByName"] = &Entity::EntityManager::getEntityByName;
  entityTable["createEntity"] = &Entity::EntityManager::createEntity;
  entityTable["isAlive"] = &Entity::EntityManager::isAlive;
}

// World API
{
  worldTable["getRootNode"] = &World::getRootNode;
  worldTable["setRootNode"] = &World::setRootNode;
  worldTable["getActiveCamera"] = &World::getActiveCamera;
  worldTable["setActiveCamera"] = &World::setActiveCamera;

  worldTable["destroyNodeFull"] = &World::destroyNodeFull;
  worldTable["cloneNodeFull"] = &World::cloneNodeFull;
}
}

{
  // Mesh component API
  {
    meshComponentTable["getComponentForEntity"] =
        &Components::MeshManager::getComponentForEntity;
    meshComponentTable["destroyResources"] =
        (void (*)(Components::MeshRef)) &
        Components::MeshManager::destroyResources;
    meshComponentTable["createResources"] =
        (void (*)(Components::MeshRef)) &
        Components::MeshManager::createResources;
    meshComponentTable["createMesh"] = &Components::MeshManager::createMesh;

    meshComponentTable["getMeshName"] = &Components::MeshManager::getMeshName;
    meshComponentTable["setMeshName"] = &Components::MeshManager::setMeshName;
  }

  // Node API
  {
    nodeComponentTable["getComponentForEntity"] =
        &Components::NodeManager::getComponentForEntity;
    nodeComponentTable["createNode"] = &Components::NodeManager::createNode;
    nodeComponentTable["attachChild"] = &Components::NodeManager::attachChild;

    nodeComponentTable["rebuildTree"] = &Components::NodeManager::rebuildTree;
    nodeComponentTable["rebuildTreeAndUpdateTransforms"] =
        &Components::NodeManager::rebuildTreeAndUpdateTransforms;
    nodeComponentTable["updateTransforms"] =
        (void (*)(Components::NodeRef)) &
        Components::NodeManager::updateTransforms;

    nodeComponentTable["getPosition"] = &Components::NodeManager::getPosition;
    nodeComponentTable["setPosition"] = &Components::NodeManager::setPosition;
    nodeComponentTable["getOrientation"] =
        &Components::NodeManager::getOrientation;
    nodeComponentTable["setOrientation"] =
        &Components::NodeManager::setOrientation;
    nodeComponentTable["getSize"] = &Components::NodeManager::getSize;
    nodeComponentTable["setSize"] = &Components::NodeManager::setSize;
  }
}
}

void ScriptManager::init()
{
  _INTR_LOG_INFO("Inititializing Script Manager...");
  Dod::Resources::ResourceManagerBase<
      ScriptData, _INTR_MAX_SCRIPT_COUNT>::_initResourceManager();

  Dod::Resources::ResourceManagerEntry managerEntry;
  {
    managerEntry.createFunction = Resources::ScriptManager::createScript;
    managerEntry.destroyFunction = Resources::ScriptManager::destroyScript;
    managerEntry.createResourcesFunction =
        Resources::ScriptManager::createResources;
    managerEntry.destroyResourcesFunction =
        Resources::ScriptManager::destroyResources;
    managerEntry.getActiveResourceAtIndexFunction =
        Resources::ScriptManager::getActiveResourceAtIndex;
    managerEntry.getActiveResourceCountFunction =
        Resources::ScriptManager::getActiveResourceCount;
    managerEntry.loadFromSingleFileFunction =
        Resources::ScriptManager::loadFromSingleFile;
    managerEntry.saveToSingleFileFunction =
        Resources::ScriptManager::saveToSingleFile;
    managerEntry.resetToDefaultFunction =
        Resources::ScriptManager::resetToDefault;

    Application::_resourceManagerMapping[_N(Script)] = managerEntry;
  }

  Dod::PropertyCompilerEntry propertyEntry;
  {
    propertyEntry.compileFunction = Resources::ScriptManager::compileDescriptor;
    propertyEntry.initFunction = Resources::ScriptManager::initFromDescriptor;
    propertyEntry.ref = Dod::Ref();

    Application::_resourcePropertyCompilerMapping[_N(Script)] = propertyEntry;
  }

  setupLuaState(&_luaState);
}

void ScriptManager::createResources(const ScriptRefArray& p_Scripts)
{
  for (uint32_t i = 0u; i < static_cast<uint32_t>(p_Scripts.size()); ++i)
  {
    ScriptRef scriptRef = p_Scripts[i];

    const _INTR_STRING filePath = _scriptPath + _descScriptFileName(scriptRef);
    _INTR_FSTREAM in = _INTR_FSTREAM(filePath.c_str(), std::ios::in);

    bool success = false;
    if (in)
    {
      _INTR_OSTRINGSTREAM contents;
      contents << in.rdbuf();
      in.close();

      SOL_PROTECTED_EXEC(_luaState.script(contents.str().c_str()), success);
    }

    if (success)
    {
      _luaState["registerTickFunction"]((uint32_t)scriptRef._id,
                                        _luaState["tick"]);
      _luaState["registerOnCreateFunction"]((uint32_t)scriptRef._id,
                                            _luaState["onCreate"]);
      _luaState["registerOnDestroyFunction"]((uint32_t)scriptRef._id,
                                             _luaState["onDestroy"]);
    }
    else
    {
      _INTR_LOG_WARNING("Failed to load script '%s'...", filePath.c_str());
    }
  }
}

void ScriptManager::destroyResources(const ScriptRefArray& p_Scripts)
{
  for (uint32_t scriptIdx = 0u; scriptIdx < p_Scripts.size(); ++scriptIdx)
  {
    ScriptRef scriptRef = p_Scripts[scriptIdx];

    _luaState["methods"]["tick"][(uint32_t)scriptRef._id] = sol::nil;
    _luaState["methods"]["onCreate"][(uint32_t)scriptRef._id] = sol::nil;
    _luaState["methods"]["onDestroy"][(uint32_t)scriptRef._id] = sol::nil;
  }
}

void ScriptManager::callTickScript(ScriptRef p_Script, uint32_t p_InstanceId,
                                   float p_DeltaT)
{
  sol::optional<sol::function> func =
      _luaState["methods"]["tick"][(uint32_t)p_Script._id];
  if (func)
  {
    bool success = false;
    SOL_PROTECTED_EXEC((*func)(p_InstanceId, p_DeltaT), success);

    if (!success)
    {
      destroyResources(p_Script);
    }
  }
}

void ScriptManager::callOnCreate(ScriptRef p_Script, uint32_t p_InstanceId)
{
  sol::optional<sol::function> func =
      _luaState["methods"]["onCreate"][(uint32_t)p_Script._id];
  if (func)
  {
    bool success = false;
    SOL_PROTECTED_EXEC((*func)(p_InstanceId), success);

    if (!success)
    {
      destroyResources(p_Script);
    }
  }
}

void ScriptManager::callOnDestroy(ScriptRef p_Script, uint32_t p_InstanceId)
{
  sol::optional<sol::function> func =
      _luaState["methods"]["onDestroy"][(uint32_t)p_Script._id];
  if (func)
  {
    bool success = false;
    SOL_PROTECTED_EXEC((*func)(p_InstanceId), success);

    if (!success)
    {
      destroyResources(p_Script);
    }
  }
}
}
}
}
