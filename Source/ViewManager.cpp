///////////////////////////////////////////////////////////////////////////////
// viewmanager.cpp
// ============
// manage the viewing of 3D objects within the viewport
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "ViewManager.h"

// GLM Math Header inclusions
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>    
#include <iostream>

// declaration of the global variables and defines
namespace
{
	// Variables for window width and height
	const int WINDOW_WIDTH = 1000;
	const int WINDOW_HEIGHT = 800;
	const char* g_ViewName = "view";
	const char* g_ProjectionName = "projection";

	// Camera position and orientation vectors
	glm::vec3 cameraPos = glm::vec3(0.0f, 3.0f, 12.0f);
	glm::vec3 cameraFront = glm::vec3(0.0f, -0.2f, -1.0f);
	glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

	// Mouse input variables
	float lastX = WINDOW_WIDTH / 2.0f;
	float lastY = WINDOW_HEIGHT / 2.0f;
	bool firstMouse = true;

	// Camera rotation angles
	float yaw = -90.0f;
	float pitch = 0.0f;
	float fov = 45.0f;

	// Frame timing variables
	float deltaTime = 0.0f;
	float lastFrame = 0.0f;

	// Movement speed variable - controlled by mouse scroll
	float movementSpeed = 2.5f;

	// Projection mode toggle (assignment requirement)
	bool orthographicProjection = false;
	bool pKeyPressed = false;
	bool oKeyPressed = false;
}

/***********************************************************
 *  ViewManager()
 *
 *  The constructor for the class
 ***********************************************************/
ViewManager::ViewManager(ShaderManager* pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_pWindow = NULL;
}

/***********************************************************
 *  ~ViewManager()
 *
 *  The destructor for the class
 ***********************************************************/
ViewManager::~ViewManager()
{
	m_pShaderManager = NULL;
	m_pWindow = NULL;
}

/***********************************************************
 *  CreateDisplayWindow()
 *
 *  This method is used to create the main display window.
 ***********************************************************/
GLFWwindow* ViewManager::CreateDisplayWindow(const char* windowTitle)
{
	GLFWwindow* window = nullptr;

	window = glfwCreateWindow(
		WINDOW_WIDTH,
		WINDOW_HEIGHT,
		windowTitle,
		NULL, NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return NULL;
	}
	glfwMakeContextCurrent(window);

	// Set mouse callback for camera orientation control
	glfwSetCursorPosCallback(window, &ViewManager::Mouse_Position_Callback);

	// Set scroll callback for movement speed control
	glfwSetScrollCallback(window, &ViewManager::Mouse_Scroll_Callback);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	m_pWindow = window;

	return(window);
}

/***********************************************************
 *  Mouse_Position_Callback()
 *
 *  This method is automatically called from GLFW whenever
 *  the mouse is moved within the active GLFW display window.
 ***********************************************************/
