///////////////////////////////////////////////////////////////////////////////
// SceneManager.cpp
// ============
// manage the loading and rendering of 3D scenes
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
// Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <iostream>

// declaration of global variables
namespace
{
	const char* g_ModelName = "model";
	const char* g_ColorValueName = "objectColor";
	const char* g_TextureValueName = "objectTexture";
	const char* g_UseTextureName = "bUseTexture";
	const char* g_UseLightingName = "bUseLighting";
}

/***********************************************************
 *  SceneManager()
 *
 *  The constructor for the class
 ***********************************************************/
SceneManager::SceneManager(ShaderManager* pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();
	m_loadedTextures = 0;
}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
	m_pShaderManager = NULL;
	delete m_basicMeshes;
	m_basicMeshes = NULL;
}

/***********************************************************
 *  CreateGLTexture()
 *
 *  This method is used for loading textures from image files,
 *  configuring the texture mapping parameters in OpenGL,
 *  generating the mipmaps, and loading the read texture into
 *  the next available texture slot in memory.
 ***********************************************************/
bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
	int width = 0;
	int height = 0;
	int colorChannels = 0;
	GLuint textureID = 0;

	// indicate to always flip images vertically when loaded
	stbi_set_flip_vertically_on_load(true);

	// try to parse the image data from the specified image file
	unsigned char* image = stbi_load(
		filename,
		&width,
		&height,
		&colorChannels,
		0);

	// if the image was successfully read from the image file
	if (image)
	{
		std::cout << "Successfully loaded image:" << filename << ", width:" << width << ", height:" << height << ", channels:" << colorChannels << std::endl;

		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// if the loaded image is in RGB format
		if (colorChannels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		// if the loaded image is in RGBA format - it supports transparency
		else if (colorChannels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			std::cout << "Not implemented to handle image with " << colorChannels << " channels" << std::endl;
			return false;
		}

		// generate the texture mipmaps for mapping textures to lower resolutions
		glGenerateMipmap(GL_TEXTURE_2D);

		// free the image data from local memory
		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0);

		// register the loaded texture and associate it with the special tag string
		m_textureIDs[m_loadedTextures].ID = textureID;
		m_textureIDs[m_loadedTextures].tag = tag;
		m_loadedTextures++;

		return true;
	}

	std::cout << "Could not load image:" << filename << std::endl;

	// Error loading the image
	return false;
}

/***********************************************************
 *  BindGLTextures()
 *
 *  This method is used for binding the loaded textures to
 *  OpenGL texture memory slots.  There are up to 16 slots.
 ***********************************************************/
void SceneManager::BindGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		// bind textures on corresponding texture units
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  DestroyGLTextures()
 *
 *  This method is used for freeing the memory in all the
 *  used texture memory slots.
 ***********************************************************/
void SceneManager::DestroyGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		glDeleteTextures(1, &m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  FindTextureID()
 *
 *  This method is used for getting an ID for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureID(std::string tag)
{
	int textureID = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureID = m_textureIDs[index].ID;
			bFound = true;
		}
		else
			index++;
	}

	return(textureID);
}

/***********************************************************
 *  FindTextureSlot()
 *
 *  This method is used for getting a slot index for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureSlot(std::string tag)
{
	int textureSlot = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureSlot = index;
			bFound = true;
		}
		else
			index++;
	}

	return(textureSlot);
}

/***********************************************************
 *  FindMaterial()
 *
 *  This method is used for getting a material from the previously
 *  defined materials list that is associated with the passed in tag.
 ***********************************************************/
