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
SceneManager::SceneManager(ShaderManager* pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();

	// initialize the texture collection
	for (int i = 0; i < 16; i++)
	{
		m_textureIDs[i].tag = "/0";
		m_textureIDs[i].ID = -1;
	}
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
 *  LoadSceneTextures()
 *
 *  This method is used for loading the textures from image
 *  files into memory and binding them to texture slots.
 ***********************************************************/
void SceneManager::LoadSceneTextures()
{
	bool bReturn = false;

	// paving stone image for the walkway, columns and arches
	bReturn = CreateGLTexture(
		"../../Utilities/textures/pavers.jpg",
		"pavers");

	// weathered stone image for the walls and tower tiers
	bReturn = CreateGLTexture(
		"../../Utilities/textures/cheese_wheel.jpg",
		"stone");

	// mossy green image for the dome
	bReturn = CreateGLTexture(
		"../../Utilities/textures/backdrop.jpg",
		"moss");

	// gold image for the finial and spire
	bReturn = CreateGLTexture(
		"../../Utilities/textures/gold-seamless-texture.jpg",
		"gold");

	// wood image for the railing top rails
	bReturn = CreateGLTexture(
		"../../Utilities/textures/rusticwood.jpg",
		"wood");

	// brushed metal image for the railing posts
	bReturn = CreateGLTexture(
		"../../Utilities/textures/stainless.jpg",
		"metal");

	// grass image for the ground plane
	bReturn = CreateGLTexture(
		"../../Utilities/textures/grass.jpg",
		"grass");

	// leafy green image for the treetops
	bReturn = CreateGLTexture(
		"../../Utilities/textures/leaves.jpg",
		"leaves");

	// after the texture image data is loaded into memory, the
	// loaded textures need to be bound to texture slots
	BindGLTextures();
}

/***********************************************************
 *  DefineObjectMaterials()
 *
 *  This method is used for configuring the material
 *  settings for all of the objects within the 3D scene.
 ***********************************************************/
void SceneManager::DefineObjectMaterials()
{
	// matte material for stone surfaces
	OBJECT_MATERIAL stoneMaterial;
	stoneMaterial.ambientColor = glm::vec3(0.2f, 0.2f, 0.2f);
	stoneMaterial.ambientStrength = 0.15f;
	stoneMaterial.diffuseColor = glm::vec3(0.3f, 0.3f, 0.3f);
	stoneMaterial.specularColor = glm::vec3(0.1f, 0.1f, 0.1f);
	stoneMaterial.shininess = 0.5f;
	stoneMaterial.tag = "stone";

	m_objectMaterials.push_back(stoneMaterial);

	// ground material that reflects a soft warm shine from the sunset light
	OBJECT_MATERIAL groundMaterial;
	groundMaterial.ambientColor = glm::vec3(0.2f, 0.2f, 0.2f);
	groundMaterial.ambientStrength = 0.15f;
	groundMaterial.diffuseColor = glm::vec3(0.3f, 0.3f, 0.3f);
	groundMaterial.specularColor = glm::vec3(0.35f, 0.3f, 0.25f);
	groundMaterial.shininess = 8.0f;
	groundMaterial.tag = "ground";

	m_objectMaterials.push_back(groundMaterial);

	// matte material for tree leaves
	OBJECT_MATERIAL foliageMaterial;
	foliageMaterial.ambientColor = glm::vec3(0.2f, 0.2f, 0.2f);
	foliageMaterial.ambientStrength = 0.15f;
	foliageMaterial.diffuseColor = glm::vec3(0.32f, 0.32f, 0.32f);
	foliageMaterial.specularColor = glm::vec3(0.05f, 0.05f, 0.05f);
	foliageMaterial.shininess = 0.2f;
	foliageMaterial.tag = "foliage";

	m_objectMaterials.push_back(foliageMaterial);

	// slightly shiny material for wood
	OBJECT_MATERIAL woodMaterial;
	woodMaterial.ambientColor = glm::vec3(0.2f, 0.2f, 0.2f);
	woodMaterial.ambientStrength = 0.15f;
	woodMaterial.diffuseColor = glm::vec3(0.3f, 0.3f, 0.3f);
	woodMaterial.specularColor = glm::vec3(0.1f, 0.1f, 0.1f);
	woodMaterial.shininess = 1.0f;
	woodMaterial.tag = "wood";

	m_objectMaterials.push_back(woodMaterial);

	// shiny material for gold and metal
	OBJECT_MATERIAL shinyMaterial;
	shinyMaterial.ambientColor = glm::vec3(0.2f, 0.2f, 0.2f);
	shinyMaterial.ambientStrength = 0.2f;
	shinyMaterial.diffuseColor = glm::vec3(0.35f, 0.35f, 0.35f);
	shinyMaterial.specularColor = glm::vec3(0.7f, 0.6f, 0.45f);
	shinyMaterial.shininess = 25.0f;
	shinyMaterial.tag = "shiny";

	m_objectMaterials.push_back(shinyMaterial);
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
	// load the textures for the 3D scene
	LoadSceneTextures();
	// define the materials for objects in the scene
	DefineObjectMaterials();

	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene

	// set the lighting values for the 3D scene
	m_pShaderManager->setBoolValue(g_UseLightingName, true);

	// light 0: dim warm glow low and far to the left, like the last of the sun
	m_pShaderManager->setVec3Value("lightSources[0].position", -40.0f, 7.0f, 12.0f);
	m_pShaderManager->setVec3Value("lightSources[0].ambientColor", 0.22f, 0.12f, 0.06f);
	m_pShaderManager->setVec3Value("lightSources[0].diffuseColor", 1.0f, 0.55f, 0.25f);
	m_pShaderManager->setVec3Value("lightSources[0].specularColor", 0.4f, 0.3f, 0.2f);
	m_pShaderManager->setFloatValue("lightSources[0].focalStrength", 32.0f);
	m_pShaderManager->setFloatValue("lightSources[0].specularIntensity", 0.3f);

	// light 1: cool blue twilight sky light high on the right to fill the shaded side
	m_pShaderManager->setVec3Value("lightSources[1].position", 30.0f, 25.0f, 20.0f);
	m_pShaderManager->setVec3Value("lightSources[1].ambientColor", 0.06f, 0.10f, 0.26f);
	m_pShaderManager->setVec3Value("lightSources[1].diffuseColor", 0.40f, 0.45f, 0.70f);
	m_pShaderManager->setVec3Value("lightSources[1].specularColor", 0.2f, 0.2f, 0.35f);
	m_pShaderManager->setFloatValue("lightSources[1].focalStrength", 12.0f);
	m_pShaderManager->setFloatValue("lightSources[1].specularIntensity", 0.15f);

	// light 2: soft purple light behind the tower to light its back side
	m_pShaderManager->setVec3Value("lightSources[2].position", 0.0f, 6.0f, -40.0f);
	m_pShaderManager->setVec3Value("lightSources[2].ambientColor", 0.04f, 0.06f, 0.16f);
	m_pShaderManager->setVec3Value("lightSources[2].diffuseColor", 0.40f, 0.45f, 0.80f);
	m_pShaderManager->setVec3Value("lightSources[2].specularColor", 0.2f, 0.2f, 0.3f);
	m_pShaderManager->setFloatValue("lightSources[2].focalStrength", 16.0f);
	m_pShaderManager->setFloatValue("lightSources[2].specularIntensity", 0.15f);

	// light 3: small warm lantern light in front of the tower entrance
	m_pShaderManager->setVec3Value("lightSources[3].position", 0.0f, 4.0f, -2.0f);
	m_pShaderManager->setVec3Value("lightSources[3].ambientColor", 0.10f, 0.06f, 0.02f);
	m_pShaderManager->setVec3Value("lightSources[3].diffuseColor", 1.0f, 0.7f, 0.4f);
	m_pShaderManager->setVec3Value("lightSources[3].specularColor", 0.3f, 0.25f, 0.2f);
	m_pShaderManager->setFloatValue("lightSources[3].focalStrength", 10.0f);
	m_pShaderManager->setFloatValue("lightSources[3].specularIntensity", 0.15f);

	m_basicMeshes->LoadPlaneMesh();
	// loaded in the shapes we need for the project
	m_basicMeshes->LoadBoxMesh();
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadSphereMesh();
	m_basicMeshes->LoadConeMesh();
	m_basicMeshes->LoadTorusMesh();
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
	scaleXYZ = glm::vec3(20.0f, 1.0f, 20.0f);

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

	// apply the grass texture to the ground plane
	SetShaderTexture("grass");
	SetTextureUVScale(10.0f, 10.0f);
	// apply the ground material so the plane reflects the sunset light
	SetShaderMaterial("ground");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();
	/****************************************************************/

	// walkway: flat stone path leading from the tower to the edge of the scene (box)
	scaleXYZ = glm::vec3(3.0f, 0.2f, 25.0f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(0.0f, 0.1f, 7.5f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// apply the paver texture tiled along the walkway
	SetShaderTexture("pavers");
	SetTextureUVScale(1.5, 12.5);
	// apply the stone material to the walkway
	SetShaderMaterial("stone");

	m_basicMeshes->DrawBoxMesh();

	// railings: one on each side of the walkway
	for (int side = -1; side <= 1; side += 2)
	{
		// left side is negative x and right side is positive x
		float railX = side * 1.35f;

		// railing: top rail running along the walkway (box)
		scaleXYZ = glm::vec3(0.1f, 0.1f, 24.0f);
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;
		positionXYZ = glm::vec3(railX, 0.9f, 7.5f);

		SetTransformations(
			scaleXYZ,
			XrotationDegrees,
			YrotationDegrees,
			ZrotationDegrees,
			positionXYZ);

		// apply the wood texture to the top rail
		SetShaderTexture("wood");
		SetTextureUVScale(1.0, 1.0);
		SetShaderMaterial("wood");

		m_basicMeshes->DrawBoxMesh();

		// railing: posts spaced evenly along the walkway (cylinders)
		for (int i = 0; i < 13; i++)
		{
			scaleXYZ = glm::vec3(0.06f, 0.65f, 0.06f);
			XrotationDegrees = 0.0f;
			YrotationDegrees = 0.0f;
			ZrotationDegrees = 0.0f;
			positionXYZ = glm::vec3(railX, 0.2f, -4.5f + i * 2.0f);

			SetTransformations(
				scaleXYZ,
				XrotationDegrees,
				YrotationDegrees,
				ZrotationDegrees,
				positionXYZ);

			// apply the metal texture to the railing posts
			SetShaderTexture("metal");
			SetTextureUVScale(1.0, 1.0);
			SetShaderMaterial("shiny");

			m_basicMeshes->DrawCylinderMesh();
		}
	}

	// arch: left pillar (cylinder)
	scaleXYZ = glm::vec3(0.24f, 2.0f, 0.45f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(-1.2f, 0.2f, -0.075f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// apply the paver texture to the arch frame
	SetShaderTexture("pavers");
	SetTextureUVScale(1.0, 1.0);
	// apply the stone material to the arch, walls and tower
	SetShaderMaterial("stone");

	m_basicMeshes->DrawCylinderMesh();

	// arch: right pillar (cylinder)
	scaleXYZ = glm::vec3(0.24f, 2.0f, 0.45f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(1.2f, 0.2f, -0.075f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// apply the paver texture to the arch frame
	SetShaderTexture("pavers");
	SetTextureUVScale(1.0, 1.0);

	m_basicMeshes->DrawCylinderMesh();

	// arch: curved top (half torus)
	scaleXYZ = glm::vec3(1.2f, 1.2f, 0.45f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(0.0f, 2.2f, -0.075f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// apply the paver texture to the arch frame
	SetShaderTexture("pavers");
	SetTextureUVScale(1.0, 1.0);

	m_basicMeshes->DrawHalfTorusMesh();

	// arch: top left corner fill (box)
	scaleXYZ = glm::vec3(0.425f, 1.45f, 0.45f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(-1.205f, 2.925f, -0.075f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// apply the paver texture sized to match the wall
	SetShaderTexture("pavers");
	SetTextureUVScale(0.21, 0.73);

	m_basicMeshes->DrawBoxMesh();

	// arch: top right corner fill (box)
	scaleXYZ = glm::vec3(0.35f, 1.45f, 0.45f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(1.205f, 2.925f, -0.075f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// apply the paver texture sized to match the wall
	SetShaderTexture("pavers");
	SetTextureUVScale(0.18, 0.73);

	m_basicMeshes->DrawBoxMesh();

	// arch: top center fill above the curve (box)
	scaleXYZ = glm::vec3(2.1f, 0.45f, 0.45f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(0.0f, 3.425f, -0.075f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// apply the paver texture sized to match the wall
	SetShaderTexture("pavers");
	SetTextureUVScale(1.05, 0.23);

	m_basicMeshes->DrawBoxMesh();

	// broken wall: left section next to the arch (box)
	scaleXYZ = glm::vec3(1.5f, 5.7f, 0.6f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(-2.1f, 0.8f, 0.0f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// apply the paver texture tiled across the wall
	SetShaderTexture("pavers");
	SetTextureUVScale(0.75, 2.85);

	m_basicMeshes->DrawBoxMesh();

	// broken wall: left section further out, shorter to look crumbled (box)
	scaleXYZ = glm::vec3(1.0f, 2.15f, 0.6f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(-3.3f, 0.425f, 0.0f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// apply the paver texture tiled across the wall
	SetShaderTexture("pavers");
	SetTextureUVScale(0.5, 1.08);

	m_basicMeshes->DrawBoxMesh();

	// broken wall: right section next to the arch, medium height (box)
	scaleXYZ = glm::vec3(1.6f, 5.7f, 0.6f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(2.1f, 0.8f, 0.0f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// apply the paver texture tiled across the wall
	SetShaderTexture("pavers");
	SetTextureUVScale(0.8, 2.85);

	m_basicMeshes->DrawBoxMesh();

	// broken wall: right section further out, low to look crumbled (box)
	scaleXYZ = glm::vec3(1.25f, 1.55f, 0.6f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(3.3f, 0.475f, 0.0f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// apply the paver texture tiled across the wall
	SetShaderTexture("pavers");
	SetTextureUVScale(0.63, 0.78);

	m_basicMeshes->DrawBoxMesh();

	// tower: foundation (cylinder)
	scaleXYZ = glm::vec3(5.2f, 0.8f, 5.2f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(0.0f, 0.0f, -10.0f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// apply the stone texture to the foundation
	SetShaderTexture("stone");
	SetTextureUVScale(1.0, 1.0);

	m_basicMeshes->DrawCylinderMesh();

	// tower: lower tier (cylinder)
	scaleXYZ = glm::vec3(4.0f, 6.0f, 4.0f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(0.0f, 0.8f, -10.0f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// apply the stone texture to the lower tier
	SetShaderTexture("stone");
	SetTextureUVScale(1.0, 1.0);

	m_basicMeshes->DrawCylinderMesh();

	// tower entrance: left pillar (cylinder)
	scaleXYZ = glm::vec3(0.28f, 3.4f, 0.48f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(-1.4f, 0.8f, -6.0f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// apply the stone texture to the right pillar
	SetShaderTexture("stone");
	SetTextureUVScale(1.0, 1.0);

	m_basicMeshes->DrawCylinderMesh();

	// tower entrance: right pillar (cylinder)
	scaleXYZ = glm::vec3(0.28f, 3.4f, 0.48f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(1.4f, 0.8f, -6.0f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// apply the stone texture to the right pillar
	SetShaderTexture("stone");
	SetTextureUVScale(1.0, 1.0);

	m_basicMeshes->DrawCylinderMesh();

	// tower entrance: curved top (half torus)
	scaleXYZ = glm::vec3(1.4f, 1.4f, 2.4f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(0.0f, 4.2f, -6.0f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// apply the stone texture to the cruved top
	SetShaderTexture("stone");
	SetTextureUVScale(1.0, 1.0);

	m_basicMeshes->DrawHalfTorusMesh();

	// tower entrance: doorway fill, lower rectangle (box)
	scaleXYZ = glm::vec3(2.4f, 3.4f, 0.2f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(0.0f, 2.5f, -6.0f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.05f, 0.05f, 0.07f, 1.0f);

	m_basicMeshes->DrawBoxMesh();

	// tower entrance: doorway fill, rounded top (cylinder turned to face forward)
	scaleXYZ = glm::vec3(1.3f, 0.2f, 1.3f);
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(0.0f, 4.2f, -6.1f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.05f, 0.05f, 0.07f, 1.0f);

	m_basicMeshes->DrawCylinderMesh();

	// tower: ledge ring (torus lying flat)
	scaleXYZ = glm::vec3(4.2f, 4.2f, 1.6f);
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(0.0f, 6.8f, -10.0f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// apply the stone texture to the ledge ring
	SetShaderTexture("stone");
	SetTextureUVScale(1.0, 1.0);

	m_basicMeshes->DrawTorusMesh();

	// tower: two identical open levels, each with six columns, six arches and a ledge ring
	for (int level = 0; level < 2; level++)
	{
		// height where this level starts: the first level sits on the lower tier
		// and the second level sits on top of the first
		float levelBaseY = 6.8f + level * 4.8f;

		for (int i = 0; i < 6; i++)
		{
			// angle of this column around the tower center
			float angle = glm::radians(i * 60.0f);

			// tower: column (cylinder)
			scaleXYZ = glm::vec3(0.3f, 4.8f, 0.3f);
			XrotationDegrees = 0.0f;
			YrotationDegrees = 0.0f;
			ZrotationDegrees = 0.0f;
			positionXYZ = glm::vec3(3.0f * cos(angle), levelBaseY, -10.0f + 3.0f * sin(angle));

			SetTransformations(
				scaleXYZ,
				XrotationDegrees,
				YrotationDegrees,
				ZrotationDegrees,
				positionXYZ);

			// apply the stone texture to the columns
			SetShaderTexture("stone");
			SetTextureUVScale(1.0, 1.0);

			m_basicMeshes->DrawCylinderMesh();

			// angle halfway between this column and the next one
			float midAngleDegrees = i * 60.0f + 30.0f;
			float midAngle = glm::radians(midAngleDegrees);
			// the arch sits on the straight line between two columns,
			// so it is a little closer to the tower center than the columns
			float archRadius = 3.0f * cos(glm::radians(30.0f));

			// tower: arch between two columns (half torus)
			scaleXYZ = glm::vec3(1.5f, 1.5f, 1.5f);
			XrotationDegrees = 0.0f;
			// turn the arch so it lines up with the two columns beside it
			YrotationDegrees = 90.0f - midAngleDegrees;
			ZrotationDegrees = 0.0f;
			positionXYZ = glm::vec3(archRadius * cos(midAngle), levelBaseY + 3.0f, -10.0f + archRadius * sin(midAngle));

			SetTransformations(
				scaleXYZ,
				XrotationDegrees,
				YrotationDegrees,
				ZrotationDegrees,
				positionXYZ);

			// apply the stone texture to the arches
			SetShaderTexture("stone");
			SetTextureUVScale(1.0, 1.0);

			m_basicMeshes->DrawHalfTorusMesh();
		}

		// tower: ledge ring on top of this level (torus lying flat)
		scaleXYZ = glm::vec3(3.4f, 3.4f, 1.6f);
		XrotationDegrees = 90.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;
		positionXYZ = glm::vec3(0.0f, levelBaseY + 4.8f, -10.0f);

		SetTransformations(
			scaleXYZ,
			XrotationDegrees,
			YrotationDegrees,
			ZrotationDegrees,
			positionXYZ);

		// apply the stone texture to the ledge ring
		SetShaderTexture("stone");
		SetTextureUVScale(1.0, 1.0);

		m_basicMeshes->DrawTorusMesh();
	}

	// tower: dome (half sphere)
	scaleXYZ = glm::vec3(3.8f, 3.4f, 3.8f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(0.0f, 16.4f, -10.0f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// apply the moss texture to the dome
	SetShaderTexture("moss");
	SetTextureUVScale(1.0, 1.0);

	m_basicMeshes->DrawHalfSphereMesh();

	// tower: small dome on top of the dome (sphere)
	scaleXYZ = glm::vec3(0.5f, 0.5f, 0.5f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(0.0f, 20.0f, -10.0f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// apply the gold texture to the small dome
	SetShaderTexture("gold");
	SetTextureUVScale(1.0, 1.0);
	// apply the shiny material to the small dome and spire
	SetShaderMaterial("shiny");

	m_basicMeshes->DrawSphereMesh();

	// tower: spire on top of the tower (cone)
	scaleXYZ = glm::vec3(0.2f, 1.4f, 0.2f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(0.0f, 20.4f, -10.0f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// apply the gold texture to the top of the tower
	SetShaderTexture("gold");
	SetTextureUVScale(1.0, 1.0);

	m_basicMeshes->DrawConeMesh();

	// forest: x position, z position and size of each pine tree
	float treeData[24][3] = {
		{ -8.0f, 16.0f, 1.2f },  { -13.0f, 12.0f, 1.6f }, { -9.0f, 7.0f, 1.4f },
		{ -15.0f, 3.0f, 1.9f },  { -9.5f, -2.0f, 1.3f },  { -14.0f, -6.0f, 2.0f },
		{ -10.5f, -13.0f, 1.5f }, { -17.0f, 9.0f, 1.9f }, { -18.0f, -3.0f, 1.7f },
		{ -16.0f, -14.0f, 1.8f }, { -12.0f, 17.0f, 1.4f }, { -18.0f, 16.0f, 1.8f },
		{ 9.0f, 17.0f, 1.4f },   { 13.0f, 13.0f, 1.8f },  { 8.5f, 8.0f, 1.2f },
		{ 15.0f, 5.0f, 2.0f },   { 9.5f, -1.0f, 1.6f },   { 12.0f, -7.0f, 1.7f },
		{ 10.5f, -14.0f, 1.3f }, { 17.5f, 10.0f, 1.9f },  { 18.0f, -4.0f, 1.5f },
		{ 16.0f, -15.0f, 2.1f }, { 12.5f, 18.0f, 1.5f },  { 18.0f, 15.0f, 2.0f }
	};

	for (int t = 0; t < 24; t++)
	{
		float treeX = treeData[t][0];
		float treeZ = treeData[t][1];
		float treeSize = treeData[t][2];

		// tree: trunk (cylinder)
		scaleXYZ = glm::vec3(0.35f * treeSize, 2.0f * treeSize, 0.35f * treeSize);
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;
		positionXYZ = glm::vec3(treeX, 0.0f, treeZ);

		SetTransformations(
			scaleXYZ,
			XrotationDegrees,
			YrotationDegrees,
			ZrotationDegrees,
			positionXYZ);

		// apply the wood texture and material to the trunk
		SetShaderTexture("wood");
		SetTextureUVScale(1.0, 1.0);
		SetShaderMaterial("wood");

		m_basicMeshes->DrawCylinderMesh();

		// tree: bottom layer of leaves (cone)
		scaleXYZ = glm::vec3(1.8f * treeSize, 2.4f * treeSize, 1.8f * treeSize);
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;
		positionXYZ = glm::vec3(treeX, 1.6f * treeSize, treeZ);

		SetTransformations(
			scaleXYZ,
			XrotationDegrees,
			YrotationDegrees,
			ZrotationDegrees,
			positionXYZ);

		// apply the leaves texture and foliage material
		SetShaderTexture("leaves");
		SetTextureUVScale(2.0, 2.0);
		SetShaderMaterial("foliage");

		m_basicMeshes->DrawConeMesh();

		// tree: middle layer of leaves (cone)
		scaleXYZ = glm::vec3(1.4f * treeSize, 2.2f * treeSize, 1.4f * treeSize);
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;
		positionXYZ = glm::vec3(treeX, 3.2f * treeSize, treeZ);

		SetTransformations(
			scaleXYZ,
			XrotationDegrees,
			YrotationDegrees,
			ZrotationDegrees,
			positionXYZ);

		SetShaderTexture("leaves");
		SetTextureUVScale(2.0, 2.0);

		m_basicMeshes->DrawConeMesh();

		// tree: top layer of leaves (cone)
		scaleXYZ = glm::vec3(1.0f * treeSize, 2.0f * treeSize, 1.0f * treeSize);
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;
		positionXYZ = glm::vec3(treeX, 4.6f * treeSize, treeZ);

		SetTransformations(
			scaleXYZ,
			XrotationDegrees,
			YrotationDegrees,
			ZrotationDegrees,
			positionXYZ);

		SetShaderTexture("leaves");
		SetTextureUVScale(2.0, 2.0);

		m_basicMeshes->DrawConeMesh();
	}
}