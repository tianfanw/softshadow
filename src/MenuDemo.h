/*
 * MenuDemo.h
 *
 *  Created on: 2013-08-22
 *      Author: tiawang
 */

#ifndef MENUDEMO_H_
#define MENUDEMO_H_

#include "AbstractUI.h"

class MenuDemo : public AbstractUI {
public:
	MenuDemo(int surfaceWidth, int surfaceHeight);
	virtual ~MenuDemo();

	void onTouch(int x, int y);
	void onMove(int x, int y);
	void onRelease(int x, int y);
	void animate();
private:
	// Objects
	GLuint m_buttons[4], m_puller;

	// Controls
	bool m_animation, m_readyToStop, m_menuSelected, m_menuOpened;
	float m_movAngle, m_destAngle;
};

#endif /* MENUDEMO_H_ */
