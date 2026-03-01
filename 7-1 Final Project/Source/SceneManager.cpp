///////////////////////////////////////////////////////////////////////////////
// shadermanager.cpp
// ============
// manage the loading and rendering of 3D scenes
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

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
	while ((index < m_objectMaterials.size()) && (bFound == false))
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
	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene

	m_basicMeshes->LoadPlaneMesh(); // Loads Floor Plane
	m_basicMeshes->LoadBoxMesh(); // Loads Box
	m_basicMeshes->LoadCylinderMesh();// Loads Cylinder
	m_basicMeshes->LoadConeMesh(); // Loads Cone
	m_basicMeshes->LoadSphereMesh(); // Loads Sphere

	// Load textures from the Textures/ folder and register them by tag.
	// Tags are used later to bind the correct texture when drawing each shape.
	CreateGLTexture("Textures/screen.jpg", "screen"); // Monitor display image
	CreateGLTexture("Textures/brushed_metal.jpg", "metal"); // Used as a shared material texture across monitor parts
	CreateGLTexture("Textures/wood_desk.jpg", "desk"); // Desk surface (tiled to avoid stretching on large plane)

	// Bind all loaded textures to texture units
	BindGLTextures();

	// ------------------------------------------------------------
	// Materials (Phong) - used by fragment shader
	// ------------------------------------------------------------
	OBJECT_MATERIAL deskMat;
	deskMat.tag = "deskMaterial";
	deskMat.ambientColor = glm::vec3(1.0f);
	deskMat.ambientStrength = 0.12f;
	deskMat.diffuseColor = glm::vec3(1.0f);
	deskMat.specularColor = glm::vec3(0.10f);
	deskMat.shininess = 12.0f;
	m_objectMaterials.push_back(deskMat);

	OBJECT_MATERIAL screenMat;
	screenMat.tag = "screenMaterial";
	screenMat.ambientColor = glm::vec3(1.0f);
	screenMat.ambientStrength = 0.18f;
	screenMat.diffuseColor = glm::vec3(1.0f);
	screenMat.specularColor = glm::vec3(0.0f);
	screenMat.shininess = 8.0f;
	m_objectMaterials.push_back(screenMat);

	OBJECT_MATERIAL metalMat;
	metalMat.tag = "metalMaterial";
	metalMat.ambientColor = glm::vec3(1.0f);
	metalMat.ambientStrength = 0.20f;
	metalMat.diffuseColor = glm::vec3(1.0f);
	metalMat.specularColor = glm::vec3(0.40f);
	metalMat.shininess = 32.0f;
	m_objectMaterials.push_back(metalMat);

	OBJECT_MATERIAL leafMat;
	leafMat.tag = "leafMaterial";
	leafMat.ambientColor = glm::vec3(1.0f);
	leafMat.ambientStrength = 0.25f;
	leafMat.diffuseColor = glm::vec3(1.0f);
	leafMat.specularColor = glm::vec3(0.02f);
	leafMat.shininess = 3.0f;
	m_objectMaterials.push_back(leafMat);
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

	// ------------------------------------------------------------
	// Lighting (once per frame) - matches fragmentShader.glsl
	// ------------------------------------------------------------
	m_pShaderManager->setIntValue(g_UseLightingName, true);

	// Light 0: warm key light above/forward (main light)
	m_pShaderManager->setVec3Value("lightSources[0].position", glm::vec3(3.0f, 6.0f, 5.0f));
	m_pShaderManager->setVec3Value("lightSources[0].ambientColor", glm::vec3(0.03f, 0.025f, 0.02f));
	m_pShaderManager->setVec3Value("lightSources[0].diffuseColor", glm::vec3(0.38f, 0.28f, 0.18f));
	m_pShaderManager->setVec3Value("lightSources[0].specularColor", glm::vec3(1.0f, 0.9f, 0.7f));
	m_pShaderManager->setFloatValue("lightSources[0].focalStrength", 16.0f);
	m_pShaderManager->setFloatValue("lightSources[0].specularIntensity", 0.06f);

	// Light 1: cool fill light (prevents harsh darkness)
	m_pShaderManager->setVec3Value("lightSources[1].position", glm::vec3(-5.0f, 4.0f, 2.0f));
	m_pShaderManager->setVec3Value("lightSources[1].ambientColor", glm::vec3(0.01f, 0.01f, 0.015f));
	m_pShaderManager->setVec3Value("lightSources[1].diffuseColor", glm::vec3(0.12f, 0.14f, 0.16f));
	m_pShaderManager->setVec3Value("lightSources[1].specularColor", glm::vec3(0.8f, 0.8f, 0.8f));
	m_pShaderManager->setFloatValue("lightSources[1].focalStrength", 10.0f);
	m_pShaderManager->setFloatValue("lightSources[1].specularIntensity", 0.05f);


	// Turn off remaining lights (because shader loops TOTAL_LIGHTS = 4)
	for (int i = 2; i < 4; i++)
	{
		m_pShaderManager->setVec3Value(("lightSources[" + std::to_string(i) + "].position").c_str(), glm::vec3(0.0f));
		m_pShaderManager->setVec3Value(("lightSources[" + std::to_string(i) + "].ambientColor").c_str(), glm::vec3(0.0f));
		m_pShaderManager->setVec3Value(("lightSources[" + std::to_string(i) + "].diffuseColor").c_str(), glm::vec3(0.0f));
		m_pShaderManager->setVec3Value(("lightSources[" + std::to_string(i) + "].specularColor").c_str(), glm::vec3(0.0f));
		m_pShaderManager->setFloatValue(("lightSources[" + std::to_string(i) + "].focalStrength").c_str(), 1.0f);
		m_pShaderManager->setFloatValue(("lightSources[" + std::to_string(i) + "].specularIntensity").c_str(), 0.0f);
	}


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

	SetShaderMaterial("deskMaterial");
	SetShaderTexture("desk");
	SetTextureUVScale(8.0f, 4.0f);

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();
	/****************************************************************/

	// ============================================================
	// Computer Monitor (replicated the monitor in the 2D reference image)
	// Built from multiple basic shapes: boxes and cylinder
	// ============================================================

	// ---- Screen (box) ----
	scaleXYZ = glm::vec3(7.3f, 3.2f, 0.2f);
	positionXYZ = glm::vec3(0.0f, 3.3f, 1.0f);
	
	SetTransformations(scaleXYZ, -5.0f, 0.0f, 0.0f, positionXYZ); // set x-rotation to -5.0f to add screen tilt to screen.

	SetShaderMaterial("screenMaterial");
	SetShaderTexture("screen");
	SetTextureUVScale(1.0f, 1.0f);

	m_basicMeshes->DrawBoxMesh();

	// ---- Bezel/frame (slightly larger box) ----
	scaleXYZ = glm::vec3(7.5f, 3.8f, 0.22f);
	positionXYZ = glm::vec3(0.0f, 3.15f, 0.97f);

	SetTransformations(scaleXYZ, -5.0f, 0.0f, 0.0f, positionXYZ); // set x-rotation to -5.0f to compliment the screen tilt of screen.

	SetShaderMaterial("metalMaterial");
	SetShaderTexture("metal");
	SetTextureUVScale(0.5f, 0.5f);

	m_basicMeshes->DrawBoxMesh();

	// ---- Connector arm (box) ----
	// Fills the gap between the monitor and the stand neck
	scaleXYZ = glm::vec3(1.2f, 0.6f, 0.6f);

	// Positioned it under the screen, slightly forward to meet the tilted monitor
	positionXYZ = glm::vec3(0.0f, 2.8f, 0.6f);

	// Match the screen tilt so it looks attached
	SetTransformations(scaleXYZ, -5.0f, 0.0f, 0.0f, positionXYZ);

	SetShaderMaterial("metalMaterial");
	SetShaderTexture("metal");
	SetTextureUVScale(0.5f, 0.5f);

	m_basicMeshes->DrawBoxMesh();

	// ---- Stand neck (cylinder) ----
	scaleXYZ = glm::vec3(1.2f, 3.2f, 0.35f);
	positionXYZ = glm::vec3(0.0f, 0.2f, 0.0f);

	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);

	SetShaderMaterial("metalMaterial");
	SetShaderTexture("metal");
	SetTextureUVScale(0.8f, 0.8f);
	m_basicMeshes->DrawCylinderMesh(true, true, true);

	// ---- Base (box) ----
	scaleXYZ = glm::vec3(2.5f, 0.25f, 2.0f);
	positionXYZ = glm::vec3(0.0f, 0.25f, 0.0f);

	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);

	SetShaderMaterial("metalMaterial");
	SetShaderTexture("metal");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawBoxMesh();

	// ----------------------------
	// Keyboard (base + top deck)
	// ----------------------------
	scaleXYZ = glm::vec3(5.0f, 0.20f, 1.6f);
	positionXYZ = glm::vec3(0.0f, 0.11f, 2.6f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderMaterial("metalMaterial");
	SetShaderTexture("metal");
	m_basicMeshes->DrawBoxMesh();

	// top inset (suggests keyboard keys)
	scaleXYZ = glm::vec3(4.7f, 0.08f, 1.3f);
	positionXYZ = glm::vec3(0.0f, 0.22f, 2.6f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderMaterial("metalMaterial");
	SetShaderTexture("metal");
	m_basicMeshes->DrawBoxMesh();

	// ----------------------------
	// Keyboard keys (simple grid of small boxes)
	// ----------------------------
	SetShaderMaterial("metalMaterial");
	SetShaderColor(0.55f, 0.55f, 0.55f, 1.0f);

	// Key sizing
	float keyW = 0.38f;
	float keyH = 0.05f;
	float keyD = 0.24f;

	// Spacing
	float gapX = 0.07f;
	float gapZ = 0.07f;

	// Grid layout
	int rows = 4;
	int cols = 10;

	// Starting position (bottom-left of keys area)
	// y is slightly above the inset top
	glm::vec3 startPos = glm::vec3(-1.995f, 0.29f, 2.165f);

	// Encapsulated key draw
	auto DrawKey = [&](const glm::vec3& pos)
		{
			SetTransformations(glm::vec3(keyW, keyH, keyD),
				0.0f, 0.0f, 0.0f,
				pos);

			m_basicMeshes->DrawBoxMesh();
		};

	for (int r = 0; r < rows; r++)
	{
		for (int c = 0; c < cols; c++)
		{
			glm::vec3 keyPos = startPos;
			keyPos.x += c * (keyW + gapX);
			keyPos.z += r * (keyD + gapZ);

			// Use helper instead of repeating the same 2 calls
			DrawKey(keyPos);
		}
	}

	// ----------------------------
	// Mouse (Magic Mouse style - smooth dome, no wheel)
	// ----------------------------
	scaleXYZ = glm::vec3(0.65f, 0.18f, 0.85f);
	positionXYZ = glm::vec3(3.8f, 0.19f, 2.6f);
	SetTransformations(scaleXYZ, -3.0f, 0.0f, 0.0f, positionXYZ); // slight forward tilt
	SetShaderColor(0.85f, 0.85f, 0.85f, 1.0f);
	SetShaderMaterial("metalMaterial");
	m_basicMeshes->DrawSphereMesh();

	// ----------------------------
	// Plant pot (cylinder)
	// ----------------------------
	scaleXYZ = glm::vec3(0.70f, 1.0f, 0.70f);
	positionXYZ = glm::vec3(-4.5f, 0.01f, 1.2f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderColor(0.95f, 0.95f, 0.95f, 1.0f);
	SetShaderMaterial("metalMaterial");
	m_basicMeshes->DrawCylinderMesh(true, true, true);

	// ----------------------------
	// Leaves (thinner, tilted, layered, tip highlights)
	// ----------------------------
	SetShaderMaterial("leafMaterial");

	// Anchor point near top of pot
	glm::vec3 potTop = glm::vec3(-4.5f, 0.95f, 1.2f);

	// ---------- Back layer (darker = depth) ----------
	SetShaderColor(0.07f, 0.40f, 0.16f, 1.0f);

	// back leaf A
	scaleXYZ = glm::vec3(0.09f, 0.78f, 0.09f);
	positionXYZ = potTop + glm::vec3(0.05f, -0.05f, -0.10f);
	SetTransformations(scaleXYZ, -22.0f, 0.0f, 6.0f, positionXYZ);
	m_basicMeshes->DrawConeMesh(true);

	// back leaf B
	positionXYZ = potTop + glm::vec3(-0.18f, -0.05f, -0.06f);
	SetTransformations(scaleXYZ, -18.0f, 0.0f, 22.0f, positionXYZ);
	m_basicMeshes->DrawConeMesh(true);

	// ---------- Front layer (brighter) ----------
	SetShaderColor(0.12f, 0.56f, 0.22f, 1.0f);

	// front leaf 1
	scaleXYZ = glm::vec3(0.10f, 0.95f, 0.10f);
	positionXYZ = potTop + glm::vec3(-0.05f, 0.00f, 0.00f);
	SetTransformations(scaleXYZ, -16.0f, 0.0f, 14.0f, positionXYZ);
	m_basicMeshes->DrawConeMesh(true);

	// front leaf 2
	scaleXYZ = glm::vec3(0.095f, 0.88f, 0.095f);
	positionXYZ = potTop + glm::vec3(0.12f, -0.02f, 0.10f);
	SetTransformations(scaleXYZ, -12.0f, 0.0f, -18.0f, positionXYZ);
	m_basicMeshes->DrawConeMesh(true);

	// front leaf 3
	scaleXYZ = glm::vec3(0.085f, 0.82f, 0.085f);
	positionXYZ = potTop + glm::vec3(-0.16f, -0.02f, 0.12f);
	SetTransformations(scaleXYZ, -10.0f, 0.0f, 8.0f, positionXYZ);
	m_basicMeshes->DrawConeMesh(true);
}
