/*
 * Quad.cpp
 *
 *  Created on: 2013-06-17
 *      Author: tiawang
 */

#include "Quad.h"
#include <stdlib.h>

static GLfloat sampleHardShadowLength = 4.0 / 3.0;

Quad::Quad(vec3 center, vec2 size, vec3 color) {
	m_quadID = 0;
	m_innerQuadID = 0;
	m_outerQuadID = 0;
	m_topQuadID = 0;
	m_transMat = mat4(1.0);
	m_isFlipped = false;
	m_frontFacing = true;
	m_visible = true;

	m_lightPos = vec3(0.0, 0.0, 0.5);
	m_lightRadius = 0.1;
	m_groundZ = -0.5;
	m_isPosUpdated = false;
	m_isShadowUpdated = false;
	m_transparent = false;
	m_visible = true;

	m_alphaCoord[0] = vec2(0.0, 0.0);
	m_alphaCoord[1] = vec2(1.0, 0.0);
	m_alphaCoord[2] = vec2(0.0, 1.0);
	m_alphaCoord[3] = vec2(1.0, 1.0);

	m_origCenter = center;
	m_center = m_origCenter;
	m_origSize = size;
	m_size = m_origSize;
	vec2 halfSize = size / 2.0;
	m_vertices[0] = vec3(center.x - halfSize.x, center.y - halfSize.y, center.z);
	m_vertices[1] = vec3(center.x + halfSize.x, center.y - halfSize.y, center.z);
	m_vertices[2] = vec3(center.x - halfSize.x, center.y + halfSize.y, center.z);
	m_vertices[3] = vec3(center.x + halfSize.x, center.y + halfSize.y, center.z);

	m_color = vec4(color, 0.0);
}

Quad::~Quad() {
	// TODO Auto-generated destructor stub
}

void Quad::setLight(vec3 lightPos, GLfloat lightRadius) {
	m_lightPos = lightPos;
	m_lightRadius = lightRadius;
	m_isShadowUpdated = false;
}

void Quad::setLightRadius(GLfloat lightRadius) {
	m_lightRadius = lightRadius;
	m_isShadowUpdated = false;
}

void Quad::setGroundZ(GLfloat groundZ) {
	m_groundZ = groundZ;
	m_isShadowUpdated = false;
}

void Quad::rotateX(GLfloat angle) {
	m_transMat = Translate(m_center) * RotateX(angle) * Translate(-m_center) * m_transMat;
	m_isPosUpdated = false;
	m_isShadowUpdated = false;
}

void Quad::rotateY(GLfloat angle) {
	m_transMat = Translate(m_center) * RotateY(angle) * Translate(-m_center) * m_transMat;
	m_isPosUpdated = false;
	m_isShadowUpdated = false;
}

void Quad::rotateZ(GLfloat angle) {
	m_transMat = Translate(m_center) * RotateZ(angle) * Translate(-m_center) * m_transMat;
	m_isPosUpdated = false;
	m_isShadowUpdated = false;
}

void Quad::translate(vec3 offset) {
	m_transMat = Translate(offset) * m_transMat;
	m_isPosUpdated = false;
	m_isShadowUpdated = false;
}

void Quad::transform(mat4 transMat) {
	m_transMat = transMat;
	m_isPosUpdated = false;
	m_isShadowUpdated = false;
}

void Quad::setTexture(TexInfo frontTexInfo, TexInfo backTexInfo) {
	m_frontTexInfo = frontTexInfo;
	m_backTexInfo = backTexInfo;
	m_currentTexInfo = m_frontTexInfo;
	if(m_frontTexInfo._alphaTex == 0 || m_backTexInfo._alphaTex == 0)
		m_transparent = false;
	else m_transparent = true;
}

