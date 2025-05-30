#pragma once

namespace Engine::Graphics
{
	class Camera
	{
	public:
		enum class ProjectionMode
		{
			Perspective,
			Orthographic
		};

		void SetMode(ProjectionMode mode);

		void SetPosition(const Math::Vector3& position);

		void SetDirection(const Math::Vector3& direction);

		void SetLookAt(const Math::Vector3& target);

		// Perspective Params
		void SetFOV(float fov);
		void SetAspectRatio(float ratio);

		// Orthographic Params
		void SetSize(float width, float height);
		float GetSize() const;

		// Shared Internal
		void SetNearPlane(float nearPlane);
		void SetFarPlane(float farPlane);

		// Controls
		void Walk(float distance);
		void Strafe(float distance);
		void Rise(float distance);

		// Look Controls
		void Yaw(float radians);
		void Pitch(float radians);

		void Zoom(float amount);

		// Getters
		const Math::Vector3& GetPosition() const;
		const Math::Vector3& GetDirection() const;

		Math::Matrix4 GetViewMatrix() const;
		Math::Matrix4 GetProjectionMatrix() const;

		Math::Matrix4 GetPerspectiveMatrix() const;
		Math::Matrix4 GetOrthographicMatrix() const;
		
	private:
		ProjectionMode mProjectionMode = ProjectionMode::Perspective;

		Math::Vector3 mPosition = Math::Vector3::Zero;
		Math::Vector3 mDirection = Math::Vector3::ZAxis;

		float mFov = 60.0f * Math::Constants::DegToRad;
		float mAspectRatio = 0.0f;

		float mWidth = 0.0f;
		float mHeight = 0.0f;

		float mNearPlane = 0.01f;
		float mFarPlane = 1000.0f;
	};
}
