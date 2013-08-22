/*
 * FolderDemo.h
 *
 *  Created on: 2013-08-22
 *      Author: tiawang
 */

#ifndef FOLDERDEMO_H_
#define FOLDERDEMO_H_

#include "AbstractUI.h"

class FolderDemo : public AbstractUI {
public:
	FolderDemo(int surfaceWidth, int surfaceHeight);
	virtual ~FolderDemo();

	void onTouch(int x, int y);
	void onMove(int x, int y);
	void onRelease(int x, int y);
	void animate();
private:
	// Textures
	GLuint m_iconTextures[4], m_frontCoverTexture;

	// Objects
	GLuint m_icons[4], m_frontCover, m_backCover;

	// Controls
	bool m_animation;
};

#endif
