///////////////////////////////////////////////////////////////////////////////
// scenemanager.cpp
// ============
// manage the preparing and rendering of 3D scenes - textures, materials, lighting
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"
#include <windows.h>
#include <iostream>

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include <glm/gtx/transform.hpp>

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
SceneManager::SceneManager(ShaderManager *pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();
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
	char buffer[MAX_PATH];
	GetCurrentDirectoryA(MAX_PATH, buffer);
	std::cout << "Runtime working directory: " << buffer << std::endl;

	int width = 0;
	int height = 0;
	int colorChannels = 0;
	GLuint textureID = 0;

	// indicate to always flip images vertically when loaded
	stbi_set_flip_vertically_on_load(true);

	std::cout << "Attemping to load image: " << filename << std::endl;

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
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

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
		glGenTextures(1, &m_textureIDs[i].ID);
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
	while ((index < m_objectMaterials.size()) && (bFound == false))
	{
		if (m_objectMaterials[index].tag.compare(tag) == 0)
		{
			bFound = true;
			material.diffuseColor = m_objectMaterials[index].diffuseColor;
			material.specularColor = m_objectMaterials[index].specularColor;
			material.shininess = m_objectMaterials[index].shininess;
		}
		else
		{
			index++;
		}
	}

	return(true);
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
	glm::vec3 positionXYZ,
	glm::vec3 offset)
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
	translation = glm::translate(positionXYZ + offset);

	modelView = translation * rotationZ * rotationY * rotationX * scale;

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

void SceneManager::DefineObjectMaterials()
{	
	// material for plane
	OBJECT_MATERIAL cementMaterial;
	cementMaterial.diffuseColor = glm::vec3(0.5f, 0.5f, 0.5f);
	cementMaterial.specularColor = glm::vec3(0.4f, 0.4f, 0.4f);
	cementMaterial.shininess = 0.5;
	cementMaterial.tag = "cement";
	m_objectMaterials.push_back(cementMaterial);

	// material for computer case
	OBJECT_MATERIAL plasticMaterial;
	plasticMaterial.diffuseColor = glm::vec3(1.0f, 1.0f, 1.0f);
	plasticMaterial.specularColor = glm::vec3(0.2f, 0.2f, 0.2f);
	plasticMaterial.shininess = 2.0;
	plasticMaterial.tag = "plastic";
	m_objectMaterials.push_back(plasticMaterial);
}

void SceneManager::SetupSceneLights()
{
	// this line of code is NEEDED for telling the shaders to render 
	// the 3D scene with custom lighting, if no light sources have
	// been added then the display window will be black - to use the 
	// default OpenGL lighting then comment out the following line
	//
	m_pShaderManager->setBoolValue(g_UseLightingName, true);

	// Point light directly above the computer
	m_pShaderManager->setVec3Value("pointLights[0].position", 0.0f, 9.5f, 2.5f);

	// White light: soft ambient point light over the scene
	m_pShaderManager->setVec3Value("pointLights[0].ambient", 0.15f, 0.15f, 0.15f);
	m_pShaderManager->setVec3Value("pointLights[0].diffuse", 0.6f, 0.6f, 0.6f);
	m_pShaderManager->setVec3Value("pointLights[0].specular", 0.5f, 0.5f, 0.5f);
	m_pShaderManager->setFloatValue("pointLights[0].constant", 1.0f);
	m_pShaderManager->setFloatValue("pointLights[0].linear", 0.045f);    // softer falloff
	m_pShaderManager->setFloatValue("pointLights[0].quadratic", 0.0075f); // wider reach

	// Activate the light
	m_pShaderManager->setBoolValue("pointLights[0].bActive", true);

	// CRT screen glow (soft blue)
	m_pShaderManager->setVec3Value("pointLights[1].position", -1.25f, 3.5f, 3.0f); // position just in front of CRT panel
	m_pShaderManager->setVec3Value("pointLights[1].ambient", 0.05f, 0.1f, 0.2f);   // subtle blue-ish ambient
	m_pShaderManager->setVec3Value("pointLights[1].diffuse", 0.2f, 0.4f, 0.8f);
	m_pShaderManager->setVec3Value("pointLights[1].specular", 0.1f, 0.2f, 0.4f); 
	m_pShaderManager->setFloatValue("pointLights[1].constant", 1.0f);
	m_pShaderManager->setFloatValue("pointLights[1].linear", 0.22f);
	m_pShaderManager->setFloatValue("pointLights[1].quadratic", 0.2f);

	// Activate the light
	m_pShaderManager->setBoolValue("pointLights[1].bActive", true);

	// Enable lighting system
	m_pShaderManager->setBoolValue(g_UseLightingName, true);
}


