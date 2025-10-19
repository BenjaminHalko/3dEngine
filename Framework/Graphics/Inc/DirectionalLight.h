#pragma once

#include "Color.h"

namespace Engine::Graphics
{
	struct DirectionalLight
	{
		Color ambient = Colors::White;
		Color diffuse = Colors::White;
		Color specular = Colors::White;
		Math::Vector3 direction = Math::Vector3::ZAxis;
		float padding = 0.0f;
	};
}
