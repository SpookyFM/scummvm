/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "common/system.h"
#include "graphics/palette.h"
#include "macs2/view1.h"
#include "macs2/macs2.h"
#include "macs2/gameobjects.h"
#include <graphics/cursorman.h>

namespace Macs2 {
Character *View1::GetCharacterByIndex(uint16_t index) {
	// TODO: Consider a map
	for (Character *c : characters) {
		if (c->GameObject->Index == index) {
			return c;
		}
	}
	return nullptr;
}
View1::View1() : UIElement("View1") {
		_backgroundSurface = g_engine->_bgImageShip;
		int mode = (int)g_engine->_cursorMode;
		g_engine->_cursorData[mode][(g_engine->_cursorWidths[mode] >> 1) + (g_engine->_cursorHeights[0] >> 1) * g_engine->_cursorWidths[mode]] = 0xFF;
		CursorMan.replaceCursor(g_engine->_cursorData[mode], g_engine->_cursorWidths[mode], g_engine->_cursorHeights[mode], g_engine->_cursorWidths[mode] >> 1, g_engine->_cursorHeights[0] >> 1, 0);
		CursorMan.showMouse(true);

		// TODO: Check if this works like this
		Character *protagonist = new Character();
		// TODO: Need to properly handle the offset
		// TODO: Remember that the game starts enumerating objects at 1 and not at 0
		protagonist->GameObject = GameObjects::instance().Objects[0x0];
		// TODO: Need to consider the DRY principle for the properties
		protagonist->Position = protagonist->GameObject->Position;
		characters.push_back(protagonist);

		// inventoryItems.push_back(GameObjects::instance().Objects[0x8 - 1]);
	}

	AnimFrame *View1::GetInventoryIcon(GameObject *gameObject) {
		AnimFrame *result = new AnimFrame();
		Common::MemoryReadStream stream(gameObject->Blobs[5-1].data(), gameObject->Blobs[5-1].size());
		// TODO: Need to check how the offset really is calculated by the game code, this will not hold
		stream.seek(23, SEEK_SET);
		result->ReadFromStream(&stream);
		return result;
		// TODO: Think about proper memory management
	}

	void View1::drawDarkRectangle(uint16 x, uint16 y, uint16 width, uint16 height)
	{
		Graphics::ManagedSurface s = getSurface();
		for (uint16 xOffset = 0; xOffset < width; xOffset++) {
			for (uint16 yOffset = 0; yOffset < height; yOffset++) {
				const uint16 currentX = x + xOffset;
				const uint16 currentY = y + yOffset;
				const uint32 currentValue = s.getPixel(currentX, currentY);
				const uint32 newValue = g_engine->_shadingTable[currentValue];
				s.setPixel(currentX, currentY, newValue);
			}
		}
	}