/***********************************************************
 *  PrepareScene()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene 
 *  rendering
 ***********************************************************/
void SceneManager::PrepareScene()
{
	// define the materials for objects in the scene
	DefineObjectMaterials();
	// add and define the light sources for the scene
	SetupSceneLights();
	
	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene

	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadBoxMesh();
	m_basicMeshes->LoadPrismMesh();
	m_basicMeshes->LoadCylinderMesh();

	// Load textures
	CreateGLTexture("textures/computer_case_texture_2.jpg", "ComputerCase");
	CreateGLTexture("textures/crt_on_texture_1.jpg", "CRTScreen");
	CreateGLTexture("textures/table_texture_1.jpeg", "TableTexture");
	BindGLTextures();
}

void SceneManager::DrawMainBody()
{
	glm::vec3 scaleXYZ = glm::vec3(7.0f, 5.0f, 5.0f);
	glm::vec3 positionXYZ = glm::vec3(0.0f, 3.5f, 0.0f);  // lift it off the ground
	SetTransformations(scaleXYZ, 0, 0, 0, positionXYZ);
	
	// Enable the texture
	SetShaderTexture("ComputerCase");
	SetShaderMaterial("plastic");
	SetTextureUVScale(2.0f, 2.0f);
	m_basicMeshes->DrawBoxMesh();
}

void SceneManager::DrawBackBase()
{
	glm::vec3 scaleXYZ = glm::vec3(7.0f, 2.5f, 3.0f);
	glm::vec3 positionXYZ = glm::vec3(0.0f, 1.25f, -1.0f);
	SetTransformations(scaleXYZ, 0, 0, 0, positionXYZ);
	
	SetShaderTexture("ComputerCase");
	SetShaderMaterial("plastic");
	SetTextureUVScale(2.0f, 2.0f);

	m_basicMeshes->DrawBoxMesh();
}

void SceneManager::DrawFeet()
{
	// ---- Left Front Foot Box ---- //
	glm::vec3 scaleXYZ = glm::vec3(1.0f, 1.0f, 0.2f);
	glm::vec3 positionXYZ = glm::vec3(-3.0f, 0.5f, 0.6f);
	SetTransformations(scaleXYZ, 0, 0, 0, positionXYZ);
	SetShaderTexture("ComputerCase");
	SetShaderMaterial("plastic");
	SetTextureUVScale(2.0f, 2.0f);
	m_basicMeshes->DrawBoxMesh();

	// ---- Left Front Foot ---- //
	scaleXYZ = glm::vec3(1.0f, 1.0f, 0.5f);
	positionXYZ = glm::vec3(-3.0f, 0.25f, 0.7f);
	SetTransformations(scaleXYZ, -90.0f, 90.0f, 0.0f, positionXYZ);
	SetShaderTexture("ComputerCase");
	SetShaderMaterial("plastic");
	SetTextureUVScale(2.0f, 2.0f);
	m_basicMeshes->DrawPrismMesh();

	// ---- Right Front Foot Box ---- //
	scaleXYZ = glm::vec3(1.0f, 1.0f, 0.2f);
	positionXYZ = glm::vec3(3.0f, 0.5f, 0.6f);
	SetTransformations(scaleXYZ, 0, 0, 0, positionXYZ);
	SetShaderTexture("ComputerCase");
	SetShaderMaterial("plastic");
	SetTextureUVScale(2.0f, 2.0f);
	m_basicMeshes->DrawBoxMesh();

	// ---- Right Front Foot Wedge ---- //
	scaleXYZ = glm::vec3(1.0f, 1.0f, 0.5f);
	positionXYZ = glm::vec3(3.0f, 0.25f, 0.7f);
	SetTransformations(scaleXYZ, -90.0f, 90.0f, 0.0f, positionXYZ);
	SetShaderTexture("ComputerCase");
	SetShaderMaterial("plastic");
	SetTextureUVScale(2.0f, 2.0f);
	m_basicMeshes->DrawPrismMesh();
}

