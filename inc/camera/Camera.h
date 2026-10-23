#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// Camera movement enum
enum Camera_Movement {
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT,
    UP,
    DOWN
};

const float YAW = -90.0f;
const float PITCH = 0.0f;
const float SPEED = 15.0f; // 2.5f
const float SENSITIVITY = 0.1f;
const float ZOOM = 45.0f; //45.0f


class Camera {
public:
    glm::vec3 Position;
    glm::vec3 Front;
    glm::vec3 Up;
    glm::vec3 Right;
    glm::vec3 WorldUp;

    float Yaw;
    float Pitch;
    float MovementSpeed;
    float MouseSensitivity;
    float Zoom;
    // Orthographic half-height (world units). Final ortho extents are OrthoSize * aspect horizontally.
    float OrthoSize = 10.0f;  // 10.0f
    float NearPlane = 0.1f;  // Default near clipping plane  
    float FarPlane = 2000.0f; // Default far clipping plane 2000

    // Orbit camera parameters or play mode parameters
    enum class CameraMode
    {
        EditorOrbit,
        Play
    };

    //ameraMode Mode = CameraMode::EditorOrbit;
    CameraMode Mode = CameraMode::Play;


    enum class ProjectionMode { Perspective = 0, Orthographic = 1 };  // ortho vs perspective mode

    Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 3.0f), //3
        glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f),
        float yaw = YAW, float pitch = PITCH);

    glm::mat4 GetViewMatrix() const;
    glm::mat4 GetProjectionMatrix(float aspectRatio) const;

    void ProcessKeyboard(Camera_Movement direction, float deltaTime);
    void ProcessMouseMovement(float xoffset, float yoffset, GLboolean constrainPitch = true);
    void ProcessMouseScroll(float yoffset);
    // ######################################################## Ortho # Helpers ########################################################
    ProjectionMode Projection = ProjectionMode::Perspective;
    void SetOrthographicTopDown(const glm::vec3& center, float orthoHalfHeight, float heightAbove = 0.0f); // 25.0f
    void SetPerspectiveFromDefaults(); // optional helper to restore perspective defaults

    // Reset camera potition
    void SetPositionYawPitch(const glm::vec3& pos, float yaw, float pitch);
    void ResetToDefaults(const glm::vec3& pos = glm::vec3(0.0f, 0.0f, 3.0f),
        float yaw = YAW, float pitch = PITCH);
    // Focus the camera on a target point at a specified distance, maintaining current yaw and pitch
    void FocusOn(const glm::vec3& target, float distance);

    // ordit camera around a target point, maintaining current distance and pitch
    glm::vec3 Target = glm::vec3(0.0f);
    float OrbitDistance = 10.0f;

    void ProcessOrbit(float xOffset, float yOffset);
    void ProcessPan(float xOffset, float yOffset);
    void ProcessOrbitZoom(float amount);
    void MoveVertical(float amount);





private:
    void UpdateOrbitPosition();
    void updateCameraVectors();
};


