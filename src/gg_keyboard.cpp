/* -----------------------------------------------------------------------------
 *
 * Giada - Your Hardcore Loopmachine
 *
 * gg_keyboard
 *
 * -----------------------------------------------------------------------------
 *
 * Copyright (C) 2010-2015 Giovanni A. Zuliani | Monocasual
 *
 * This file is part of Giada - Your Hardcore Loopmachine.
 *
 * Giada - Your Hardcore Loopmachine is free software: you can
 * redistribute it and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * Giada - Your Hardcore Loopmachine is distributed in the hope that it
 * will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Giada - Your Hardcore Loopmachine. If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * -------------------------------------------------------------------------- */


#include "gg_keyboard.h"
#include "gd_browser.h"
#include "gd_mainWindow.h"
#include "gd_editor.h"
#include "gd_warnings.h"
#include "ge_channel.h"
#include "ge_sampleChannel.h"
#include "mixer.h"
#include "core/conf.h"
#include "const.h"
#include "glue.h"
#include "patch.h"
#include "core/channel.h"
#include "sampleChannel.h"
#include "log.h"


extern Mixer 		     G_Mixer;
extern Conf  		     G_Conf;
extern Patch 		     G_Patch;
extern gdMainWindow *mainWin;


int gKeyboard::indexColumn = 0;


/* -------------------------------------------------------------------------- */


gKeyboard::gKeyboard(int X, int Y, int W, int H)
: Fl_Scroll    (X, Y, W, H),
	bckspcPressed(false),
	endPressed   (false),
	spacePressed (false),
	addColumnBtn (NULL)
{
	color(COLOR_BG_MAIN);
	type(Fl_Scroll::BOTH_ALWAYS);
	scrollbar.color(COLOR_BG_0);
	scrollbar.selection_color(COLOR_BG_1);
	scrollbar.labelcolor(COLOR_BD_1);
	scrollbar.slider(G_BOX);
	hscrollbar.color(COLOR_BG_0);
	hscrollbar.selection_color(COLOR_BG_1);
	hscrollbar.labelcolor(COLOR_BD_1);
	hscrollbar.slider(G_BOX);

	addColumnBtn = new gClick(8, y(), 200, 20, "Add new column");
	addColumnBtn->callback(cb_addColumn, (void*) this);
	add(addColumnBtn);

	init();
}


/* -------------------------------------------------------------------------- */


void gKeyboard::init()
{
	/* add 6 empty columns as init layout */

	__cb_addColumn();
	__cb_addColumn();
	__cb_addColumn();
	__cb_addColumn();
	__cb_addColumn();
	__cb_addColumn();
}


/* -------------------------------------------------------------------------- */


void gKeyboard::freeChannel(gChannel *gch)
{
	gch->reset();
}


/* -------------------------------------------------------------------------- */


void gKeyboard::deleteChannel(gChannel *gch)
{
	for (unsigned i=0; i<columns.size; i++) {
		int k = columns.at(i)->find(gch);
		if (k != columns.at(i)->children()) {
			columns.at(i)->deleteChannel(gch);
			return;
		}
	}
}


/* -------------------------------------------------------------------------- */


void gKeyboard::updateChannel(gChannel *gch)
{
	gch->update();
}


/* -------------------------------------------------------------------------- */


void gKeyboard::organizeColumns()
{
	/* if only one column exists don't cleanup: the initial column must
	 * stay here. */

	if (columns.size == 1)
		return;

	/* otherwise delete all empty columns */
	/** FIXME - this for loop might not work correctly! */

	for (unsigned i=columns.size-1; i>=1; i--) {
		if (columns.at(i)->isEmpty()) {
			//Fl::delete_widget(columns.at(i));
			delete columns.at(i);
			columns.del(i);
		}
	}

	/* compact column, avoid empty spaces */

	for (unsigned i=1; i<columns.size; i++)
		columns.at(i)->position(columns.at(i-1)->x() + columns.at(i-1)->w() + 16, y());

	addColumnBtn->position(columns.last()->x() + columns.last()->w() + 16, y());

	redraw();
}


/* -------------------------------------------------------------------------- */


void gKeyboard::cb_addColumn(Fl_Widget *v, void *p) { ((gKeyboard*)p)->__cb_addColumn(); }


/* -------------------------------------------------------------------------- */


gChannel *gKeyboard::addChannel(int colIndex, Channel *ch, bool build)
{
	gColumn *col = getColumn(colIndex);

	/* no column with index 'colIndex' found? Just create it and set its index
	to 'colIndex'. */

	if (!col) {
		__cb_addColumn();
		col = columns.last();
		col->setIndex(colIndex);
		gLog("[gKeyboard::addChannel] created new column with index=%d\n", colIndex);
	}

	gLog("[gKeyboard::addChannel] add to column with index = %d\n", col->getIndex());
	return col->addChannel(ch);
}


/* -------------------------------------------------------------------------- */


