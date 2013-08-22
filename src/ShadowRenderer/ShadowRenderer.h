/*
 * ShadowRenderer.h
 *
 *  Created on: 2013-08-01
 *      Author: tiawang
 */

#ifndef SHADOWRENDERER_H_
#define SHADOWRENDERER_H_

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <vector>
#include <map>
#include "Quad.h"

using namespace std;

//#ifndef MIPMAP_TEXTURE
//#define MIPMAP_TEXTURE
//#endif

#ifndef LERP_TEXTURE_NUM
#define LERP_TEXTURE_NUM 4
#endif

#define USE_TWO_FBO 0
#define USE_16BIT_DEPTH 1


struct RenderProperty {
	GLuint _objectId;
	GLfloat _depth;
	bool _transparent;
	bool _visible;

	RenderProperty(GLuint objectId, GLfloat depth, bool transparent, bool visible = true) :
		_objectId(objectId), _depth(depth), _transparent(transparent), _visible(visible) {}
};

class ShadowRenderer {
public:
	ShadowRenderer(int surfaceWidth = 720, int surfaceHeight = 720);
	virtual ~ShadowRenderer();

	// Scene configurations
	void setLight(vec3 lightPos, GLfloat lightRadius);
	void setGroundZ(GLfloat groundZ);

	// Textures
	GLuint addTexture(GLuint tex, GLfloat aspect = 1.0, GLfloat texWidth = 1.0, GLfloat texHeight = 1.0, bool shareShadow = false, GLuint shareWith = 0);

	// Objects
	GLuint addObject(vec3 center, vec2 size, vec3 color = vec3(1.0));
	Quad* getObject(GLuint objectId) { return m_objects[objectId];}
	int bindTexture(GLuint objectId, GLuint texture);
	int bindTexture(GLuint objectId, GLuint frontTexture, GLuint backTexture);
	void hide(GLuint objectId) { m_objects[objectId]->hide(); m_objectsUpdated = false; m_sceneUpdated = false; }
	void show(GLuint objectId) { m_objects[objectId]->show(); m_objectsUpdated = false; m_sceneUpdated = false; }

	void render();

private:
	static int genShaders();
	int genShadowTexture(TexInfo& texInfo);

	// Render functions
	void drawGroundPlane();
	void renderDepth();
	void renderShadow();
	void renderScene();

	int m_surfaceWidth, m_surfaceHeight;
	GLuint m_blurFbo, m_blurTex;
	GLuint m_depthFbo, m_depthTex, m_depthRender;
	GLuint m_shadowFbo, m_shadowTex, m_shadowRender;
	GLuint m_sceneFbo, m_sceneTex, m_sceneRender;
	vector<Quad*> m_objects;
	vector<RenderProperty> m_renderProperty;
	map<GLuint, TexInfo> m_textures; // 256 * 256 sample shadow textures

	vec3 m_lightPos;
	GLfloat m_lightRadius;
	GLfloat m_groundZ;
	vec3 m_groundPlane[4];

	bool m_sceneUpdated;
	bool m_objectsUpdated;

	GLuint m_depthBufferTex;
};



#endif /* SHADOWRENDERER_H_ */