void Quad::drawQuad(GLuint posLoc, GLuint alphaLoc, GLuint colorLoc) {

	if(!m_isPosUpdated) {
		updateShadowBound();
	}

	glEnableVertexAttribArray(posLoc);
	glEnableVertexAttribArray(alphaLoc);
	glDisableVertexAttribArray(colorLoc);

	glBindBuffer(GL_ARRAY_BUFFER, m_quadID);
	glVertexAttribPointer(posLoc, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glVertexAttribPointer(alphaLoc, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), m_alphaCoord);
	glVertexAttrib4fv(colorLoc, m_color);
	glDrawArrays(GL_TRIANGLE_STRIP, 0 , 4);

	glDisableVertexAttribArray(posLoc);
	glDisableVertexAttribArray(alphaLoc);
}

void Quad::drawTopQuad(GLuint posLoc, GLuint colorLoc) {
	glEnableVertexAttribArray(posLoc);
	glDisableVertexAttribArray(colorLoc);

	glBindBuffer(GL_ARRAY_BUFFER, m_topQuadID);
	glVertexAttribPointer(posLoc, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), 0);
	glVertexAttrib4f(colorLoc, 0.0, 0.0, 1.0, 1.0);
	glDrawArrays(GL_TRIANGLE_STRIP, 0 , 4);

	glDisableVertexAttribArray(posLoc);
}

void Quad::drawHardQuad(GLuint posLoc, GLuint colorLoc) {
	glEnableVertexAttribArray(posLoc);
	glDisableVertexAttribArray(colorLoc);

	glBindBuffer(GL_ARRAY_BUFFER, m_hardQuadID);
	glVertexAttribPointer(posLoc, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), 0);
	glVertexAttrib4f(colorLoc, 1.0, 0.0, 0.0, 1.0);
	glDrawArrays(GL_TRIANGLE_STRIP, 0 , 10);

	glDisableVertexAttribArray(posLoc);
}

void Quad::drawOuterQuad(GLuint posLoc, GLuint colorLoc) {
	glEnableVertexAttribArray(posLoc);
	glDisableVertexAttribArray(colorLoc);

	glBindBuffer(GL_ARRAY_BUFFER, m_outerQuadID);
	glVertexAttribPointer(posLoc, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), 0);
	glVertexAttrib4f(colorLoc, 1.0, 0.0, 1.0, 1.0);
	glDrawArrays(GL_TRIANGLE_STRIP, 0 , 10);

	glDisableVertexAttribArray(posLoc);
}

void Quad::drawShadow(GLuint shadowShader, GLuint positionLoc, GLuint alphaLoc, GLuint colorLoc) {

	if(!m_isShadowUpdated)
		updateShadowBound();

	glDepthMask(false);

	glStencilMask(0xff);
	glClearStencil(0);
	glEnable(GL_STENCIL_TEST);
	glClear(GL_STENCIL_BUFFER_BIT);

	glColorMask(true, true, true, true);
	glUniformMatrix3fv(glGetUniformLocation(shadowShader, "cMat"), 1, false, transpose(m_cMat) );
	glUniform2fv(glGetUniformLocation(shadowShader, "centerCa"), 1, m_centerCa);
	glUniform2fv(glGetUniformLocation(shadowShader, "centerCb"), 1, m_centerCb);
	glUniform2fv(glGetUniformLocation(shadowShader, "texQuadLength"), 1, m_currentTexInfo._texQuadLength);
	glUniform3fv(glGetUniformLocation(shadowShader, "vl"), 1, m_newLightPos);
	glUniform3fv(glGetUniformLocation(shadowShader, "normal"), 1, m_normal);
	glUniform3fv(glGetUniformLocation(shadowShader, "newNormal"), 1, m_newNormal);
	glUniform1f(glGetUniformLocation(shadowShader, "blurCa"), m_blurCa);
	glUniform1f(glGetUniformLocation(shadowShader, "blurCb"), m_blurCb);
	glUniform2fv(glGetUniformLocation(shadowShader, "quadLengthCa"), 1, m_quadLengthCa);
	glUniform2fv(glGetUniformLocation(shadowShader, "quadLengthCb"), 1, m_quadLengthCb);

	glUniform3fv(glGetUniformLocation(shadowShader, "quadCenter"), 1, m_center);
	{
		GLfloat flipped = m_isFlipped ? -1.0 : 1.0;
		glUniform1f(glGetUniformLocation(shadowShader, "flipped"), flipped);
	}

	glStencilFunc(GL_EQUAL, 0, 0xff);
	glStencilOp(GL_KEEP, GL_KEEP, GL_INCR);

	glFrontFace(GL_CCW);
	glEnable(GL_CULL_FACE);

	glCullFace(GL_FRONT);
	glColorMask(false, false, false, false);
	drawOuterQuad(positionLoc, colorLoc);
	drawTopQuad(positionLoc, colorLoc);

	glCullFace(GL_BACK);
	glColorMask(true, true, true, true);
	drawOuterQuad(positionLoc, colorLoc);
	drawTopQuad(positionLoc, colorLoc);

	glDisable(GL_CULL_FACE);

	glStencilMask(0x00);
	glDisable(GL_STENCIL_TEST);
	glDepthMask(true);
	glBindTexture(GL_TEXTURE_2D, 0);
}