	void View1::drawStringBackground(uint16 x, uint16 y, uint16 width, uint16 height) {
		Graphics::ManagedSurface s = getSurface();

		// TODO: Look up how we determine the width of the right border
		constexpr int borderWidth = 10;
		constexpr int highlightWidth = 2;
		// Draw the background
		// Draw the border segments
		drawDarkRectangle(x, y, width, height);

		// TODO: Is this the same calculation?
		uint16 xSegments = (width / g_engine->_borderWidth) + 1;
		uint16 ySegments = (height / g_engine->_borderHeight) + 1;

		

		// First the left side
		Common::Rect clippingRect(x, y, x + borderWidth, y + height);
		int currentX = x;
		int currentY = y;
		for (int iy = 0; iy < ySegments; iy++) {
			// DrawSprite(currentX, currentY, g_engine->_borderWidth, g_engine->_borderHeight, g_engine->_borderData, s);
			DrawSpriteClipped(currentX, currentY, clippingRect, g_engine->_borderWidth, g_engine->_borderHeight, g_engine->_borderData, s);
			currentY += g_engine->_borderHeight;
		}

		// Top
		clippingRect = Common::Rect(x, y, x + width, y + borderWidth);
		currentX = x;
		currentY = y;
		for (int ix = 0; ix < xSegments; ix++) {
			DrawSpriteClipped(currentX, currentY, clippingRect, g_engine->_borderWidth, g_engine->_borderHeight, g_engine->_borderData, s);
			currentX += g_engine->_borderWidth;
		}

		// Right
		// TODO: Need to figure out the margin here
		currentX = x + width - borderWidth;
		currentY = y;
		clippingRect = Common::Rect(currentX, y, x + width, y + height);
		for (int iy = 0; iy < ySegments; iy++) {
			DrawSpriteClipped(currentX, currentY, clippingRect, g_engine->_borderWidth, g_engine->_borderHeight, g_engine->_borderData, s);
			currentY += g_engine->_borderHeight;
		}

		// Bottom
		currentX = x;
		currentY = y + height - borderWidth;
		clippingRect = Common::Rect(x, currentY, x + width, y + height);
		for (int ix = 0; ix < xSegments; ix++) {
			DrawSpriteClipped(currentX, currentY, clippingRect, g_engine->_borderWidth, g_engine->_borderHeight, g_engine->_borderData, s);
			currentX += g_engine->_borderWidth;
		}

		// Highlight on the top and left
		// TODO: Check if it's also done to the inside of the frame
		// TODO: Refactor code to have less copy paste
		// First the left side
		clippingRect = Common::Rect(x, y, x + highlightWidth, y + height);
		currentX = x;
		currentY = y;
		for (int iy = 0; iy < ySegments; iy++) {
			// DrawSprite(currentX, currentY, g_engine->_borderWidth, g_engine->_borderHeight, g_engine->_borderData, s);
			DrawSpriteClipped(currentX, currentY, clippingRect, g_engine->_borderWidth, g_engine->_borderHeight, g_engine->_borderHighlightData, s);
			currentY += g_engine->_borderHeight;
		}

		// Top
		clippingRect = Common::Rect(x, y, x + width, y + highlightWidth);
		currentX = x;
		currentY = y;
		for (int ix = 0; ix < xSegments; ix++) {
			DrawSpriteClipped(currentX, currentY, clippingRect, g_engine->_borderWidth, g_engine->_borderHeight, g_engine->_borderHighlightData, s);
			currentX += g_engine->_borderWidth;
		}
	}

	void View1::drawBackgroundAnimations(Graphics::ManagedSurface &s) {
		for (int i = 0; i < g_engine->_numBackgroundAnimations; i++) {
			BackgroundAnimation& current = g_engine->_backgroundAnimations[i];
			AnimFrame &currentFrame = current.Frames[current.FrameIndex];
			DrawSprite(current.X, current.Y, currentFrame.Width, currentFrame.Height, currentFrame.Data, s);
		}
	}

	void View1::drawBackgroundAnimationNumbers(Graphics::ManagedSurface &s) {
		for (int i = 0; i < g_engine->_numBackgroundAnimations; i++) {
			BackgroundAnimation &current = g_engine->_backgroundAnimations[i];
			renderString(current.X, current.Y, Common::String::format("%u", i));
		}
	}

	void View1::renderString(uint16 x, uint16 y, Common::String s) {
		Graphics::ManagedSurface surf = getSurface();
		uint16 currentX = x;
		uint16 currentY = y;
		for (auto iter = s.begin(); iter != s.end(); iter++) {
			GlyphData data;
			bool found = g_engine->FindGlyph(*iter, data);
			if (found) {
				DrawSprite(currentX, currentY, data.Width, data.Height, data.Data, surf);
				currentX += data.Width + 1;
				// TODO: Add reference to where this is defined
			} else {
				// TODO: Different character for not found?
				currentX += 10;
			}
		}
	}

	void View1::showStringBox(const Common::StringArray &sa) {
		// TODO: Naive and hardcoded implementation
		int contentHeight = sa.size() * 10;
		int contentWidth = g_engine->MeasureStrings(sa);
		int borderWidth = 10;
		int padding = 3;
		int totalWidth = contentWidth + (borderWidth * padding) * 2;
		int totalHeight = contentHeight + (borderWidth * padding) * 2;

		drawStringBackground(20, 20, totalWidth, totalHeight);
		// TODO range based
		int lineOffset = borderWidth + padding;
		for (auto iter = sa.begin(); iter < sa.end(); iter++) {
			renderString(20 + borderWidth + padding, 20 + lineOffset, *iter);
			lineOffset += 10;
		}
	}