void ViewManager::Mouse_Position_Callback(GLFWwindow* window, double xpos, double ypos)
{
	// Handle first mouse movement to prevent camera jump
	if (firstMouse)
	{
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	// Calculate mouse movement offsets
	float xoffset = xpos - lastX;
	float yoffset = lastY - ypos;
	lastX = xpos;
	lastY = ypos;

	// Apply mouse sensitivity
	const float sensitivity = 0.1f;
	xoffset *= sensitivity;
	yoffset *= sensitivity;

	// Update camera angles
	yaw += xoffset;
	pitch += yoffset;

	// Constrain pitch to prevent camera flipping
	if (pitch > 89.0f)
		pitch = 89.0f;
	if (pitch < -89.0f)
		pitch = -89.0f;

	// Calculate new camera direction from angles
	glm::vec3 direction;
	direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
	direction.y = sin(glm::radians(pitch));
	direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
	cameraFront = glm::normalize(direction);
}

/***********************************************************
 *  Mouse_Scroll_Callback()
 *
 *  This method is automatically called from GLFW whenever
 *  the mouse scroll wheel is used within the active GLFW display window.
 *  Controls the movement speed of the camera (REQUIRED FOR ASSIGNMENT)
 ***********************************************************/
void ViewManager::Mouse_Scroll_Callback(GLFWwindow* window, double xoffset, double yoffset)
{
	// Adjust movement speed based on scroll direction
	movementSpeed += (float)yoffset * 0.5f;

	// Clamp movement speed to reasonable bounds
	if (movementSpeed < 0.1f)
		movementSpeed = 0.1f;
	if (movementSpeed > 10.0f)
		movementSpeed = 10.0f;


}

/***********************************************************
 *  ProcessKeyboardEvents()
 *
 *  This method is called to process any keyboard events
 *  that may be waiting in the event queue.
 ***********************************************************/
void ViewManager::ProcessKeyboardEvents()
{
	// Close window on escape
	if (glfwGetKey(m_pWindow, GLFW_KEY_ESCAPE) == GLFW_PRESS)
	{
		glfwSetWindowShouldClose(m_pWindow, true);
	}

	// Calculate frame-rate independent movement speed using scroll-controlled speed
	const float cameraSpeed = movementSpeed * deltaTime;

	// WASD movement controls (assignment requirement)
	if (glfwGetKey(m_pWindow, GLFW_KEY_W) == GLFW_PRESS)
		cameraPos += cameraSpeed * cameraFront;
	if (glfwGetKey(m_pWindow, GLFW_KEY_S) == GLFW_PRESS)
		cameraPos -= cameraSpeed * cameraFront;
	if (glfwGetKey(m_pWindow, GLFW_KEY_A) == GLFW_PRESS)
		cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
	if (glfwGetKey(m_pWindow, GLFW_KEY_D) == GLFW_PRESS)
		cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;

	// QE movement controls (assignment requirement)
	if (glfwGetKey(m_pWindow, GLFW_KEY_Q) == GLFW_PRESS)
		cameraPos += cameraSpeed * cameraUp;
	if (glfwGetKey(m_pWindow, GLFW_KEY_E) == GLFW_PRESS)
		cameraPos -= cameraSpeed * cameraUp;

	// P key for Perspective projection (assignment requirement)
	if (glfwGetKey(m_pWindow, GLFW_KEY_P) == GLFW_PRESS)
	{
		if (!pKeyPressed)
		{
			orthographicProjection = false;
			pKeyPressed = true;
			std::cout << "Switched to Perspective Projection" << std::endl;
		}
	}
	else
	{
		pKeyPressed = false;
	}

	// O key for Orthographic projection (assignment requirement)
	if (glfwGetKey(m_pWindow, GLFW_KEY_O) == GLFW_PRESS)
	{
		if (!oKeyPressed)
		{
			orthographicProjection = true;
			oKeyPressed = true;
			std::cout << "Switched to Orthographic Projection" << std::endl;
		}
	}
	else
	{
		oKeyPressed = false;
	}
}

/***********************************************************
 *  PrepareSceneView()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene
 *  rendering
 ***********************************************************/
void ViewManager::PrepareSceneView()
{
	glm::mat4 view;
	glm::mat4 projection;

	// Calculate frame timing for smooth movement
	float currentFrame = glfwGetTime();
	deltaTime = currentFrame - lastFrame;
	lastFrame = currentFrame;

	// Process keyboard input
	ProcessKeyboardEvents();

	// Create view matrix using camera vectors
	view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);

	// Create projection matrix based on mode 
	if (orthographicProjection)
	{
		// Orthographic projection for 2D-style view
		float orthoSize = 15.0f;
		float aspectRatio = (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT;
		projection = glm::ortho(
			-orthoSize * aspectRatio / 2.0f,
			orthoSize * aspectRatio / 2.0f,
			-orthoSize / 2.0f,
			orthoSize / 2.0f,
			0.1f,
			100.0f
		);
	}
	else
	{
		// Perspective projection for 3D view
		projection = glm::perspective(glm::radians(fov), (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT, 0.1f, 100.0f);
	}

	// Send matrices to shader
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ViewName, view);
		m_pShaderManager->setMat4Value(g_ProjectionName, projection);
		m_pShaderManager->setVec3Value("viewPosition", cameraPos);
	}
}