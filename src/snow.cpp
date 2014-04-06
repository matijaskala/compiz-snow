/**
 *
 * Compiz snow plugin
 *
 * snow.cpp
 *
 * Copyright (c) 2006 Eckhart P. <beryl@cornergraf.net>
 * Copyright (c) 2006 Brian Jørgensen <qte@fundanemt.com>
 * Maintained by Danny Baumann <maniac@opencompositing.org>
 * Copyright (c) 2012 Matija Skala <mskala@gmx.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 **/

/*
 * Many thanks to Atie H. <atie.at.matrix@gmail.com> for providing
 * a clean plugin template
 * Also thanks to the folks from #beryl-dev, especially Quinn_Storm
 * for helping me make this possible
 */

#include <snow/snow.h>

COMPIZ_PLUGIN_20090315 (snow, SnowVTable)

void SnowScreen::SnowFlake::think (int ms)
{
	SnowScreen* snowScreen = get(screen);
	int boxing = snowScreen->optionGetScreenBoxing ();
	if (
	y >= screen->height() + boxing || y <= -boxing ||
	x >= screen->width () + boxing || x <= -boxing ||
	z <= -snowScreen->optionGetScreenDepth() / 500.0 || z >= 1)
		put();
	else
	{
		float tmp = ms / (101.0f - get(screen)->optionGetSnowSpeed());
		x += xs * tmp;
		y += ys * tmp;
		z += zs * tmp;
		ra+= ms / (10.0f - rs);
	}
}

bool SnowScreen::step ()
{
	if(!activated)
		return true;
	int ms = optionGetSnowUpdateDelay();
	foreach (SnowFlake& snowFlake, snowFlakes)
		snowFlake.think(ms);
	if (optionGetSnowOverWindows())
		cScreen->damageScreen();
	else
		foreach (CompWindow *w, screen->windows())
			if (w->type() & CompWindowTypeDesktopMask)
				CompositeWindow::get(w)->addDamage ();
	return true;
}

bool SnowScreen::snowToggle (CompAction* action, CompAction::State state, CompOption::Vector &opt)
{
	activated = !activated;
	if(!activated)
		cScreen->damageScreen();
	return true;
}

inline float mmRand (int min, int max, float divisor)
{
	return (rand() % (max - min + 1) + min) / divisor;
};

void SnowScreen::setupDisplayList (float size)
{
	displayList = glGenLists (1);

	glNewList (displayList, GL_COMPILE);
	glBegin (GL_QUADS);

	glColor3f (1.0, 1.0, 1.0);
	glVertex2f (0, 0);
	glColor3f (1.0, 1.0, 1.0);
	glVertex2f (0, size);
	glColor3f (1.0, 1.0, 1.0);
	glVertex2f (size, size);
	glColor3f (1.0, 1.0, 1.0);
	glVertex2f (size, 0);

	glEnd ();
	glEndList ();
}

SnowScreen::SnowTexture* SnowScreen::SnowTexture::create(CompString &s)
{
	CompSize compSize;
	GLTexture::List list = GLTexture::readImageToTexture(s,pname,compSize);
	if(list.empty())
		return NULL;

	int snowSize = get(screen)->optionGetSnowSize();
	int snowHeight = snowSize * compSize.height() / compSize.width();

	GLTexture* texture = list.front();
	SnowTexture* snowTexture = new SnowTexture;
	GLTexture::incRef(texture);
	snowTexture->texture = texture;
	snowTexture->dList = glGenLists (1);
	glNewList (snowTexture->dList, GL_COMPILE);

	glBegin (GL_QUADS);
		glTexCoord2f (COMP_TEX_COORD_X (texture->matrix(), 0),
			      COMP_TEX_COORD_Y (texture->matrix(), 0));
		glVertex2f (0, 0);
		glTexCoord2f (COMP_TEX_COORD_X (texture->matrix(), 0),
			      COMP_TEX_COORD_Y (texture->matrix(), compSize.height()));
		glVertex2f (0, snowHeight);
		glTexCoord2f (COMP_TEX_COORD_X (texture->matrix(), compSize.width()),
			      COMP_TEX_COORD_Y (texture->matrix(), compSize.height()));
		glVertex2f (snowSize, snowHeight);
		glTexCoord2f (COMP_TEX_COORD_X (texture->matrix(), compSize.width()),
			      COMP_TEX_COORD_Y (texture->matrix(), 0));
		glVertex2f (snowSize, 0);
	glEnd ();

	glEndList ();

	return snowTexture;
}

SnowScreen::SnowTexture::~SnowTexture()
{
	glDeleteLists (dList, 1);
	GLTexture::decRef(texture);
}

