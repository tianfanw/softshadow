/*
 * AbstractUI.cpp
 *
 *  Created on: 2013-08-22
 *      Author: tiawang
 */

#include "AbstractUI.h"
#include "bbutil.h"

AbstractUI::AbstractUI(int surfaceWidth, int surfaceHeight)
: m_surfaceWidth(surfaceWidth), m_surfaceHeight(surfaceHeight) {
	m_shadowRenderer = new ShadowRenderer(surfaceWidth, surfaceHeight);
};

AbstractUI::~AbstractUI() {
	delete m_shadowRenderer;
	std::cout << "AbstractUI destructed!" << std::endl;
}

int AbstractUI::loadTexture(const char* filename, bool shareShadow, GLuint sharedWith) {
	int imgWidth, imgHeight, rtn;
	float texWidth, texHeight, aspect;
	GLuint tex;

	if( (rtn = bbutil_load_texture(filename, &imgWidth, &imgHeight, &texWidth, &texHeight, &tex)) == EXIT_SUCCESS) {
		aspect = 1.0 * imgWidth / imgHeight;
		return m_shadowRenderer->addTexture(tex, aspect, texWidth, texHeight, shareShadow, sharedWith);
	} else {
		return 0;
	}
}

void AbstractUI::render() {
	 animate();
	 m_shadowRenderer->render();
}
