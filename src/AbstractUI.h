/*
 * AbstractUI.h
 *
 *  Created on: 2013-08-22
 *      Author: tiawang
 */

#ifndef ABSTRACTUI_H_
#define ABSTRACTUI_H_

#include "ShadowRenderer/ShadowRenderer.h"

class AbstractUI {
public:
	AbstractUI(int surfaceWidth, int surfaceHeight);
	virtual ~AbstractUI();

	virtual void onTouch(int x, int y) {};
	virtual void onMove(int x, int y) {};
	virtual void onRelease(int x, int y) {};
	virtual void animate() {};

	int loadTexture(const char* filename, bool shareShadow = false, GLuint sharedWith = 0);
	void render();

protected:
	int m_surfaceWidth, m_surfaceHeight;
	ShadowRenderer *m_shadowRenderer;
};

#endif
