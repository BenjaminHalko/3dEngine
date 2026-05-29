#pragma once

#include "Common.h"

#include "App.h"
#include "AppState.h"
#include "SaveUtil.h"

#include "GameObject.h"
#include "GameWorld.h"
#include "GameObjectHandle.h"
#include "GameObjectFactory.h"

#include "TypeIds.h"
#include "Component.h"
#include "TransformComponent.h"
#include "CameraComponent.h"
#include "FPSCameraComponent.h"
#include "RenderObjectComponent.h"
#include "MeshComponent.h"
#include "ModelComponent.h"
#include "AnimatorComponent.h"
#include "RigidBodyComponent.h"
#include "SoundEventComponent.h"
#include "SoundBankComponent.h"
#include "UIComponent.h"
#include "UITextComponent.h"
#include "UISpriteComponent.h"
#include "UIButtonComponent.h"

#include "Service.h"
#include "CameraService.h"
#include "RenderService.h"
#include "PhysicsService.h"
#include "UIRenderService.h"

namespace Engine
{
class App;
App& MainApp();
} // namespace Engine