void gKeyboard::refreshColumns()
{
	for (unsigned i=0; i<columns.size; i++)
		columns.at(i)->refreshChannels();
}


/* -------------------------------------------------------------------------- */


gColumn *gKeyboard::getColumn(int index)
{
	for (unsigned i=0; i<columns.size; i++)
		if (columns.at(i)->getIndex() == index)
			return columns.at(i);
	return NULL;
}


/* -------------------------------------------------------------------------- */


int gKeyboard::handle(int e)
{
	int ret = Fl_Group::handle(e);  // assume the buttons won't handle the Keyboard events
	switch (e) {
		case FL_FOCUS:
		case FL_UNFOCUS: {
			ret = 1;                	// enables receiving Keyboard events
			break;
		}
		case FL_SHORTCUT:           // in case widget that isn't ours has focus
		case FL_KEYDOWN:            // Keyboard key pushed
		case FL_KEYUP: {            // Keyboard key released

			/* rewind session. Avoid retrigs */

			if (e == FL_KEYDOWN) {
				if (Fl::event_key() == FL_BackSpace && !bckspcPressed) {
					bckspcPressed = true;
					glue_rewindSeq();
					ret = 1;
					break;
				}
				else if (Fl::event_key() == FL_End && !endPressed) {
					endPressed = true;
					glue_startStopInputRec(false);  // update gui
					ret = 1;
					break;
				}
				else if (Fl::event_key() == FL_Enter && !enterPressed) {
					enterPressed = true;
					glue_startStopActionRec();
					ret = 1;
					break;
				}
				else if (Fl::event_key() == ' ' && !spacePressed) {
					spacePressed = true;
					G_Mixer.running ? glue_stopSeq() : glue_startSeq(); // TODO - glue_startStopSeq, no core logic here
					ret = 1;
					break;
				}
			}
			else if (e == FL_KEYUP) {
				if (Fl::event_key() == FL_BackSpace)
					bckspcPressed = false;
				else if (Fl::event_key() == FL_End)
					endPressed = false;
				else if (Fl::event_key() == ' ')
					spacePressed = false;
				else if (Fl::event_key() == FL_Enter)
					enterPressed = false;
			}

			/* Walk button arrays, trying to match button's label with the Keyboard event.
			 * If found, set that button's value() based on up/down event,
			 * and invoke that button's callback() */

			for (unsigned i=0; i<columns.size; i++)
				for (int k=1; k<columns.at(i)->children(); k++)
					ret &= ((gChannel*)columns.at(i)->child(k))->keyPress(e);
			break;
		}
	}
	return ret;
}


/* -------------------------------------------------------------------------- */


void gKeyboard::clear()
{
	for (unsigned i=0; i<columns.size; i++)
		delete columns.at(i);
	columns.clear();
	indexColumn = 0;     // new columns will start from index=0
	addColumnBtn->position(8, y());
}


/* -------------------------------------------------------------------------- */


void gKeyboard::setChannelWithActions(gSampleChannel *gch)
{
	if (gch->ch->hasActions)
		gch->addActionButton();
	else
		gch->delActionButton();
}


/* -------------------------------------------------------------------------- */


void gKeyboard::printChannelMessage(int res)
{
	if      (res == SAMPLE_NOT_VALID)
		gdAlert("This is not a valid WAVE file.");
	else if (res == SAMPLE_MULTICHANNEL)
		gdAlert("Multichannel samples not supported.");
	else if (res == SAMPLE_WRONG_BIT)
		gdAlert("This sample has an\nunsupported bit-depth (> 32 bit).");
	else if (res == SAMPLE_WRONG_ENDIAN)
		gdAlert("This sample has a wrong\nbyte order (not little-endian).");
	else if (res == SAMPLE_WRONG_FORMAT)
		gdAlert("This sample is encoded in\nan unsupported audio format.");
	else if (res == SAMPLE_READ_ERROR)
		gdAlert("Unable to read this sample.");
	else if (res == SAMPLE_PATH_TOO_LONG)
		gdAlert("File path too long.");
	else
		gdAlert("Unknown error.");
}

/* -------------------------------------------------------------------------- */


void gKeyboard::__cb_addColumn()
{
	int colx;
	int colxw;
	int colw = 380;
	if (columns.size == 0) {
		colx  = x() - xposition();  // mind the offset with xposition()
		colxw = colx + colw;
	}
	else {
		gColumn *prev = columns.last();
		colx  = prev->x()+prev->w() + 16;
		colxw = colx + colw;
	}

	/* add gColumn to gKeyboard and to columns vector */

	gColumn *gc = new gColumn(colx, y(), colw-20, 2000, indexColumn, this);
  add(gc);
	columns.add(gc);
	indexColumn++;

	/* move addColumn button */

	addColumnBtn->position(colxw-4, y());
	redraw();

	gLog("[gKeyboard::__cb_addColumn] new column added (index = %d), total count=%d, addColumn(x)=%d\n",
		gc->getIndex(), columns.size, addColumnBtn->x());
}