void SceneManager::DrawFloppyDrive() 
{
	// ---- Floppy Drive Indentation ---- //
	glm::vec3 scaleXYZ = glm::vec3(2.0f, 2.5f, 0.2f);
	glm::vec3 positionXYZ = glm::vec3(2.0f, 3.5f, 2.6f);
	SetTransformations(scaleXYZ, 0, 0, 0, positionXYZ);
	SetShaderTexture("ComputerCase");
	SetShaderMaterial("plastic");
	SetTextureUVScale(2.0f, 2.0f);
	m_basicMeshes->DrawBoxMesh();

	// ---- Upper Floppy Slot ---- //
	scaleXYZ = glm::vec3(1.5f, 0.1f, 0.05f);
	positionXYZ = glm::vec3(2.0f, 4.4f, 2.7f);
	SetTransformations(scaleXYZ, 0, 0, 0, positionXYZ);
	SetShaderColor(0.1f, 0.1f, 0.1f, 1);  // dark slot
	m_basicMeshes->DrawBoxMesh();

	// ---- Lower Floppy Slot ---- //
	scaleXYZ = glm::vec3(1.5f, 0.1f, 0.05f);
	positionXYZ = glm::vec3(2.0f, 2.8f, 2.7f);
	SetTransformations(scaleXYZ, 0, 0, 0, positionXYZ);
	SetShaderColor(0.1f, 0.1f, 0.1f, 1);
	m_basicMeshes->DrawBoxMesh();
}

void SceneManager::DrawCRTPanel()
{
	glm::vec3 scaleXYZ = glm::vec3(3.5f, 2.8f, 0.2f);
	glm::vec3 positionXYZ = glm::vec3(-1.25f, 3.5f, 2.6f);
	SetTransformations(scaleXYZ, 0, 0, 0, positionXYZ);
	
	// Enable the texture
	SetShaderTexture("CRTScreen");
	SetTextureUVScale(1.0f, 1.0f); // Default scaling to fit the screen
	m_basicMeshes->DrawBoxMesh();
}

void SceneManager::DrawProFile()
{
	// ---- ProFile Main Body ---- //
	glm::vec3 scaleXYZ = glm::vec3(6.5f, 1.0f, 4.0f);
	glm::vec3 positionXYZ = glm::vec3(0.0f, 6.7f, 0.0f);
	SetTransformations(scaleXYZ, 0, 0, 0, positionXYZ);

	SetShaderTexture("ComputerCase");
	SetShaderMaterial("plastic");
	SetTextureUVScale(2.0f, 2.0f);
	m_basicMeshes->DrawBoxMesh();

	// ---- Rubber Feet ---- //
	glm::vec3 footScale = glm::vec3(0.3f, 0.2f, 0.3f);  // Small black rubber feet
	
	float baseY = 6.10f;  // Slightly above computer body
	float xInset = 2.75f;
	float zInset = 1.5f;

	// Top-Left Foot
	SetTransformations(footScale, 0, 0, 0, glm::vec3(-xInset, baseY, -zInset));
	SetShaderColor(0.05f, 0.05f, 0.05f, 1);
	m_basicMeshes->DrawBoxMesh();

	// Top-Right Foot
	SetTransformations(footScale, 0, 0, 0, glm::vec3(xInset, baseY, -zInset));
	SetShaderColor(0.05f, 0.05f, 0.05f, 1);
	m_basicMeshes->DrawBoxMesh();

	// Bottom-Left Foot
	SetTransformations(footScale, 0, 0, 0, glm::vec3(-xInset, baseY, zInset));
	SetShaderColor(0.05f, 0.05f, 0.05f, 1);
	m_basicMeshes->DrawBoxMesh();

	// Bottom-Right Foot
	SetTransformations(footScale, 0, 0, 0, glm::vec3(xInset, baseY, zInset));
	SetShaderColor(0.05f, 0.05f, 0.05f, 1);
	m_basicMeshes->DrawBoxMesh();
}