	void View1::drawGlyphs(Macs2::GlyphData *data, int count, uint16 x, uint16 y, Graphics::ManagedSurface& s) {
		uint16 currentX = x;
		uint16 currentY = y;
		for (int i = 0; i < count; i++) {
			const Macs2::GlyphData &currentData = data[i];
			if (currentX + currentData.Width > s.w) {
				currentY += currentData.Height;
				currentX = x;
			}
			DrawSprite(currentX, currentY, currentData.Width, currentData.Height, currentData.Data, s);
			currentX += currentData.Width;
		}
	}

	void View1::handleFading() {

		currentFadeValue -= fadeDelta;
		if (currentFadeValue < 0) {
			return;
		}

		byte *colors = new byte[256 * 3];
		// g_system->getPaletteManager()->grabPalette(colors, 0, 256);
		// Copy the untouched palette over
		memcpy(colors, g_engine->_palVanilla, 256 * 3);

		for (int i = 0; i < 256 * 3; i++) {
			if (colors[i] < currentFadeValue) {
				colors[i] = 0;
			} else {
				colors[i] -= currentFadeValue;
			}
			colors[i] = (colors[i] * 259 + 33) >> 6;
		}

		g_system->getPaletteManager()->setPalette(colors, 0, 256);

	}

	void View1::drawPathfindingPoints(Graphics::ManagedSurface &s) {
		for (int i = 0; i < 16; i++) {
			renderString(g_engine->_pathfindingPoints[2 * i], g_engine->_pathfindingPoints[2 * i + 1], "x");
		}
	}

	void View1::drawPath(Graphics::ManagedSurface &s) {
		if (g_engine->_path.size() < 2) {
			return;
		}
		for (int i = 0; i < g_engine->_path.size() - 1; i++) {
			s.drawLine(g_engine->_path[i].x, g_engine->_path[i].y, g_engine->_path[i + 1].x, g_engine->_path[i + 1].y, 0xFF);
		}
	}

	void View1::setStringBox(const Common::StringArray& sa) {
		_drawnStringBox = sa;
		_isShowingStringBox = true;

		// TODO: Change cursor, stop animations, hide again
		redraw();
	}

	void View1::clearStringBox() {
		_isShowingStringBox = false;
		speakingCharacter = nullptr;
		redraw();
		if (_continueScriptAfterUI) {
			_continueScriptAfterUI = false;
			// TODO: Check which one it should be
			g_engine->RunScriptExecutor(false);
		}
		
	}

	int View1::GetCharacterArrayIndex(const Character *c) const {
		// TODO: Check if there is a find function somewhere
		for (int i = 0; i < characters.size(); i++) {
			if (characters[i] == c) {
				return i;
			}
		}
		return -1;
	}

	void View1::startFading() {
		currentFadeValue = 0x40;
	}

	bool View1::msgFocus(const FocusMessage &msg) {
	//Common::fill(&_pal[0], &_pal[256 * 3], 0);
	// _offset = 128;
	return true;
}

