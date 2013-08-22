/*
 * FolderDemo.cpp
 *
 *  Created on: 2013-08-22
 *      Author: tiawang
 */

#include "FolderDemo.h"

FolderDemo::FolderDemo(int surfaceWidth, int surfaceHeight) : AbstractUI(surfaceWidth, surfaceHeight) {

	m_shadowRenderer->setLight(vec3(-1.0, 0.0, 2.0), 0.5);

	m_iconTextures[0] = loadTexture("app/native/assets/icon1.png");
	m_iconTextures[1] = loadTexture("app/native/assets/icon2.png");
	m_iconTextures[2] = loadTexture("app/native/assets/icon3.png");
	m_iconTextures[3] = loadTexture("app/native/assets/icon4.png");
	m_frontCoverTexture = loadTexture("app/native/assets/cover.png", true, 0);  // share shadow with default square

	m_backCover = m_shadowRenderer->addObject(vec3(0.0, 0.0, -0.45), vec2(0.6, 0.6), vec3(0.6, 0.6, 1.0));
	m_frontCover = m_shadowRenderer->addObject(vec3(0.0, 0.0, -0.45), vec2(0.6, 0.6), vec3(0.6, 0.6, 1.0));
	m_shadowRenderer->bindTexture(m_frontCover, m_frontCoverTexture, 0);

	for(int i = 0; i < 4; i++) {
		m_icons[i] = m_shadowRenderer->addObject(vec3(0.14, 0.0, -0.45), vec2(0.2, 0.2), vec3(1.0, 1.0, 1.0));
		m_shadowRenderer->bindTexture(m_icons[i], m_iconTextures[i]);
	}

	// Rotate the front cover a bit
	vec3 center = m_shadowRenderer->getObject(m_frontCover)->origCenter();
	vec2 size = m_shadowRenderer->getObject(m_frontCover)->origSize();
	vec3 offset = center - vec3(size.x / 2.0, 0.0, 0.0);
	m_shadowRenderer->getObject(m_frontCover)->transform(Translate(offset) * RotateY(-20.0) * Translate(-offset));

	m_animation = false;
}

FolderDemo::~FolderDemo() {

}

void FolderDemo::onTouch(int x, int y) {
	m_animation = !m_animation;
}

void FolderDemo::onMove(int x, int y) {

}

void FolderDemo::onRelease(int x, int y) {

}

void FolderDemo::animate() {
	static int stage = 0;
	static int frame = 0;
	static int maxFrame[4] = {100, 100, 30, 30};
	static float frontCoverAngle = -20.0;
	static float iconAngles[4] = {0.0, 0.0, 0.0, 0.0};
	static float scale = 0.0;
	static float zoffset = 0.0;

	if(m_animation) {
		if(stage == 0) {
			scale += 1.0 / maxFrame[0];
			frontCoverAngle -= 120.0 / maxFrame[0];
			for(int i = 0; i < 4; i++)
				iconAngles[i] -= (50.0 * i + 20.0 ) / maxFrame[1];
			zoffset += 0.2 / maxFrame[0];
			frame++;
			if(frame >= maxFrame[0]) { stage++; frame = 0; m_animation = false; }
		} else{
			scale -= 1.0 / maxFrame[1];
			frontCoverAngle += 120.0 / maxFrame[1];
			for(int i = 0; i < 4; i++)
				iconAngles[i] += (50.0 * i + 20.0 ) / maxFrame[1];
			zoffset -= 0.2 / maxFrame[1];
			frame++;
			if(frame >= maxFrame[1]) { stage = 0; frame = 0; m_animation = false; }
		}

		Quad* quad;
		vec3 center;
		vec2 size;
		vec3 offset;
		mat4 folderMat;
		quad = m_shadowRenderer->getObject(m_backCover);
		center = quad->origCenter();
		size = quad->origSize();
		folderMat = Translate(0.0, 0.0, zoffset) * Translate(center) * Scale(1.0 + scale, 1.0 + scale, 1.0) * Translate(-center);
		quad->transform(folderMat);

		{
			Quad* quad = m_shadowRenderer->getObject(m_frontCover);
			vec3 center = quad->origCenter();
			vec2 size = quad->origSize();
			vec3 offset = vec3(size.x * (1.0 + scale) / 2.0, 0.0, 0.0);
			mat4 transMat = Translate(0.0, 0.0, zoffset) * Translate(center) * Translate(-offset) * RotateY(frontCoverAngle) * Translate(offset)
					* Scale(1.0 + scale, 1.0 + scale, 1.0) * Translate(-center);
//			std::cout << "zoffset = " << zoffset << std::endl;
//			std::cout << "angle[1] = " << angle[1] << std::endl;
//			std::cout << "scale = " << scale << std::endl;
			quad->transform(transMat);
		}

		for(int i = 0; i < 4; i++) {
			Quad* quad = m_shadowRenderer->getObject(m_icons[i]);
			vec3 center = quad->origCenter();
			vec2 size = quad->origSize();
			vec3 offset = vec3(size.x * (1.0 + scale) / 2.0, 0.0, 0.0);
			mat4 transMat = folderMat * Translate(0.0, 0.0, center.z) * RotateY(iconAngles[i]) * Translate(0.0, 0.0, -center.z);
			quad->transform(transMat);
		}
	}
}
