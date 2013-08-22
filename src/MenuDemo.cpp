/*
 * MenuDemo.cpp
 *
 *  Created on: 2013-08-22
 *      Author: tiawang
 */

#include "MenuDemo.h"

MenuDemo::MenuDemo(int surfaceWidth, int surfaceHeight) : AbstractUI(surfaceWidth, surfaceHeight) {

	m_shadowRenderer->setLight(vec3(1.0, 0.0, 2.0), 0.5);

	m_buttons[0] = m_shadowRenderer->addObject(vec3(0.0, 0.0, 0.0), vec2(0.4, 0.4), vec3(1.0, 0.0, 0.0));
	m_buttons[1] = m_shadowRenderer->addObject(vec3(0.0, 0.0, 0.0), vec2(0.4, 0.4), vec3(0.0, 1.0, 0.0));
	m_buttons[2] = m_shadowRenderer->addObject(vec3(0.0, 0.0, 0.0), vec2(0.4, 0.4), vec3(1.0, 1.0, 0.0));
	m_buttons[3] = m_shadowRenderer->addObject(vec3(0.0, 0.0, 0.0), vec2(0.4, 0.4), vec3(0.0, 1.0, 1.0));
	m_puller = m_shadowRenderer->addObject(vec3(0.0, 0.0, 0.0), vec2(0.4, 0.1), vec3(0.0, 0.0, 0.0));

	m_shadowRenderer->getObject(m_buttons[0])->transform(Translate(0.8, 1.0, -0.05) * RotateY(5.0) * RotateX(90.0) * Translate(0.0, -0.2, 0.0));
	m_shadowRenderer->getObject(m_buttons[1])->transform(Translate(0.8, 1.0, -0.05) * RotateY(5.0) * RotateX(-90.0) * Translate(0.0, 0.2, 0.0));
	m_shadowRenderer->getObject(m_buttons[2])->transform(Translate(0.8, 1.0, -0.05) * RotateY(5.0) * RotateX(90.0) * Translate(0.0, -0.2, 0.0));
	m_shadowRenderer->getObject(m_buttons[3])->transform(Translate(0.8, 1.0, -0.05) * RotateY(5.0) * RotateX(-90.0) * Translate(0.0, 0.2, 0.0));
	m_shadowRenderer->getObject(m_puller)->transform(Translate(0.8, 1.0 - 0.05, -0.05) * RotateY(5.0));
}

MenuDemo::~MenuDemo() {

}

void MenuDemo::onTouch(int x, int y) {
	float coord[2];
	coord[0] = 2.0 * x / m_surfaceWidth - 1.0;
	coord[1] = 1.0 - 2.0 * y / m_surfaceHeight;
	if(!m_menuOpened && coord[0] >= 0.7 && coord[1] >= 0.7) {
		m_menuSelected = true;
		m_destAngle = 0.0;
	} else if (m_menuOpened && coord[0] >= 0.7 && coord[1] <= -0.6) {
		m_menuSelected = true;
		m_destAngle = 90.0;
	}
}

static float clamp(float x, float a, float b)
{

    return x < a ? a : (x > b ? b : x);

}

void MenuDemo::onMove(int x, int y) {
	float coord[2];
	if(m_menuSelected) {
		coord[0] = 2.0 * x / m_surfaceWidth - 1.0;
		coord[1] = 1.0 - 2.0 * y / m_surfaceHeight;
		m_destAngle = clamp(asin(clamp((1.0 - coord[1]) / 1.6, 0.0, 1.0)) * 180.0 / M_PI, 0.0, 90.0);
	}
}

void MenuDemo::onRelease(int x, int y) {
	float coord[2];
	if(m_menuSelected) {
		coord[0] = 2.0 * x / m_surfaceWidth - 1.0;
		coord[1] = 1.0 - 2.0 * y / m_surfaceHeight;
		if(coord[1] < 0.0) {
			m_menuOpened = true;
		} else {
			m_menuOpened = false;
		}
		std::cout << "release!" << std::endl;
		m_menuSelected = false;
	}
}

void MenuDemo::animate() {
	if(m_menuSelected) {
		m_animation = true;
		m_movAngle = m_destAngle;
	} else {
		if(m_menuOpened) {
			if(m_movAngle < 90.0) {
				m_movAngle += 3.0;
				if(m_movAngle > 90.0) {
					m_movAngle = 90.0;
					m_readyToStop = true;
				}
			}
		} else if(!m_menuOpened) {
			if(m_movAngle > 0.0) {
				m_movAngle -= 3.0;
				if(m_movAngle < 0.0) {
					m_movAngle = 0.0;
					m_readyToStop = true;
				}
			}
		}
	}

	if(m_animation) {
		m_shadowRenderer->getObject(m_buttons[0])->transform(Translate(0.8, 1.0, -0.05) * RotateY(5.0) * RotateX(90.0 - m_movAngle) * Translate(0.0, -0.2, 0.0));
//		m_shadowRenderer->getObject(m_buttons[1])->transform(Translate(0.8, 1.0 - 0.8 * sin(m_movAngle * M_PI / 180.0), -0.05 - sin(rAngle * M_PI / 180.0)) * RotateY(5.0 - rAngle) * RotateX(m_movAngle - 90.0) * Translate(0.0, 0.2, 0.0));
		m_shadowRenderer->getObject(m_buttons[1])->transform(Translate(0.8, 1.0 - 0.8 * sin(m_movAngle * M_PI / 180.0), -0.05) * RotateY(5.0) * RotateX(m_movAngle - 90.0) * Translate(0.0, 0.2, 0.0));
		m_shadowRenderer->getObject(m_buttons[2])->transform(Translate(0.8, 1.0 - 0.8 * sin(m_movAngle * M_PI / 180.0), -0.05) * RotateY(5.0) * RotateX(90.0 - m_movAngle) * Translate(0.0, -0.2, 0.0));
		m_shadowRenderer->getObject(m_buttons[3])->transform(Translate(0.8, 1.0 - 1.6 * sin(m_movAngle * M_PI / 180.0), -0.05) * RotateY(5.0) * RotateX(m_movAngle - 90.0) * Translate(0.0, 0.2, 0.0));
		m_shadowRenderer->getObject(m_puller)->transform(Translate(0.8, 1.0 - 1.6 * sin(m_movAngle * M_PI / 180.0) - 0.05, -0.05) * RotateY(5.0));
		if(m_readyToStop) {
			m_readyToStop = false;
			m_animation = false;
		}
	}
}