	bool View1::msgMouseDown(const MouseDownMessage& msg)
	{
		if (msg._button == MouseMessage::MB_LEFT) {
			// Handle string boxes
			if (_isShowingStringBox) {
				clearStringBox();
				return true;
			}
			if (_isShowingInventory) {
				// Check if we hit an item
				// TODO: Skipping this for now while we only have one item
				if (inventoryItems.size() == 1) {
					activeInventoryItem = inventoryItems[0];
				}
				return true;
			}



			uint32 value = getSurface().getPixel(msg._pos.x, msg._pos.y);
			g_system->setWindowCaption(Common::String::format("%u,%u: %u", msg._pos.x, msg._pos.y, value));
			//g_engine->CalculatePath(Common::Point(154, 136), Common::Point(msg._pos.x, msg._pos.y));

			if (g_engine->_cursorMode == CursorMode::Walk) {
				// TODO: Should address the protagonist differently
				// TODO: Sort out the different modes and only define them once
				characters[0]->StartLerpTo(msg._pos, 1000);
				return true;
			}

			// Check if we hit something
			uint16_t index = GetHitObjectID(Common::Point(msg._pos.x, msg._pos.y));
			if (index != 0) {
				debug("*** New interaction started");
				// TODO: Mode hardcoded
				Script::MouseMode m = Script::MouseMode::Use;
				if (activeInventoryItem != nullptr) {
					m = Script::MouseMode::UseInventory;
				}
				g_engine->_scriptExecutor->_mouseMode = m;
				g_engine->_scriptExecutor->_charPosX = characters[0]->Position.x;
				g_engine->_scriptExecutor->_charPosY = characters[0]->Position.y;
				g_engine->_scriptExecutor->_interactedObjectID = index;
				g_engine->_scriptExecutor->_interactedOtherObjectID = activeInventoryItem != nullptr ? activeInventoryItem->Index + 0x0400 : 0x0000;

				// Set the script
				g_engine->_scriptExecutor->SetScript(Scenes::instance().CurrentSceneScript);
				// TODO: Not sure where the original code rewinds the script
				Scenes::instance().CurrentSceneScript->seek(0, SEEK_SET);
				g_engine->RunScriptExecutor(false);
			}
			return true;
		} else if (msg._button == MouseMessage::MB_RIGHT) {
			g_engine->NextCursorMode();
			int mode = (int)g_engine->_cursorMode;
			CursorMan.replaceCursor(g_engine->_cursorData[mode], g_engine->_cursorWidths[mode], g_engine->_cursorHeights[mode], g_engine->_cursorWidths[mode] >> 1, g_engine->_cursorHeights[0] >> 1, 0);
			return true;
		}
	}

bool View1::msgKeypress(const KeypressMessage &msg) {
	// Any keypress to close the view
	// close();
	if (msg.ascii == (uint16)'m') {
		// _backgroundSurface = g_engine->_map;
		_backgroundSurface = g_engine->_pathfindingMap;
		redraw();
	}
	else if (msg.ascii == (uint16)'b') {
		_backgroundSurface = g_engine->_bgImageShip;
		startFading();
		redraw();
	} else if (msg.ascii == (uint16)'s') {
		// g_engine->ExecuteScript(g_engine->_scriptStream);
		g_engine->RunScriptExecutor(true);
		// Also test the lerping
		characters[0]->StartLerpTo(Common::Point(200, 100), 5000);
	} else if (msg.ascii == (uint16)'i') {
		_isShowingInventory = !_isShowingInventory;
	}
	return true;
}

void View1::draw() {
	g_system->getPaletteManager()->setPalette(g_engine->_pal, 0, 256);

	handleFading();
	
	Graphics::ManagedSurface s = getSurface();

	s.blitFrom(_backgroundSurface);

	// Draw the character

	// uint16 charX = 50;
	// uint16 charY = 100;
	// TODO: I don't have the right offset yet plus there must be some trick to reading sequential frames, probl. need
	// to seek in between frames
	// AnimFrame &f = g_engine->_animFrames[_guyFrameIndex];
	// DrawSprite(charX, charY, f.Width, f.Height, f.Data, s);
	// DrawSpriteAdvanced(charX, charY, f.Width, f.Height, 26, f.Data, s);
	/* for (int x = 0; x < g_engine->_charWidth; x++) {
		for (int y = 0; y < g_engine->_charHeight; y++) {
			uint8 val = g_engine->_charData[y * g_engine->_charWidth + x];
			if (val != 0) {
				s.setPixel(charX + x, charY + y, val);
			}
		}
	} */

	// Draw the border part
	/* uint16 borderX = 100;
	uint16 borderY = 50; 
	for (int x = 0; x < g_engine->_borderWidth; x++) {
		for (int y = 0; y < g_engine->_borderHeight; y++) {
			uint8 val = g_engine->_borderData[y * g_engine->_borderWidth + x];
			if (val != 0) {
				s.setPixel(borderX + x, borderY + y, val);
			}
		}
	} */

	// And the highlight part
	/* borderX = 150;
	borderY = 100;
	for (int x = 0; x < g_engine->_borderHighlightWidth; x++) {
		for (int y = 0; y < g_engine->_borderHighlightHeight; y++) {
			uint8 val = g_engine->_borderHighlightData[y * g_engine->_borderHighlightWidth + x];
			if (val != 0) {
				s.setPixel(borderX + x, borderY + y, val);
			}
		}
	}
	*/

	
	// DrawSprite(200, 100, g_engine->_flagWidths[1], g_engine->_flagHeights[1], g_engine->_flagData[1], s);
	// DrawSprite(200, 150, g_engine->_flagWidths[2], g_engine->_flagHeights[2], g_engine->_flagData[2], s);

	// Draw the mouse cursor
	// DrawSprite(100, 100, g_engine->_cursorWidth, g_engine->_cursorHeight, g_engine->_cursorData, s);

	// Draw the animation frame
	// DrawSprite(180, 80, g_engine->_guyWidth, g_engine->_guyHeight, g_engine->_guyData, s);
	
	
	//for (int i = 0; i < 100; ++i)
	//	s.frameRect(Common::Rect(i, i, 320 - i, 200 - i), i);

	// Draw a shaded rectangle
	// drawDarkRectangle(50, 50, 100, 50);
	// drawStringBackground(50, 50, 100, 50);
	if (_isShowingStringBox) {
		showStringBox(_drawnStringBox);
		if (speakingCharacter != nullptr) {
			// TODO: Improve addressing of the memory
			AnimFrame *frame = speakingCharacter->GetCurrentPortrait();

			DrawSprite(Common::Point(0xa, 0xa), frame->Width, frame->Height, frame->Data, s);
		}
	}

	// Draw all glyphs
	// drawGlyphs(g_engine->_glyphs, g_engine->numGlyphs, 10, 10, s);

	// DrawSprite(108, 14, g_engine->_flagWidths[_flagFrameIndex], g_engine->_flagHeights[_flagFrameIndex], g_engine->_flagData[_flagFrameIndex], s);
	drawBackgroundAnimations(s);
	// renderString(200, 100, "Hello, world!");

	// DrawSprite(100, 100, g_engine->_stick.Width, g_engine->_stick.Height, g_engine->_stick.Data, s);

	if (_isShowingInventory) {
		drawInventory(s);
	}

	if (activeInventoryItem != nullptr) {
		AnimFrame *icon = GetInventoryIcon(activeInventoryItem);
		DrawSprite(0x00, 0x00, icon->Width, icon->Height, icon->Data, s);
	}

	// drawPathfindingPoints(s);
	// drawPath(s);
	// drawBackgroundAnimationNumbers(s);
	DrawCharacters(s);
}

bool View1::tick() {
	// Cycle the palette
	++_offset;
	//for (int i = 0; i < 256; ++i)
	//	_pal[i * 3 + 1] = (i + _offset) % 256;
	// g_system->getPaletteManager()->setPalette(_pal, 0, 256);

	// Below is redundant since we're only cycling the palette, but it demonstrates
	// how to trigger the view to do further draws after the first time, since views
	// don't automatically keep redrawing unless you tell it to
	//if ((_offset % 256) == 0)
	//	redraw();

	// Update the flag
	// TODO: Think about all these and compare other implementations, e.g. if we should rather update anims in draw
	// TODO: Consider wraparout
	uint32 tick_time = g_events->currentMillis;
	uint32 delta = tick_time - _lastMillis;
	_nextFrameFlag -= delta;

	// TODO: Consider the case of frame skipping
	if (_nextFrameFlag <= 0) {
		_flagFrameIndex++;
		if (_flagFrameIndex == 3) {
			_flagFrameIndex = 0;
		}
		// TODO: Handle cleaner
		_nextFrameFlag = _frameDelayFlag;
		// TODO: Check if this is necessary

		// Proper update of the background anims
		// TODO: Hardcoding start to 2 to have the manually flipped animations not change automatically
		for (int i = 2; i < g_engine->_numBackgroundAnimations; i++) {
			BackgroundAnimation &current = g_engine->_backgroundAnimations[i];
			current.FrameIndex++;
			current.FrameIndex = current.FrameIndex % current.numFrames;
		}

		// TODO: Piggybacking the guy on this
		_guyFrameIndex++;
		_guyFrameIndex = _guyFrameIndex % 6;
		redraw();
	}

	_lastMillis = tick_time;

	int i = 0;
	for (auto currentCharacter : characters) {
		currentCharacter->Update();
		i++;
	}
	
	return true;
}

void View1::drawInventory(Graphics::ManagedSurface &s) {
	drawDarkRectangle(0x36, 0x2c, 0x10A - 0x36, 0x82 - 0x2c);
	for (GameObject *currentItem : inventoryItems) {
		AnimFrame *icon = GetInventoryIcon(currentItem);
		DrawSprite(0x36, 0x2c, icon->Width, icon->Height, icon->Data, s);

	}
}

void View1::DrawSprite(int16 x, int16 y, uint16 width, uint16 height, byte* data, Graphics::ManagedSurface& s)
{
	for (int currentX = 0; currentX < width; currentX++) {
		for (int currentY = 0; currentY < height; currentY++) {
			uint8 val = data[currentY * width + currentX];
			if (val != 0) {
				int finalX = x + currentX;
				int finalY = y + currentY;
				if (finalX >= 0 && finalX < s.w && finalY >= 0 && finalY < s.h) {
					s.setPixel(x + currentX, y + currentY, val);
				}
			}
		}
	}
}

void View1::DrawSprite(const Common::Point &pos, uint16 width, uint16 height, byte *data, Graphics::ManagedSurface &s) {
	DrawSprite(pos.x, pos.y, width, height, data, s);
}

void View1::DrawSpriteClipped(uint16 x, uint16 y, Common::Rect &clippingRect, uint16 width, uint16 height, byte *data, Graphics::ManagedSurface &s) {
	for (int currentX = 0; currentX < width; currentX++) {
		for (int currentY = 0; currentY < height; currentY++) {
			uint8 val = data[currentY * width + currentX];
			if (val != 0) {
				if (clippingRect.contains(x + currentX, y + currentY)) {
					s.setPixel(x + currentX, y + currentY, val);
				}
			}
		}
	}
}

void View1::DrawSpriteAdvanced(uint16 x, uint16 y, uint16 width, uint16 height, uint16 scaling, byte *data, Graphics::ManagedSurface &s) {
	int xScaling = 0;
	int yScaling = 0;

	int currentTargetX = 0;
	int currentTargetY = 0;

	for (int currentSourceY = 0; currentSourceY < height; currentSourceY++) {
		currentTargetX = 0;
		for (int currentSourceX = 0; currentSourceX < width; currentSourceX++) {
			uint8 val = data[currentSourceY * width + currentSourceX];
			if (val != 0) {
				uint16 finalX = x + currentTargetX;
				uint16 finalY = y + currentTargetY;
				if (finalX >= 0 && finalX < s.w && finalY >= 0 && finalY < s.h)
					s.setPixel(x + currentTargetX, y + currentTargetY, val);
			}
			xScaling += 0x64;
			currentTargetX++;
			do {
				// Handle x scaling
				if (xScaling <= scaling) {
					// This means we repeat a pixel
					currentSourceX--;
					break;
				}
				xScaling -= scaling;
				currentSourceX++;
			} while (currentSourceX < width);
		}
		yScaling += 0x64;
		currentTargetY++;
		do {
			// Handle y scaling
			if (yScaling <= scaling) {
				// This means we repeat a row
				currentSourceY--;
				break;
			}
			yScaling -= scaling;
			currentSourceY++;
		} while (currentSourceY < height);
	}


}

void View1::DrawCharacters(Graphics::ManagedSurface &s) {
	int i = -1;
	for (auto current : characters) {
		int index = current->GameObject->Index;
		// TODO: Object 50h is a special one, it is the invisible object that moves along the
		// ground during the stick throw. Need to check how this is handled it the game
		if (index == 0x50) {
			continue;
		}
		AnimFrame* frame = current->GetCurrentAnimationFrame();
		
		
		// AnimFrame *frame = current->GetCurrentPortrait();
		DrawSprite(current->Position - frame->GetBottomMiddleOffset(), frame->Width, frame->Height, frame->Data, s);
		// DrawSprite(Common::Point(50, 50), frame->Width, frame->Height, frame->Data, s);
	}
}

void View1::ShowSpeechAct(uint16_t characterIndex, const Common::Array<Common::String> &strings, const Common::Point &position, bool onRightSide) {
	// TODO: Handle setting up the updated drawing better
	speakingCharacter = GetCharacterByIndex(characterIndex);
	// TODO: Need to reset this
	// TODO: Add position: position.x, position.y,
	setStringBox(strings);
	_continueScriptAfterUI = true;

	if (autoclickActive) {
		clearStringBox();
	}
}

uint16_t View1::GetHitObjectID(const Common::Point& pos) const {
	// TODO: Naive implementation for now
	for (auto currentCharacter : characters) {
		auto animFrame = currentCharacter->GetCurrentAnimationFrame();

		// Saved point of the object is at the bottom in the middle, frame local space starts
		// at top left
		Common::Point localPoint = pos - (currentCharacter->Position - animFrame->GetBottomMiddleOffset());
		bool isHit = animFrame->PixelHit(localPoint);
		if (isHit) {
			return 0x0400 + currentCharacter->GameObject->Index;
		}
	}
	// TODO: Ignore background image lookup for now
	return 0x0000;
}

Macs2::AnimFrame *Character::GetCurrentAnimationFrame() {
	int blobIndex = 0x2;
	// int offset = 0x1C;

	// TODO: Need to figure out the access pattern more systematically
	if (GameObject->Index == 0x8) {
		blobIndex = 4;
	//	offset = 23;
	} else if (GameObject->Index == 0x0a) {
		// TODO: Figure out how we find these
		blobIndex = 6;
	}
	Common::MemoryReadStream stream(this->GameObject->Blobs[blobIndex].data(), this->GameObject->Blobs[blobIndex].size());
	stream.seek(0xA, SEEK_SET);
	uint16_t offset = stream.readUint16LE();
	offset += 0x8;
	stream.seek(offset, SEEK_CUR);

	AnimFrame* result = new AnimFrame();
	
	// TODO: Need to check how the offset really is calculated by the game code
	result->ReadFromStream(&stream);
	return result;
	// TODO: Think about proper memory management
}

Macs2::AnimFrame *Character::GetCurrentPortrait() {
	AnimFrame *result = new AnimFrame();
	Common::MemoryReadStream stream(this->GameObject->Blobs[17].data(), this->GameObject->Blobs[17].size());
	// TODO: Need to check how the offset really is calculated by the game code, this will not hold
	if (GameObject->Index == 2 || GameObject->Index == 6) {
		stream.seek(35, SEEK_SET);
	} else {
		stream.seek(36, SEEK_SET);
	}
	
	result->ReadFromStream(&stream);
	return result;
	// TODO: Think about proper memory management
}

void Character::StartLerpTo(const Common::Point &target, uint32 duration) {
	StartPosition = Position;
	EndPosition = target;
	StartTime = g_events->currentMillis;
	Duration = duration;
	IsLerping = true;
}

void Character::StartPickup(Character *object) {
	objectToPickUp = object;
	ExecuteScriptOnFinishLerp = true;
	StartLerpTo(objectToPickUp->Position, 1000);
}

void Character::RegisterWaitForMovementFinishedEvent() {

	// For now, we are treating this one as a flag to send an event
	// even if we are not lerping, so that we have a delay between action 0x11
	// and the new execution
	ExecuteScriptOnFinishLerp = true;
	
}

void Character::Update() {
	if (!IsLerping) {
		// We might have gotten the 0x11 command after we stopped moving
		// TODO: Check if the code handles this similarly
		// TODO: Consider which run function to use
		if (ExecuteScriptOnFinishLerp) {
			ExecuteScriptOnFinishLerp = false;
			g_engine->ScheduleRun();
		}
		return;
	}
	uint32 endTime = StartTime + Duration;
	bool isDone = endTime < g_events->currentMillis;
	
	if (isDone) {
		IsLerping = false;

		// Check if we need to pick something up
		if (objectToPickUp != nullptr) {

			View1 *currentView = (View1 *)g_engine->findView("View1");
			int index = currentView->GetCharacterArrayIndex(objectToPickUp);
			currentView->inventoryItems.push_back(objectToPickUp->GameObject);
			currentView->characters.remove_at(index);
			objectToPickUp = nullptr;
		}


		// Check if we need to execute the script
		// TODO: Consider which run function to use
		if (ExecuteScriptOnFinishLerp) {
			ExecuteScriptOnFinishLerp = false;
			g_engine->ScheduleRun();
		}
		return;
	}

	float progress = (float) (g_events->currentMillis - StartTime) / (float) Duration;
	Position = StartPosition + (EndPosition - StartPosition) * progress;
}

bool Button::IsPointInside(const Common::Point &p) const {
	return false;
}

void Button::Render(Graphics::ManagedSurface &s) {
}

} // namespace Macs2