void SceneManager::DrawMouse()
{
	// ---- Mouse Base ---- //
	glm::vec3 baseScale = glm::vec3(1.5f, 0.2f, 2.5f);
	glm::vec3 basePosition = glm::vec3(6.0f, 0.1f, 4.0f);

	SetTransformations(baseScale, 0.0f, 0.0f, 0.0f, basePosition);
	SetShaderColor(0.96f, 0.91f, 0.76f, 1.0f);  // Beige
	m_basicMeshes->DrawBoxMesh();

	// ---- Mouse Top ---- //
	glm::vec3 topScale = glm::vec3(1.5f, 0.5f, 2.5f);
	glm::vec3 topPosition = glm::vec3(6.0f, 0.1f, 4.0f);

	float slopeDegrees = 7.0f;  // Slopes upward from user to back
	SetTransformations(topScale, slopeDegrees, 0.0f, 0.0f, topPosition);
	SetShaderColor(0.96f, 0.91f, 0.76f, 1.0f);  // beige
	m_basicMeshes->DrawBoxMesh();

	// ---- Mouse Button ---- //
	glm::vec3 buttonScale = glm::vec3(1.2f, 0.05f, 0.5f);
	glm::vec3 buttonPosition = glm::vec3(6.0f, 0.45f, 3.3f);

	SetTransformations(buttonScale, slopeDegrees, 0.0f, 0.0f, buttonPosition);
	SetShaderColor(0.2f, 0.2f, 0.2f, 1.0f);  // Dark button
	m_basicMeshes->DrawBoxMesh();
}

void SceneManager::DrawMouseCable()
{
	glm::vec3 scaleCable = glm::vec3(0.05f, 0.5f, 0.05f);
	SetShaderColor(0.1f, 0.1f, 0.1f, 1.0f);  // black

	// Segment 1
	scaleCable.y = 7.5f;
	SetTransformations(scaleCable, 90.0f, 0.0f, 0.25f, glm::vec3(6.0f, 0.0f, -3.0f)); 
	m_basicMeshes->DrawCylinderMesh();

	// Segment 2
	scaleCable.y = 1.0f;
	SetTransformations(scaleCable, 90.0f, 0.0f, 0.25f, glm::vec3(1.25f, 0.0f, -3.0f));
	m_basicMeshes->DrawCylinderMesh();

	// Segment 3
	scaleCable.y = 4.85f; 
	SetTransformations(scaleCable, 0.0f, 0.0f, 90.0f, glm::vec3(6.05f, 0.0f, -3.0f));
	m_basicMeshes->DrawCylinderMesh();
}