bool SceneManager::FindMaterial(std::string tag, OBJECT_MATERIAL& material)
{
	if (m_objectMaterials.size() == 0)
	{
		return(false);
	}

	int index = 0;
	bool bFound = false;
	while ((index < (int)m_objectMaterials.size()) && (bFound == false))
	{
		if (m_objectMaterials[index].tag.compare(tag) == 0)
		{
			bFound = true;
			material.ambientColor = m_objectMaterials[index].ambientColor;
			material.ambientStrength = m_objectMaterials[index].ambientStrength;
			material.diffuseColor = m_objectMaterials[index].diffuseColor;
			material.specularColor = m_objectMaterials[index].specularColor;
			material.shininess = m_objectMaterials[index].shininess;
		}
		else
		{
			index++;
		}
	}

	return(bFound);
}

/***********************************************************
 *  SetTransformations()
 *
 *  This method is used for setting the transform buffer
 *  using the passed in transformation values.
 ***********************************************************/
void SceneManager::SetTransformations(
	glm::vec3 scaleXYZ,
	float XrotationDegrees,
	float YrotationDegrees,
	float ZrotationDegrees,
	glm::vec3 positionXYZ)
{
	// variables for this method
	glm::mat4 modelView;
	glm::mat4 scale;
	glm::mat4 rotationX;
	glm::mat4 rotationY;
	glm::mat4 rotationZ;
	glm::mat4 translation;

	// set the scale value in the transform buffer
	scale = glm::scale(scaleXYZ);
	// set the rotation values in the transform buffer
	rotationX = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
	rotationY = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
	rotationZ = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
	// set the translation value in the transform buffer
	translation = glm::translate(positionXYZ);

	modelView = translation * rotationX * rotationY * rotationZ * scale;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ModelName, modelView);
	}
}

/***********************************************************
 *  SetShaderColor()
 *
 *  This method is used for setting the passed in color
 *  into the shader for the next draw command
 ***********************************************************/
void SceneManager::SetShaderColor(
	float redColorValue,
	float greenColorValue,
	float blueColorValue,
	float alphaValue)
{
	// variables for this method
	glm::vec4 currentColor;

	currentColor.r = redColorValue;
	currentColor.g = greenColorValue;
	currentColor.b = blueColorValue;
	currentColor.a = alphaValue;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, false);
		m_pShaderManager->setVec4Value(g_ColorValueName, currentColor);
	}
}

/***********************************************************
 *  SetShaderTexture()
 *
 *  This method is used for setting the texture data
 *  associated with the passed in ID into the shader.
 ***********************************************************/
void SceneManager::SetShaderTexture(
	std::string textureTag)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, true);

		int textureID = -1;
		textureID = FindTextureSlot(textureTag);
		m_pShaderManager->setSampler2DValue(g_TextureValueName, textureID);
	}
}

/***********************************************************
 *  SetTextureUVScale()
 *
 *  This method is used for setting the texture UV scale
 *  values into the shader.
 ***********************************************************/
void SceneManager::SetTextureUVScale(float u, float v)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setVec2Value("UVscale", glm::vec2(u, v));
	}
}

/***********************************************************
 *  SetShaderMaterial()
 *
 *  This method is used for passing the material values
 *  into the shader.
 ***********************************************************/
void SceneManager::SetShaderMaterial(
	std::string materialTag)
{
	if (m_objectMaterials.size() > 0)
	{
		OBJECT_MATERIAL material;
		bool bReturn = false;

		bReturn = FindMaterial(materialTag, material);
		if (bReturn == true)
		{
			m_pShaderManager->setVec3Value("material.ambientColor", material.ambientColor);
			m_pShaderManager->setFloatValue("material.ambientStrength", material.ambientStrength);
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
		}
	}
}

/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
/**************************************************************/

/***********************************************************
 *  PrepareScene()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene
 *  rendering
 ***********************************************************/
