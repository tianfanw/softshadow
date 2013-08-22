/*
 * Quad.h
 *
 *  Created on: 2013-06-17
 *      Author: tiawang
 */

#ifndef QUAD_H_
#define QUAD_H_

#include "mat.h"
#include <string.h>

struct TexInfo {
	GLuint _alphaTex, _shadowTex;
	GLint _imgWidth, _imgHeight;
	vec2 _hardShadowCoord[4];
	vec2 _alphaCoord[4];
	GLuint _widhght;
	vec2 _texQuadLength;
};

class Quad {
	friend class ShadowRenderer;
public:
	Quad(vec3 center = vec3(0.0), vec2 size = vec2(1.0), vec3 color = vec3(1.0));
	virtual ~Quad();

	void rotateX(GLfloat angle);
	void rotateY(GLfloat angle);
	void rotateZ(GLfloat angle);
	void translate(vec3 offset);
	void reset() { m_transMat = mat4(1.0); }
	void transform(mat4 transMat);

	vec3 center() const { return m_center; }
	vec3 origCenter() const { return m_origCenter; }
	vec2 size() const { return m_size; }
	vec2 origSize() const { return m_origSize; }

protected:
	void setLight(vec3 lightPos, GLfloat lightRadius);
	void setLightRadius(GLfloat lightRadius);
	void setGroundZ(GLfloat groundZ);
	vec3 getNormal() const { return m_normal; }

	void updateShadowBound();
	bool isPosUpdated() { return m_isPosUpdated; }
	bool isTransparent() { return m_transparent; }
	void drawQuad(GLuint posLoc, GLuint alphaLoc, GLuint colorLoc);
	void drawTopQuad(GLuint posLoc, GLuint colorLoc);
	void drawOuterQuad(GLuint posLoc, GLuint colorLoc);
	void drawHardQuad(GLuint posLoc, GLuint colorLoc);
	void drawShadow(GLuint shadowShader, GLuint positionLoc, GLuint alphaLoc, GLuint colorLoc);

	void setTexture(TexInfo frontTexInfo, TexInfo backTexInfo);
	TexInfo getTexture() const { return m_currentTexInfo; }

	void hide() { m_visible = false; }
	void show() { m_visible = true; }
	bool isVisible() { return m_visible; }

private:
	vec3 m_vertices[4];
	vec3 m_quad[4];
	vec3 m_innerQuad[4], m_outerQuad[4], m_hardQuad[4];
	vec2 m_alphaCoord[4];

	GLuint m_quadID, m_innerQuadID, m_outerQuadID, m_hardQuadID, m_topQuadID;
	vec4 m_color;
	vec3 m_origCenter, m_center;
	mat3 m_cMat;
	vec3 m_normal;
	vec3 m_lightPos;
	GLfloat m_lightRadius;
	GLfloat m_groundZ;
	vec2 m_origSize, m_size;

	GLfloat m_angle;
	mat4 m_transMat;

	bool m_isFlipped, m_frontFacing;
	bool m_transparent;
	bool m_visible;

	vec3 m_newLightPos;
	vec2 m_centerCa, m_centerCb;
	vec4 m_quadCa, m_quadCb;

	GLfloat m_blurCa, m_blurCb;
	vec3 m_newNormal;
	vec2 m_quadLengthCa, m_quadLengthCb;

	TexInfo m_currentTexInfo, m_frontTexInfo, m_backTexInfo;
	bool m_isPosUpdated, m_isShadowUpdated;
};

#endif /* QUAD_H_ */