void SceneManager::DrawKeyboard()
{
	// --- BASE --- //
	glm::vec3 baseScale = glm::vec3(5.0f, 0.5f, 2.0f);
	glm::vec3 basePosition = glm::vec3(0.0f, 0.1f, 5.0f);
	SetTransformations(baseScale, 0.0f, 0.0f, 0.0f, basePosition);
	SetShaderMaterial("plastic");
	SetShaderColor(0.96f, 0.91f, 0.76f, 1.0f);  // beige
	m_basicMeshes->DrawBoxMesh();

	// --- SLOPED UPPER HALF --- //
	glm::vec3 topScale = glm::vec3(5.0f, 0.4f, 2.0f);
	glm::vec3 topPosition = glm::vec3(0.0f, 0.35f, 5.0f);
	float slopeDegrees = 7.0f;  // Slopes upward from user to back
	SetTransformations(topScale, slopeDegrees, 0.0f, 0.0f, topPosition);
	SetShaderMaterial("plastic");
	SetShaderColor(0.96f, 0.91f, 0.76f, 1.0f);
	m_basicMeshes->DrawBoxMesh();

	// Letter section keys
	int rows = 5;
	int cols = 14;
	float keySpacingX = 0.35f;
	float keySpacingZ = 0.35f;

	float gridStartX = -((cols - 1) * keySpacingX) / 2.0f;  // Center X
	float gridStartZ = 5.3f;  // Forward-most row (closest to user)
	float y = 0.55f;

	// Dynamically draw the rows of regular sized keys
	for (int row = 0; row < rows; ++row)
	{
		// Regular keys section
		if (row < 4)
		{
			for (int col = 0; col < cols; ++col)
			{
				float x = gridStartX + col * keySpacingX;
				float z = gridStartZ - row * keySpacingZ;

				// Adjust vertical position for each key
				float adjustedY = y;

				if (row == 3)
				{
					adjustedY += 0.1f;
				}
				if (row == 2)
				{
					adjustedY += 0.06f;
				}
				if (row == 1)
				{
					adjustedY += 0.02f;
				}
				if (row == 0)
				{
					adjustedY -= 0.02f;
				}

				DrawKey(x, adjustedY, z, slopeDegrees);
			}
		}
		// Special bottom row section
		else
		{
			float customRowY = y - 0.05f;
			float customZ = gridStartZ + keySpacingZ * 1.2f;
			float slopeDegrees = 7.0f;

			float spacing = 0.05f;  // Space between keys

			// Define key widths in the special row
			float widths[5] = { 0.3f, 0.6f, 1.8f, 0.6f, 0.3f };

			// Calculate total width
			float totalKeyWidth = widths[0] + widths[1] + widths[2] + widths[3] + widths[4];
			float totalSpacing = spacing * 4;
			float totalRowWidth = totalKeyWidth + totalSpacing;

			// Starting x position (centered)
			float xCursor = -totalRowWidth / 2.0f;

			// Draw each key
			for (int i = 0; i < 5; ++i)
			{
				float keyWidth = widths[i];
				glm::vec3 keyScale = glm::vec3(keyWidth, 0.1f, 0.3f);
				DrawKey(xCursor + keyWidth / 2.0f, customRowY, customZ, slopeDegrees, keyScale);
				xCursor += keyWidth + spacing;  // Move to next key
			}
		}
		
	}
}

void SceneManager::DrawKey(float x, float y, float z, float slopeDegrees, glm::vec3 keyScale)
{
	SetTransformations(keyScale, slopeDegrees, 0.0f, 0.0f, glm::vec3(x, y, z));
	SetShaderMaterial("plastic");
	SetShaderColor(0.2f, 0.2f, 0.2f, 1.0f);  // Dark gray
	m_basicMeshes->DrawBoxMesh();
}

void SceneManager::DrawKeyboardCable()
{
	glm::vec3 cableThickness = glm::vec3(0.05f, 0.5f, 0.05f);
	SetShaderColor(0.1f, 0.1f, 0.1f, 1.0f);  // black

	// Segment 1
	glm::vec3 backCenter = glm::vec3(0.0f, 0.1f, 3.5f);
	SetTransformations(cableThickness, 90.0f, 0.0f, 0.0f, backCenter);
	m_basicMeshes->DrawCylinderMesh();

	// Segment 2
	glm::vec3 nextCableScale = glm::vec3(0.05f, 2.02f, 0.05f);

	glm::vec3 segment2Center = glm::vec3(2.0f, 0.1f, 3.5f);
	SetTransformations(nextCableScale, 0.0f, 0.0f, 90.0f, segment2Center);
	m_basicMeshes->DrawCylinderMesh();

	// Segment 3
	glm::vec3 segment3Scale = glm::vec3(0.05f, 3.0f, 0.05f);
	glm::vec3 segment3Center = glm::vec3(2.0f, 0.1f, 0.5f);
	SetTransformations(segment3Scale, 90.0f, 0.0f, 0.0f, segment3Center);
	m_basicMeshes->DrawCylinderMesh();
}

/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by 
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(20.0f, 1.0f, 10.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 0.0f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("TableTexture");
	SetShaderMaterial("cement");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();

	// ---- Main Computer Body ---- //
	DrawMainBody();

	// ---- Back Support Base of Computer ---- //
	DrawBackBase();

	// ---- Supporting Feet ---- //
	DrawFeet();

	// ---- Floppy Drive Section of Computer Body ---- //
	DrawFloppyDrive();

	// ---- CRT Screen Panel Backing ---- //
	DrawCRTPanel();

	// ---- ProFile Hard Drive ---- //
	DrawProFile();

	// ---- Mouse ---- //
	DrawMouse();
	DrawMouseCable();

	// ---- Keyboard ---- //
	DrawKeyboard();
	DrawKeyboardCable();
}