void SceneManager::PrepareScene()
{
	// Load blueberry texture
	bool bSuccess = CreateGLTexture("textures/blueberry_v1.1.jpg", "blueberry");

	// Load whipped cream texture
	bSuccess = CreateGLTexture(
		"textures/whipped_cream2.jpg",
		"whipped_cream");

	// Load strawberry texture
	bSuccess = CreateGLTexture(
		"textures/strawberry1.jpg",
		"strawberry");

	// Load carrot cake texture
	bSuccess = CreateGLTexture(
		"textures/carrot_cake.jpg",
		"carrot_cake");

	// Load frosting layer texture
	bSuccess = CreateGLTexture(
		"textures/frosting1.jpg",
		"frosting");

	// Load plate texture
	bSuccess = CreateGLTexture(
		"textures/plate.jpg",
		"plate");

	// Load tablecloth texture
	bSuccess = CreateGLTexture(
		"textures/tablecloth.jpg",
		"tablecloth");

	// Load caramel texture
	bSuccess = CreateGLTexture("textures/caramel.jpg", "caramel");

	// after the texture image data is loaded into memory, the
	// loaded textures need to be bound to texture slots
	BindGLTextures();

	DefineObjectMaterials();  // Define materials for lighting
	SetupSceneLights();       // Setup the light sources

	// The meshes needed for the cake slice
	m_basicMeshes->LoadPlaneMesh();      // table surface
	m_basicMeshes->LoadPrismMesh();      // Cake layers
	m_basicMeshes->LoadBoxMesh();        // Frosting layers
	m_basicMeshes->LoadCylinderMesh();   // For the plate
	m_basicMeshes->LoadSphereMesh();     // Blueberries and whipped cream
}