static GLfloat step(GLfloat edge, GLfloat x) {
	if(x < edge) return 0.0;
	else return 1.0;
}

template <class T>
static T mix(T x, T y, GLfloat a) {
	return x * (1.0 - a) + y * a;
}

template <class T>
static void swap(T& x, T& y) {
	T tmp;
	tmp = x;
	x = y;
	y = tmp;
}

void Quad::updateShadowBound() {

	if(m_isShadowUpdated || !m_visible) return;

//	std::cout << "---------- Quad Begin ------------" << std::endl;
	// 2   3  -->  3   2
	// 0   1  -->  0   1
	vec4 quadv4[4];
	quadv4[0] = m_transMat * vec4(m_vertices[0], 1.0);
	quadv4[1] = m_transMat * vec4(m_vertices[1], 1.0);
	quadv4[2] = m_transMat * vec4(m_vertices[3], 1.0);
	quadv4[3] = m_transMat * vec4(m_vertices[2], 1.0);
	for(int i = 0; i < 4; i++)
		m_quad[i] = quadv4[i].xyz() / quadv4[i].w;
//	m_quad[0] = (m_transMat * vec4(m_vertices[0], 1.0)).xyz();
//	m_quad[1] = (m_transMat * vec4(m_vertices[1], 1.0)).xyz();
//	m_quad[2] = (m_transMat * vec4(m_vertices[3], 1.0)).xyz();
//	m_quad[3] = (m_transMat * vec4(m_vertices[2], 1.0)).xyz();

//	for(int i = 0; i < 4; i++)
//		std::cout << "m_quad[" << i << "] = " << m_quad[i] << std::endl;
//	std::cout << "m_lightPos = " << m_lightPos << std::endl;
//	std::cout << "m_lightRadius = " << m_lightRadius << std::endl;

	m_center = (m_quad[0] + m_quad[1] + m_quad[2] + m_quad[3])/4;
	m_normal = normalize(cross(m_quad[1] - m_quad[0], m_quad[2] - m_quad[1]));

	m_frontFacing = (m_normal.z > 0.0);
//	std::cout << "m_normal = " << m_normal << std::endl;
	if (dot(m_lightPos - m_center, m_normal) < 0) {

		swap(m_quad[0], m_quad[1]);
		swap(m_quad[2], m_quad[3]);

		m_normal = -m_normal;
		m_isFlipped = true;
//		std::cout << "FLIPPED!" << std::endl;
//		for(int i = 0; i < 4; i++)
//			std::cout << "m_quad[" << i << "] = " << m_quad[i] << std::endl;
	} else {
		m_isFlipped = false;
	}

	m_size = vec2(length(m_quad[1] - m_quad[0]), length(m_quad[2] - m_quad[1]));

	// Calculate hard quad coordinates, basically extrude quad corners to the ground plane from light center
	for(int i = 0; i < 4; i++) {
		if(m_quad[i].z > m_groundZ)
			m_hardQuad[i] = (m_quad[i] - m_lightPos) / (m_quad[i].z - m_lightPos.z) * (m_groundZ - m_lightPos.z) + m_lightPos;
		else
			m_hardQuad[i] = (m_quad[i] - m_lightPos) * 1.01 + m_lightPos;
	}
//	for(int i = 0; i < 4; i++)
//		std::cout << "m_hardQuad[" << i << "] = " << m_hardQuad[i] << std::endl;

	// Calculte the projective mapping on samplebration shadow texture

	// Change of basis
	vec3 newX, newY, newZ;
	newX = normalize(m_quad[1] - m_quad[0]);
	newY = normalize(m_quad[2] - m_quad[1]);
	newZ = normalize(cross(newX, newY));
	m_cMat = mat3(newX, newY, newZ);
//	std::cout << "m_cMat = " << m_cMat << std::endl;

	vec3 lightPos = m_cMat * m_lightPos;
//	std::cout << "transformed lightPos = " << lightPos << std::endl;
	m_newLightPos = lightPos;


	vec3 center = m_cMat * m_center;
//	std::cout << "center = " << center << std::endl;
	m_centerCa = (center - lightPos).xy() / (center.z - lightPos.z);
	m_centerCb = lightPos.xy() - m_centerCa * lightPos.z;

	m_newNormal = m_cMat * m_normal;
//	std::cout << "m_newNormal = " << m_newNormal << std::endl;

	vec3 quad[4];
	for(int i = 0; i < 4; i++)
		quad[i] = m_cMat * m_quad[i];

//	for(int i = 0; i < 4; i++)
//		std::cout << "transformed quad[" << i << "] = " << quad[i] << std::endl;

	GLfloat quadz = quad[0].z;
	// minx miny maxx maxy
	m_quadCa = (vec4(quad[0].xy(), quad[2].xy()) - vec4(lightPos.xy(), lightPos.xy())) / (quadz - lightPos.z);
	m_quadCb = vec4(lightPos.xy(), lightPos.xy()) - m_quadCa * lightPos.z;

	m_quadLengthCa = vec2(m_quadCa[2] - m_quadCa[0], m_quadCa[3] - m_quadCa[1]);
	m_quadLengthCb = vec2(m_quadCb[2] - m_quadCb[0], m_quadCb[3] - m_quadCb[1]);

//	float blurRad = m_lightRadius / (lightPos.z - quadz) * (quadz - vq.z + 0.01) / dot(normalize(vl - vq), normal);
	m_blurCa = -m_lightRadius / (lightPos.z - quadz);
	m_blurCb = -m_blurCa * (quadz + 0.00);

#if 0
	vec3 outerQuad[4];
	std::cout << "shadow center = " << 	m_centerCa * -0.5 + m_centerCb << std::endl;
	for(int i = 0; i < 4; i++)
		std::cout << "quad[" << i << "] = " << quad[i] << std::endl;

	// Generate shadow quads for different blur levels on the caster quad, for later extrusion
	vec3 shadowQuad[4];

	float blurRad = 50.0 / 256.0;
	float offset = blurRad * (quad[1].x - quad[0].x) / texQuadLength;
	shadowQuad[0] = quad[0] + vec3(-offset, -offset, 0.0);
	shadowQuad[1] = quad[1] + vec3(offset, -offset, 0.0);
	shadowQuad[2] = quad[2] + vec3(offset, offset, 0.0);
	shadowQuad[3] = quad[3] + vec3(-offset, offset, 0.0);

	// Now extrude each shadow quad to the right place, ie, where each corner just reaches the blur radius
	for(int j = 0; j < 4; j++) {
		float blurBias = 1.0 / dot(normalize(lightPos - quad[j]), m_newNormal);
		// In this case, i = 0 always results in biggest tangent
		// blur radius at vz for p = shadowQuad[j]:
		// (blurCa * vz + blurCb) * blurBias * texQuadLength / (m_quadLengthCa * vz + m_quadLengthCb) = blurRad;
		float vz = (blurRad * m_quadLengthCb - blurBias * texQuadLength * m_blurCb) / (blurBias * texQuadLength * m_blurCa - blurRad * m_quadLengthCa);
		// the coord for the corner j at vz is:
		vec3 p = shadowQuad[j];
		p = (p - lightPos) / (p.z - lightPos.z) * (vz - lightPos.z) + lightPos;
		p = transpose(m_cMat) * p;
		p = (p - m_lightPos) / (p.z - m_lightPos.z) * (m_groundZ - m_lightPos.z) + m_lightPos;
		m_outerQuad[j] = p;
	}

#else
	vec3 hardQuad[4];
	for(int i = 0; i < 4; i++) {
		hardQuad[i] = m_cMat * m_hardQuad[i];
		vec3 hCenter = vec3(m_centerCa * hardQuad[i].z + m_centerCb, hardQuad[i].z);
		float blurRad = (m_blurCa * hardQuad[i].z + m_blurCb);
		vec2 sSize = m_quadLengthCa * hardQuad[i].z + m_quadLengthCb;
		float scaleSize = sampleHardShadowLength / fmax(sSize.x, sSize.y);
		blurRad = scaleSize * blurRad * 256.0;
//		std::cout << "myBlurRad = " << blurRad << std::endl;
//		m_outerQuad[i] = transpose(m_cMat) * ( (hardQuad[i] - hCenter) * (1.0 + sqrt(blurRad) / 10.0 * 0.5) + hCenter );
//		m_outerQuad[i] = transpose(m_cMat) * ( (hardQuad[i] - hCenter) * (1.0 + fmin(blurRad, 30.0) / 30.0 * 0.5) + hCenter );
		m_outerQuad[i] = transpose(m_cMat) * ( (hardQuad[i] - hCenter) * (1.0 + fmin(blurRad, 45.0) / 45.0 * 0.5) + hCenter );
		if(m_quad[i].z > m_groundZ)
			m_outerQuad[i] = (m_outerQuad[i] - m_quad[i]) / (m_outerQuad[i].z - m_quad[i].z) * (m_groundZ - m_quad[i].z) + m_quad[i];
//		m_outerQuad[i] = m_hardQuad[i];
//		std::cout << "m_outerQuad[" << i << "] = " << m_outerQuad[i] << std::endl;
//		std::cout << "m_hardQuad[" << i << "] = " << m_hardQuad[i] << std::endl;
	}
//	m_outerQuad[0] = vec3(-1.0, -1.0, -0.5);
//	m_outerQuad[1] = vec3( 1.0, -1.0, -0.5);
//	m_outerQuad[2] = vec3( 1.0,  1.0, -0.5);
//	m_outerQuad[3] = vec3(-1.0,  1.0, -0.5);
#endif

#if 0  // Shader codes test
	// Sample vq
	std:: cout << (vec3(-0.4, 0.0, 0.0) - m_lightPos) / (0.0 - m_lightPos.z) * (-0.5 - m_lightPos.z) + m_lightPos<< std::endl;
	vec3 vq = vec3( -0.67907, -0.0465116, -0.5 );
	vq = m_cMat * vq;
	std::cout << "vq = " << vq << std::endl;

	{
		vec3 vl = lightPos;
		vec2 vo = m_centerCa * vq.z + m_centerCb;
		float scaleSize = sampleHardShadowLength / (m_quadLengthCa * vq.z + m_quadLengthCb).x;

		float blurRad = (m_blurCa * vq.z + m_blurCb);
		std::cout << "blurRad = " << blurRad << std::endl;
		float sampleBlurRad = blurRad * scaleSize * 256;
		std::cout << "sampleBlurRad = " << sampleBlurRad << std::endl;

		blurRad = blurRad / dot(normalize(vl - vq), m_newNormal);
		std::cout << "blurRad = " << blurRad << std::endl;
		std::cout << "scaleSize = " << scaleSize << std::endl;
		sampleBlurRad = blurRad * scaleSize * 256;
		std::cout << "sampleBlurRad = " << sampleBlurRad << std::endl;

		vec2 shadowCoord = (vq.xy() - vo) * scaleSize;
		std::cout << "shadowCoord = " << shadowCoord << std::endl;
	}
#endif

//	std::cout << "---------- Quad End ------------\n" << std::endl;

	vec3 vertices[10];
	// Store the geometry for quad
	vertices[0] = m_quad[0];
	vertices[1] = m_quad[1];
	vertices[2] = m_quad[3];
	vertices[3] = m_quad[2];

	if(glIsBuffer(m_quadID)) {
		glBindBuffer(GL_ARRAY_BUFFER, m_quadID);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(GLfloat) * 12, vertices);
	} else {
		glGenBuffers(1, &m_quadID);
		glBindBuffer(GL_ARRAY_BUFFER, m_quadID);
		glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 12, vertices, GL_DYNAMIC_DRAW);
	}

	// Store the geometry for hard quad
	for(int i = 0; i < 4; i++) {
		memcpy(vertices[2*i], m_quad[i], sizeof(GLfloat) * 3);
		memcpy(vertices[2*i+1], m_hardQuad[i], sizeof(GLfloat) * 3);
	}
	memcpy(vertices[8], m_quad[0], sizeof(GLfloat) * 3);
	memcpy(vertices[9], m_hardQuad[0], sizeof(GLfloat) * 3);

	if(glIsBuffer(m_hardQuadID)) {
		glBindBuffer(GL_ARRAY_BUFFER, m_hardQuadID);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
	} else {
		glGenBuffers(1, &m_hardQuadID);
		glBindBuffer(GL_ARRAY_BUFFER, m_hardQuadID);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);
	}

	// Store the geometry for outer quad
	for(int i = 0; i < 4; i++) {
		memcpy(vertices[2*i], m_quad[i], sizeof(GLfloat) * 3);
		memcpy(vertices[2*i+1], m_outerQuad[i], sizeof(GLfloat) * 3);
	}
	memcpy(vertices[8], m_quad[0], sizeof(GLfloat) * 3);
	memcpy(vertices[9], m_outerQuad[0], sizeof(GLfloat) * 3);

	if(glIsBuffer(m_outerQuadID)) {
		glBindBuffer(GL_ARRAY_BUFFER, m_outerQuadID);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
	} else {
		glGenBuffers(1, &m_outerQuadID);
		glBindBuffer(GL_ARRAY_BUFFER, m_outerQuadID);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);
	}

	// Store the geometry for the top quad, basically a quad that is a bit lower than the quad itself
	vertices[0] = vec3(m_quad[0].x, m_quad[0].y, m_quad[0].z - 0.01);
	vertices[1] = vec3(m_quad[1].x, m_quad[1].y, m_quad[1].z - 0.01);
	vertices[2] = vec3(m_quad[3].x, m_quad[3].y, m_quad[3].z - 0.01);
	vertices[3] = vec3(m_quad[2].x, m_quad[2].y, m_quad[2].z - 0.01);

	if(glIsBuffer(m_topQuadID)) {
		glBindBuffer(GL_ARRAY_BUFFER, m_topQuadID);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(GLfloat) * 12, vertices);
	} else {
		glGenBuffers(1, &m_topQuadID);
		glBindBuffer(GL_ARRAY_BUFFER, m_topQuadID);
		glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 12, vertices, GL_DYNAMIC_DRAW);
	}

	if(m_frontFacing) {
		m_currentTexInfo = m_frontTexInfo;
	} else {
		m_currentTexInfo = m_backTexInfo;
	}

	// No idea what this do
	if(m_currentTexInfo._alphaTex == 0)
		m_color.w = 1.0;
	else m_color.w = 0.0;

	if(m_isFlipped) {
		m_alphaCoord[1] = m_currentTexInfo._alphaCoord[0];
		m_alphaCoord[0] = m_currentTexInfo._alphaCoord[1];
		m_alphaCoord[3] = m_currentTexInfo._alphaCoord[2];
		m_alphaCoord[2] = m_currentTexInfo._alphaCoord[3];
	} else {
		m_alphaCoord[0] = m_currentTexInfo._alphaCoord[0];
		m_alphaCoord[1] = m_currentTexInfo._alphaCoord[1];
		m_alphaCoord[2] = m_currentTexInfo._alphaCoord[2];
		m_alphaCoord[3] = m_currentTexInfo._alphaCoord[3];
	}

	m_isPosUpdated = true;
	m_isShadowUpdated = true;

}