void SnowScreen::SnowFlake::render ()
{
	SnowScreen* snowScreen = get(screen);
	bool snowRotation = snowScreen->optionGetSnowRotation();
	glTranslatef(x,y,z);
	if(snowRotation)
		glRotatef(ra,0,0,1);
	if (snowScreen->optionGetUseTextures() && tex)
	{
		tex->texture->enable(GLTexture::Good);
		glCallList(tex->dList);
		tex->texture->disable();
	}
	else
	{
		glCallList(snowScreen->displayList);
	}
	if(snowRotation)
		glRotatef(-ra,0,0,1);
	glTranslatef(-x,-y,-z);
}

void SnowScreen::beginRendering ()
{
	bool useBlending = optionGetUseBlending ();
	if (useBlending)
		glEnable (GL_BLEND);
	glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glColor3f (1.0, 1.0, 1.0);
	foreach (SnowFlake& snowFlake, snowFlakes)
		snowFlake.render();
	glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	if (useBlending)
	{
		glDisable (GL_BLEND);
		glBlendFunc (GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
	}
}

bool SnowScreen::glPaintOutput (const GLScreenPaintAttrib &sa, const GLMatrix &transform, const CompRegion &region, CompOutput *output, unsigned int mask)
{
    if (activated && !optionGetSnowOverWindows ()) {/*
	GLMatrix trans = transform;
	trans.toScreenSpace (output, -DEFAULT_Z_CAMERA);

	glPushMatrix ();
	glLoadMatrixf (trans.getMatrix());
	beginRendering ();
	glPopMatrix ();*/
	mask |= PAINT_SCREEN_WITH_TRANSFORMED_WINDOWS_MASK;
    }

    bool status = gScreen->glPaintOutput (sa, transform, region, output, mask);

    if (activated && optionGetSnowOverWindows())
    {
	GLMatrix trans = transform;
	trans.toScreenSpace (output, -DEFAULT_Z_CAMERA);

	glPushMatrix ();
	glLoadMatrixf (trans.getMatrix());
	beginRendering ();
	glPopMatrix ();
    }

    return status;
}

bool SnowWindow::glDraw (const GLMatrix& transform, const GLWindowPaintAttrib& attrib, const CompRegion& region, unsigned mask)
{
    SnowScreen *ss = SnowScreen::get(screen);
    /* First draw Window as usual */
    bool status = gWindow->glDraw (transform, attrib, region, mask);

    /* Check whether this is the Desktop Window */
    if (ss->activated && (window->type() & CompWindowTypeDesktopMask) && !optionGetSnowOverWindows ()) {
		ss->beginRendering ();}
    return status;
}

void SnowScreen::SnowFlake::set()
{
	SnowScreen* snowScreen = get(screen);
	int boxing = snowScreen->optionGetScreenBoxing ();
	x = mmRand (-boxing, screen->width() + boxing, 1);
	y = mmRand (-boxing, screen->height() + boxing, 1);
	z = mmRand (-snowScreen->optionGetScreenDepth(), 0.1, 5000);
	switch (snowScreen->optionGetSnowDirection()) {
		case SnowOptions::SnowDirectionTopToBottom:
			xs = mmRand (-10, 10, 5000);
			ys = mmRand (10, 30, 10);
			break;
		case SnowOptions::SnowDirectionBottomToTop:
			xs = mmRand (-10, 10, 5000);
			ys = -mmRand (10, 30, 10);
			break;
		case SnowOptions::SnowDirectionRightToLeft:
			xs = -mmRand (10, 30, 10);
			ys = mmRand (-10, 10, 5000);
			break;
		case SnowOptions::SnowDirectionLeftToRight:
			xs = mmRand (10, 30, 10);
			ys = mmRand (-10, 10, 5000);
			break;
	}
}

void SnowScreen::SnowFlake::put ()
{
	SnowScreen* snowScreen = get(screen);
	int boxing = snowScreen->optionGetScreenBoxing ();
    /* TODO: possibly place snowflakes based on FOV, instead of a cube. */
    switch (snowScreen->optionGetSnowDirection())
    {
    case SnowOptions::SnowDirectionTopToBottom:
	x  = mmRand (-boxing, screen->width() + boxing, 1);
	y  = mmRand (-boxing, 0, 1);
	break;
    case SnowOptions::SnowDirectionBottomToTop:
	x  = mmRand (-boxing, screen->width() + boxing, 1);
	y  = mmRand (screen->height(), screen->height() + boxing, 1);
	break;
    case SnowOptions::SnowDirectionRightToLeft:
	x  = mmRand (screen->width(), screen->width() + boxing, 1);
	y  = mmRand (-boxing, screen->height() + boxing, 1);
	break;
    case SnowOptions::SnowDirectionLeftToRight:
	x  = mmRand (-boxing, 0, 1);
	y  = mmRand (-boxing, screen->height() + boxing, 1);
	break;
    default:
	break;
    }
    z  = mmRand (-snowScreen->optionGetScreenDepth(), 0.1, 5000);
    zs = mmRand (-1000, 1000, 500000);
    ra = mmRand (-1000, 1000, 50);
    rs = mmRand (-1000, 1000, 1000);
}

void SnowScreen::updateFiles (CompOption::Value::Vector &snowTexFiles)
{
	foreach (SnowTexture* snowTexture, snowTex)
		delete snowTexture;
	snowTex.reserve(snowTexFiles.size());
	for(CompOption::Value::Vector::iterator i = snowTexFiles.begin(); i != snowTexFiles.end(); i++)
	{
		CompString s = i->s();
		SnowTexture* snowTexture = SnowTexture::create(s);
		if(snowTexture)
			snowTex.push_back(snowTexture);
		else
			compLogMessage("snow", CompLogLevelError, "image %s not found", s.c_str());
	}
}

SnowScreen::SnowScreen (CompScreen *screen) :
	PluginClassHandler< SnowScreen, CompScreen >(screen),
	compScreen(screen),
	cScreen(CompositeScreen::get(screen)),
	gScreen(GLScreen::get(screen)),
	activated(false), snowTex()
{
	ScreenInterface::setHandler(screen);
	CompositeScreenInterface::setHandler(cScreen);
	GLScreenInterface::setHandler(gScreen);
	ChangeNotify optionC = boost::bind(&SnowScreen::OptionChanged, this, _1, _2);
	optionSetToggleKeyInitiate (boost::bind(&SnowScreen::snowToggle, this, _1, _2, _3));
	optionSetNumSnowflakesNotify (optionC);
	optionSetSnowSizeNotify (optionC);
	optionSetSnowTexturesNotify (optionC);
	optionSetSnowSpeedNotify(optionC);
	optionSetSnowRotationNotify(optionC);
	optionSetScreenDepthNotify(optionC);
	optionSetScreenBoxingNotify(optionC);
	optionSetSnowDirectionNotify(optionC);
	optionSetUseTexturesNotify(optionC);
	optionSetUseBlendingNotify(optionC);
	optionSetSnowOverWindowsNotify(optionC);
	optionSetSnowUpdateDelayNotify(optionC);
	screen->addAction(&get(screen)->optionGetToggleKey());
	connection = Glib::signal_timeout().connect(sigc::mem_fun(*this, &SnowScreen::step), optionGetSnowUpdateDelay());
	setupDisplayList(optionGetSnowSize ());
	updateFiles(optionGetSnowTextures ());
	snowFlakes.resize(optionGetNumSnowflakes());
	foreach (SnowFlake& snowFlake, snowFlakes)
		snowFlake.set();
	update();
}

SnowScreen::~SnowScreen () {
	foreach (SnowTexture* snowTexture, snowTex)
		delete snowTexture;
}

SnowWindow::SnowWindow (CompWindow *w) :
	PluginClassHandler< SnowWindow, CompWindow >(w),
	window(w),
	gWindow(GLWindow::get(w))
{
	WindowInterface::setHandler(window);
	GLWindowInterface::setHandler(gWindow);
}

void SnowScreen::OptionChanged (CompOption*opt, Options num)
{
	switch (num)
	{
		case SnowSize:
			setupDisplayList (optionGetSnowSize());
		case SnowTextures:
			snowTex.clear();
			updateFiles(optionGetSnowTextures ());
			update();
			break;
		case NumSnowflakes:
			snowFlakes.resize(optionGetNumSnowflakes());
	foreach (SnowFlake& snowFlake, snowFlakes)
		snowFlake.set();
			update();
			break;
		case SnowSpeed:
		case ScreenDepth:
		case UseTextures:
		case UseBlending:
		case SnowRotation:
		case ScreenBoxing:
		case SnowDirection:
		case SnowOverWindows:
		case ToggleKey:
		case OptionNum:
			break;
		case SnowUpdateDelay:
			connection.disconnect();
			connection = Glib::signal_timeout().connect(sigc::mem_fun(*this, &SnowScreen::step), optionGetSnowUpdateDelay());
			break;
	}
}

bool SnowVTable::init()
{
	if (!CompPlugin::checkPluginABI ("core", CORE_ABIVERSION))
		return false;
	if (!CompPlugin::checkPluginABI ("composite", COMPIZ_COMPOSITE_ABI))
		return false;
	if (!CompPlugin::checkPluginABI ("opengl", COMPIZ_OPENGL_ABI))
		return false;
	return true;
}