/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()
{
	// Declare variables for transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	// RENDER TABLE SURFACE (Ground Plane)
	scaleXYZ = glm::vec3(20.0f, 1.0f, 15.0f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(0.0f, 0.0f, 0.0f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("tablecloth");
	SetShaderMaterial("table");
	SetTextureUVScale(3.0f, 3.0f);
	m_basicMeshes->DrawPlaneMesh();

	// RENDER DESSERT PLATE
	scaleXYZ = glm::vec3(4.2f, 0.1f, 4.0f);  
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(0.0f, 0.1f, 0.0f);  

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("plate");
	SetShaderMaterial("plate");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawCylinderMesh();

	// CAKE BASE LAYER 
	scaleXYZ = glm::vec3(1.5f, 0.8f, 4.0f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = -10.0f;
	ZrotationDegrees = -90.0f;
	positionXYZ = glm::vec3(-2.55f, 0.35f, 0.09f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("carrot_cake");
	SetShaderMaterial("cake");
	SetTextureUVScale(2.0f, 2.0f);
	m_basicMeshes->DrawPrismMesh();

	// FROSTING LAYER 1
	scaleXYZ = glm::vec3(1.45f, 0.095f, 4.0f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = -10.0f;
	ZrotationDegrees = -90.0f;
	positionXYZ = glm::vec3(-2.10f, 0.35f, 0.09f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("frosting");
	SetShaderMaterial("frosting");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawPrismMesh();

	// CAKE MIDDLE LAYER
	scaleXYZ = glm::vec3(1.5f, 0.7f, 4.0f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = -10.0f;
	ZrotationDegrees = -90.0f;
	positionXYZ = glm::vec3(-1.70f, 0.35f, 0.09f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("carrot_cake");
	SetShaderMaterial("cake");
	SetTextureUVScale(2.0f, 2.0f);
	m_basicMeshes->DrawPrismMesh();

	// FROSTING LAYER 2
	scaleXYZ = glm::vec3(1.45f, 0.095f, 4.0f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = -10.0f;
	ZrotationDegrees = -90.0f;
	positionXYZ = glm::vec3(-1.30f, 0.35f, 0.09f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("frosting");
	SetShaderMaterial("frosting");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawPrismMesh();

	// CAKE TOP LAYER
	scaleXYZ = glm::vec3(1.5f, 0.6f, 4.0f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = -10.0f;
	ZrotationDegrees = -90.0f;
	positionXYZ = glm::vec3(-0.95f, 0.35f, 0.09f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("carrot_cake");
	SetShaderMaterial("cake");
	SetTextureUVScale(2.0f, 2.0f);
	m_basicMeshes->DrawPrismMesh();

	// FROSTING LAYER TOP CAP on left side
	scaleXYZ = glm::vec3(1.45f, 0.10f, 4.0f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = -10.0f;
	ZrotationDegrees = -90.0f;
	positionXYZ = glm::vec3(-3.00f, 0.36f, 0.09f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("frosting");
	SetShaderMaterial("frosting");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawPrismMesh();

	// FROSTING BACK SIDE thin rectangle
	scaleXYZ = glm::vec3(2.40f, 1.60f, 0.4f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = -1.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(-1.5f, 0.35f, -1.93f); 

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("frosting");
	SetShaderMaterial("frosting");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawBoxMesh();

	// whipped cream
	// BASE WHIPPED CREAM 
	scaleXYZ = glm::vec3(0.8f, 0.25f, 0.7f);  
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(-0.85f, 0.28f, 2.8f);  

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("whipped_cream");
	SetShaderMaterial("cream");
	SetTextureUVScale(3.0f, 3.0f);
	m_basicMeshes->DrawSphereMesh();

	// MIDDLE LAYER
	scaleXYZ = glm::vec3(0.6f, 0.3f, 0.55f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 15.0f;  
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(-1.0f, 0.4f, 2.75f);  

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("whipped_cream");
	SetShaderMaterial("cream");
	SetTextureUVScale(3.0f, 3.0f);
	m_basicMeshes->DrawSphereMesh();

	// TOP PEAK - small peak 
	scaleXYZ = glm::vec3(0.35f, 0.4f, 0.3f);  
	XrotationDegrees = 10.0f;
	YrotationDegrees = -20.0f;
	ZrotationDegrees = 5.0f;
	positionXYZ = glm::vec3(-1.0f, 0.55f, 2.7f); 

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("whipped_cream");
	SetShaderMaterial("cream");
	SetTextureUVScale(3.0f, 3.0f);
	m_basicMeshes->DrawSphereMesh();

	// Strawberry
	scaleXYZ = glm::vec3(0.5f, 0.45f, 0.30f);
	XrotationDegrees = -5.0f;
	YrotationDegrees = -45.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(0.02f, 0.35f, -0.8f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("strawberry");
	SetShaderMaterial("berry");
	SetTextureUVScale(2.0f, 3.3f);
	m_basicMeshes->DrawSphereMesh();

	// Blueberry 1 - Top left touching pair (first berry)
	scaleXYZ = glm::vec3(0.18f, 0.18f, 0.18f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(-3.35f, 0.35f, 0.2f); 

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("blueberry");
	SetShaderMaterial("berry");
	SetTextureUVScale(0.6f, 0.6f);
	m_basicMeshes->DrawSphereMesh();

	// Blueberry 2 - Top left touching pair (second berry, touching the first)
	scaleXYZ = glm::vec3(0.17f, 0.17f, 0.17f);
	positionXYZ = glm::vec3(-3.35F, 0.35f, 0.7f); 

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("blueberry");
	SetShaderMaterial("berry");
	SetTextureUVScale(0.8f, 0.8f);
	m_basicMeshes->DrawSphereMesh();

	// Blueberry 3 - In front of cake(left-side)
	scaleXYZ = glm::vec3(0.2f, 0.2f, 0.2f);
	positionXYZ = glm::vec3(-2.8f, 0.3f, 2.5f);  

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("blueberry");
	SetShaderMaterial("berry");
	SetTextureUVScale(0.9f, 0.9f);
	m_basicMeshes->DrawSphereMesh();

	// Blueberry 4 - center plate below number 5
	scaleXYZ = glm::vec3(0.18f, 0.18f, 0.18f);
	positionXYZ = glm::vec3(0.5f, 0.3f, 1.0);  

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("blueberry");
	SetShaderMaterial("berry");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawSphereMesh();

	// Blueberry 5 - center plate above number 4
	scaleXYZ = glm::vec3(0.17f, 0.17f, 0.17f);
	positionXYZ = glm::vec3(0.6f, 0.3f, 1.45f);  

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("blueberry");
	SetShaderMaterial("berry");
	SetTextureUVScale(1.2f, 1.2f);
	m_basicMeshes->DrawSphereMesh();

	// Blueberry 6 - bottom berry in the triangle formation top right 
	scaleXYZ = glm::vec3(0.16f, 0.16f, 0.16f);
	positionXYZ = glm::vec3(1.8f, 0.3f, -0.4f);  

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("blueberry");
	SetShaderMaterial("berry");
	SetTextureUVScale(1.1f, 1.1f);
	m_basicMeshes->DrawSphereMesh();

	// Blueberry 7 - left berry in the triangle formation top right 
	scaleXYZ = glm::vec3(0.19f, 0.19f, 0.19f);
	positionXYZ = glm::vec3(1.5f, 0.3f, -1.2f);  

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("blueberry");
	SetShaderMaterial("berry");
	SetTextureUVScale(1.25f, 1.25f);
	m_basicMeshes->DrawSphereMesh();

	// Blueberry 8 - right berry in the triangle formation top right 
	scaleXYZ = glm::vec3(0.15f, 0.15f, 0.15f);
	positionXYZ = glm::vec3(2.2, 0.3f, -1.1f);  

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("blueberry");
	SetShaderMaterial("berry");
	SetTextureUVScale(0.3f, 0.3f);
	m_basicMeshes->DrawSphereMesh();

	// Blueberry 9 - right behind whipped cream(barely visible)
	scaleXYZ = glm::vec3(0.15f, 0.15f, 0.15f);
	positionXYZ = glm::vec3(-0.5f, 0.3f, 2.0f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("blueberry");
	SetShaderMaterial("berry");
	SetTextureUVScale(0.3f, 0.3f);
	m_basicMeshes->DrawSphereMesh();

	// Caramel Drizzle lines
	// Drizzle line 1
	scaleXYZ = glm::vec3(5.6f, 0.065f, 0.085f); 
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.8f;  
	positionXYZ = glm::vec3(0.1f, 0.18f, 1.4f);  

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("caramel");
	SetShaderMaterial("caramel");
	SetTextureUVScale(1.0f, 7.8f);
	m_basicMeshes->DrawBoxMesh();

	// End caps for line 1
	scaleXYZ = glm::vec3(0.120f, 0.100f, 0.120f);
	positionXYZ = glm::vec3(-2.7f, 0.21f, 1.37f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderTexture("caramel");
	SetShaderMaterial("caramel");
	m_basicMeshes->DrawSphereMesh();

	scaleXYZ = glm::vec3(0.120f, 0.100f, 0.120f);
	positionXYZ = glm::vec3(2.9f, 0.21f, 1.43f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderTexture("caramel");
	SetShaderMaterial("caramel");
	m_basicMeshes->DrawSphereMesh();

	// Drizzle line 2
	scaleXYZ = glm::vec3(5.4f, 0.065f, 0.085f);
	ZrotationDegrees = -0.6f;  
	positionXYZ = glm::vec3(0.2f, 0.18f, 0.9f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("caramel");
	SetShaderMaterial("caramel");
	SetTextureUVScale(1.0f, 7.6f);
	m_basicMeshes->DrawBoxMesh();

	// End caps for line 2
	scaleXYZ = glm::vec3(0.115f, 0.100f, 0.115f);
	positionXYZ = glm::vec3(-2.5f, 0.21f, 0.92f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderTexture("caramel");
	SetShaderMaterial("caramel");
	m_basicMeshes->DrawSphereMesh();

	scaleXYZ = glm::vec3(0.115f, 0.100f, 0.115f);
	positionXYZ = glm::vec3(2.9f, 0.21f, 0.88f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderTexture("caramel");
	SetShaderMaterial("caramel");
	m_basicMeshes->DrawSphereMesh();

	// Drizzle line 3
	scaleXYZ = glm::vec3(5.2f, 0.065f, 0.085f);
	ZrotationDegrees = 0.4f;
	positionXYZ = glm::vec3(-0.1f, 0.18f, 0.4f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("caramel");
	SetShaderMaterial("caramel");
	SetTextureUVScale(1.0f, 7.4f);
	m_basicMeshes->DrawBoxMesh();

	// End caps for line 3
	scaleXYZ = glm::vec3(0.112f, 0.100f, 0.112f);
	positionXYZ = glm::vec3(-2.7f, 0.21f, 0.38f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderTexture("caramel");
	SetShaderMaterial("caramel");
	m_basicMeshes->DrawSphereMesh();

	scaleXYZ = glm::vec3(0.112f, 0.100f, 0.112f);
	positionXYZ = glm::vec3(2.5f, 0.21f, 0.42f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderTexture("caramel");
	SetShaderMaterial("caramel");
	m_basicMeshes->DrawSphereMesh();

	// Drizzle line 4
	scaleXYZ = glm::vec3(5.0f, 0.065f, 0.085f);
	ZrotationDegrees = -0.3f;
	positionXYZ = glm::vec3(0.3f, 0.18f, -0.1f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("caramel");
	SetShaderMaterial("caramel");
	SetTextureUVScale(1.0f, 7.0f);
	m_basicMeshes->DrawBoxMesh();

	// End caps for line 4
	scaleXYZ = glm::vec3(0.110f, 0.100f, 0.110f);
	positionXYZ = glm::vec3(-2.2f, 0.21f, -0.08f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderTexture("caramel");
	SetShaderMaterial("caramel");
	m_basicMeshes->DrawSphereMesh();

	scaleXYZ = glm::vec3(0.110f, 0.100f, 0.110f);
	positionXYZ = glm::vec3(2.8f, 0.21f, -0.12f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderTexture("caramel");
	SetShaderMaterial("caramel");
	m_basicMeshes->DrawSphereMesh();

	// Drizzle line 5
	scaleXYZ = glm::vec3(4.8f, 0.065f, 0.085f);
	ZrotationDegrees = 0.7f;
	positionXYZ = glm::vec3(-0.3f, 0.18f, -0.6f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("caramel");
	SetShaderMaterial("caramel");
	SetTextureUVScale(1.0f, 6.8f);
	m_basicMeshes->DrawBoxMesh();

	// End caps for line 5
	scaleXYZ = glm::vec3(0.108f, 0.100f, 0.108f);
	positionXYZ = glm::vec3(-2.7f, 0.21f, -0.64f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderTexture("caramel");
	SetShaderMaterial("caramel");
	m_basicMeshes->DrawSphereMesh();

	scaleXYZ = glm::vec3(0.108f, 0.100f, 0.108f);
	positionXYZ = glm::vec3(2.1f, 0.21f, -0.56f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderTexture("caramel");
	SetShaderMaterial("caramel");
	m_basicMeshes->DrawSphereMesh();

	//Drizzle line 6 
	scaleXYZ = glm::vec3(4.6f, 0.065f, 0.085f);
	ZrotationDegrees = -0.5f;
	positionXYZ = glm::vec3(0.4f, 0.18f, -1.1f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("caramel");
	SetShaderMaterial("caramel");
	SetTextureUVScale(1.0f, 6.6f);
	m_basicMeshes->DrawBoxMesh();

	// End caps for line 6
	scaleXYZ = glm::vec3(0.106f, 0.100f, 0.106f);
	positionXYZ = glm::vec3(-1.9f, 0.21f, -1.08f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderTexture("caramel");
	SetShaderMaterial("caramel");
	m_basicMeshes->DrawSphereMesh();

	scaleXYZ = glm::vec3(0.106f, 0.100f, 0.106f);
	positionXYZ = glm::vec3(2.7f, 0.21f, -1.12f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderTexture("caramel");
	SetShaderMaterial("caramel");
	m_basicMeshes->DrawSphereMesh();
}

/***********************************************************
 *  DefineObjectMaterials()
 *
 *  This method is used for configuring the various material
 *  settings for all of the objects within the 3D scene.
 ***********************************************************/
void SceneManager::DefineObjectMaterials()
{
	// CAKE MATERIAL - Slightly glossy
	OBJECT_MATERIAL cakeMaterial;
	cakeMaterial.ambientStrength = 0.2f;
	cakeMaterial.ambientColor = glm::vec3(0.1f, 0.05f, 0.02f);    
	cakeMaterial.diffuseColor = glm::vec3(0.8f, 0.6f, 0.4f);    
	cakeMaterial.specularColor = glm::vec3(0.2f, 0.2f, 0.2f);
	cakeMaterial.shininess = 4.0f;                                
	cakeMaterial.tag = "cake";
	m_objectMaterials.push_back(cakeMaterial);

	// FROSTING MATERIAL - Smooth, more reflective than cake
	OBJECT_MATERIAL frostingMaterial;
	frostingMaterial.ambientStrength = 0.3f;
	frostingMaterial.ambientColor = glm::vec3(0.15f, 0.15f, 0.12f);
	frostingMaterial.diffuseColor = glm::vec3(0.9f, 0.9f, 0.8f); 
	frostingMaterial.specularColor = glm::vec3(0.4f, 0.4f, 0.4f);
	frostingMaterial.shininess = 16.0f;                          
	frostingMaterial.tag = "frosting";
	m_objectMaterials.push_back(frostingMaterial);

	// BERRY MATERIAL - For blueberries and strawberries
	OBJECT_MATERIAL berryMaterial;
	berryMaterial.ambientStrength = 0.25f;
	berryMaterial.ambientColor = glm::vec3(0.08f, 0.02f, 0.08f);
	berryMaterial.diffuseColor = glm::vec3(0.6f, 0.3f, 0.7f);    
	berryMaterial.specularColor = glm::vec3(0.5f, 0.5f, 0.5f);   
	berryMaterial.shininess = 32.0f;
	berryMaterial.tag = "berry";
	m_objectMaterials.push_back(berryMaterial);

	// CREAM MATERIAL - For whipped cream (very smooth and reflective)
	OBJECT_MATERIAL creamMaterial;
	creamMaterial.ambientStrength = 0.3f;
	creamMaterial.ambientColor = glm::vec3(0.2f, 0.2f, 0.18f);
	creamMaterial.diffuseColor = glm::vec3(0.95f, 0.95f, 0.9f);
	creamMaterial.specularColor = glm::vec3(0.6f, 0.6f, 0.6f);   
	creamMaterial.shininess = 64.0f;                              
	creamMaterial.tag = "cream";
	m_objectMaterials.push_back(creamMaterial);

	// PLATE MATERIAL - Ceramic with moderate reflectivity
	OBJECT_MATERIAL plateMaterial;
	plateMaterial.ambientStrength = 0.2f;
	plateMaterial.ambientColor = glm::vec3(0.1f, 0.1f, 0.1f);
	plateMaterial.diffuseColor = glm::vec3(0.8f, 0.8f, 0.8f);    
	plateMaterial.specularColor = glm::vec3(0.3f, 0.3f, 0.3f);
	plateMaterial.shininess = 24.0f;
	plateMaterial.tag = "plate";
	m_objectMaterials.push_back(plateMaterial);

	// TABLE MATERIAL - Fabric tablecloth (low reflectivity)
	OBJECT_MATERIAL tableMaterial;
	tableMaterial.ambientStrength = 0.15f;
	tableMaterial.ambientColor = glm::vec3(0.05f, 0.05f, 0.05f);
	tableMaterial.diffuseColor = glm::vec3(0.4f, 0.4f, 0.4f);    
	tableMaterial.specularColor = glm::vec3(0.1f, 0.1f, 0.1f);   
	tableMaterial.shininess = 2.0f;                               
	tableMaterial.tag = "table";
	m_objectMaterials.push_back(tableMaterial);

	// LEAF MATERIAL - For strawberry leaves (matte green)
	OBJECT_MATERIAL leafMaterial;
	leafMaterial.ambientStrength = 0.2f;
	leafMaterial.ambientColor = glm::vec3(0.02f, 0.08f, 0.02f);
	leafMaterial.diffuseColor = glm::vec3(0.2f, 0.6f, 0.2f);     
	leafMaterial.specularColor = glm::vec3(0.1f, 0.2f, 0.1f);   
	leafMaterial.shininess = 4.0f;                              
	leafMaterial.tag = "leaf";
	m_objectMaterials.push_back(leafMaterial);

	// CARAMEL MATERIAL - Glossy, golden
	OBJECT_MATERIAL caramelMaterial;
	caramelMaterial.ambientStrength = 0.3f;
	caramelMaterial.ambientColor = glm::vec3(0.15f, 0.1f, 0.05f);
	caramelMaterial.diffuseColor = glm::vec3(0.9f, 0.6f, 0.2f);    
	caramelMaterial.specularColor = glm::vec3(0.8f, 0.7f, 0.6f);   
	caramelMaterial.shininess = 64.0f;                           
	caramelMaterial.tag = "caramel";
	m_objectMaterials.push_back(caramelMaterial);
}

/***********************************************************
 *  SetupSceneLights()
 *
 *  This method is called to add and configure the light
 *  sources for the 3D scene.
 ***********************************************************/
void SceneManager::SetupSceneLights()
{
	// Enable lighting
	m_pShaderManager->setBoolValue(g_UseLightingName, true);

	// MAIN LIGHT - Warm kitchen lighting from above-right
	m_pShaderManager->setVec3Value("pointLights[0].position", 6.0f, 12.0f, 4.0f);
	m_pShaderManager->setVec3Value("pointLights[0].ambient", 0.15f, 0.12f, 0.1f);    
	m_pShaderManager->setVec3Value("pointLights[0].diffuse", 0.9f, 0.8f, 0.7f);      
	m_pShaderManager->setVec3Value("pointLights[0].specular", 0.6f, 0.6f, 0.5f);    
	m_pShaderManager->setBoolValue("pointLights[0].bActive", true);

	// ACCENT LIGHT - Soft blue light from the left
	m_pShaderManager->setVec3Value("pointLights[1].position", -8.0f, 8.0f, 2.0f);
	m_pShaderManager->setVec3Value("pointLights[1].ambient", 0.05f, 0.08f, 0.12f);   
	m_pShaderManager->setVec3Value("pointLights[1].diffuse", 0.3f, 0.4f, 0.6f);      
	m_pShaderManager->setVec3Value("pointLights[1].specular", 0.2f, 0.3f, 0.4f);     
	m_pShaderManager->setBoolValue("pointLights[1].bActive", true);

	// FILL LIGHT - fill light from front-right
	m_pShaderManager->setVec3Value("pointLights[2].position", 4.0f, 6.0f, 8.0f);
	m_pShaderManager->setVec3Value("pointLights[2].ambient", 0.08f, 0.08f, 0.08f);   
	m_pShaderManager->setVec3Value("pointLights[2].diffuse", 0.4f, 0.4f, 0.4f);      
	m_pShaderManager->setVec3Value("pointLights[2].specular", 0.2f, 0.2f, 0.2f);    
	m_pShaderManager->setBoolValue("pointLights[2].bActive", true);

	// Disable remaining lights
	for (int i = 3; i < 5; i++)
	{
		m_pShaderManager->setBoolValue("pointLights[" + std::to_string(i) + "].bActive", false);
	}

	// Disable directional and spot lights
	m_pShaderManager->setBoolValue("directionalLight.bActive", false);
	m_pShaderManager->setBoolValue("spotLight.bActive", false);
}
/****************************************************************/