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

#include "macs2/macs2.h"
#include "macs2/detection.h"
#include "macs2/console.h"
#include "common/scummsys.h"
#include "common/config-manager.h"
#include "common/debug.h"
#include "common/debug-channels.h"
#include "common/events.h"
#include "common/system.h"
#include "engines/util.h"
#include "graphics/palette.h"
#include "graphics/surface.h"
#include "graphics/pixelformat.h"

namespace Macs2 {

Macs2Engine *g_engine;

Graphics::ManagedSurface Macs2Engine::readRLEImage(int64 offs, Common::File& file)
{
	file.seek(offs);

	Graphics::ManagedSurface result = new Graphics::Surface();
	result.create(320, 200, Graphics::PixelFormat::createFormatCLUT8());
	

	// TODO: Fix length
	uint8* data = new uint8[1024];

	// TODO: Consider if it can be that the data is more than this. Maybe the tooling of the engine can make bad calls and
	// try to RLE something which would be better not RLE encoded.

	// TODO: Fix start position
	for (int y = 0; y < 200; y++) {
		uint16 length = file.readUint16LE();
		file.read(data, length);
		uint16 remainingPixels = 320;
		uint8* dataPointer = data;
		uint16 x = 0;
		while (remainingPixels > 0) {
			const uint8& value = dataPointer[0];
			dataPointer++;
			if (value != 0xF0) {
				/* if (x >= 320) {
					// TODO: This happens during loading the map
					break;
				}*/
				result.setPixel(x, y, value);
				remainingPixels--;
				x++;
			}
			else {
				// We need to decode the RLE data
				const uint8& runlength = dataPointer[0];
				dataPointer++;
				const uint8& encodedValue = dataPointer[0];
				dataPointer++;
				for (int i = 0; i < runlength; i++) {
					/* if (x >= 320) {
						// TODO: This happens for loading the map - maybe this code is a bit different?
						break;
					} */
					result.setPixel(x++, y, encodedValue);
				}
				remainingPixels -= runlength;
			}
		}
	}

	return result;
}



void Macs2Engine::readResourceFile() {
	Common::File file;
	if (!file.open("RESOURCE.MCS"))
		error("readResourceFile(): Error reading MCS file");

	file.seek(0x23BE09);


	// file.read(data, 320 * 240);

	// uint8 b0 = data[0];

	_bgImageShip.create(320, 200, Graphics::PixelFormat::createFormatCLUT8());

	uint8* lengthData = new uint8[2];
	uint8* data = new uint8[320];

	// TODO: Consider if it can be that the data is more than this. Maybe the tooling of the engine can make bad calls and
	// try to RLE something which would be better not RLE encoded.

	for (int y = 0; y < 200; y++) {
		// TODO: Use the proper read function, it seems to be available
		file.read(lengthData, 2);
		uint16 length = lengthData[1] << 8 | lengthData[0];
		file.read(data, length);
		uint16 remainingPixels = 320;
		uint8* dataPointer = data;
		uint16 x = 0;
		while (remainingPixels > 0) {
			const uint8& value = dataPointer[0];
			dataPointer++;
			if (value != 0xF0) {
				_bgImageShip.setPixel(x, y, value);
				remainingPixels--;
				x++;
			}
			else {
				// We need to decode the RLE data
				const uint8& runlength = dataPointer[0];
				dataPointer++;
				const uint8& encodedValue = dataPointer[0];
				dataPointer++;
				for (int i = 0; i < runlength; i++) {
					_bgImageShip.setPixel(x++, y, encodedValue);
				}
				remainingPixels -= runlength;
			}
		}
	}

	// Load the palette
	file.seek(0x00248BCB);
	file.read(_pal, 256 * 3);

	// Adjust the palette
	for (int i = 0; i < 256 * 3; i++) {
		_pal[i] = (_pal[i] * 259 + 33) >> 6;
	}

	// return check_cast<uint8_t>((c * 259 + 33) >> 6);

	// Iterate over 0 to 199 for each row
	// Load the amount of bytes for the row
	// Load the row data and do run-length decoding
	// Save into the right row of the surface


	// Load the data for a character

	file.seek(0x6FB2);
	_charASCII = file.readByte();
	_charWidth = file.readUint16LE();
	_charHeight = file.readUint16LE();
	_charData = new byte[_charWidth * _charHeight];
	file.read(_charData, _charWidth * _charHeight);

	file.seek(0x00006DFE);
	// Load more characters
	for (int i = 0; i < numGlyphs; i++) {
		_glyphs[i].ReadFromeFile(file);
	}

	// Load the data for a border part
	file.seek(0x64C6);

	_borderWidth = file.readUint16LE();
	_borderHeight = file.readUint16LE();
	_borderData = new byte[_borderWidth * _borderHeight];
	file.read(_borderData, _borderWidth * _borderHeight);

	// And the highlight part
	file.seek(0x6962);

	_borderHighlightWidth = file.readUint16LE();
	_borderHighlightHeight = file.readUint16LE();
	_borderHighlightData = new byte[_borderHighlightWidth * _borderHighlightHeight];
	file.read(_borderHighlightData, _borderHighlightWidth * _borderHighlightHeight);

	// The flag animation frames
	file.seek(0x00250D47);
	_flagData = new byte * [3];
	_flagWidths = new uint16[3];
	_flagHeights = new uint16[3];
	_flagWidths[0] = file.readUint16LE();
	_flagHeights[0] = file.readUint16LE();
	_flagData[0] = new byte[_flagWidths[0] * _flagHeights[0]];
	file.read(_flagData[0], _flagWidths[0] * _flagHeights[0]);

	file.seek(0x00250B3C);
	_flagWidths[1] = file.readUint16LE();
	_flagHeights[1] = file.readUint16LE();
	_flagData[1] = new byte[_flagWidths[1] * _flagHeights[1]];
	file.read(_flagData[1], _flagWidths[1] * _flagHeights[1]);

	file.seek(0x00250931);
	_flagWidths[2] = file.readUint16LE();
	_flagHeights[2] = file.readUint16LE();
	_flagData[2] = new byte[_flagWidths[2] * _flagHeights[2]];
	file.read(_flagData[2], _flagWidths[2] * _flagHeights[2]);

	// Load the script
	file.seek(0x000A3B98);
	uint16 scriptLength = file.readUint16LE();
	_scriptData = new byte[scriptLength];
	file.read(_scriptData, scriptLength);
	_scriptStream = new Common::MemoryReadStream(_scriptData, scriptLength);

	// Try executing the script
	ExecuteScript(_scriptStream);

	// Load the background map
	// _map = readRLEImage(0x0024BD9B, file);
	// _map = readRLEImage(0x00248FCE, file);
	_map = readRLEImage(0x0024B0DF, file);

	// Load the data for the mouse cursor

	// TODO: Figure out which is the correct one
	// file.seek(0x00003C74);
	file.seek(0x0000524A);
	_cursorWidth = file.readUint16LE();
	_cursorHeight = file.readUint16LE();
	_cursorData = new byte[_cursorWidth * _cursorHeight];
	file.read(_cursorData, _cursorWidth* _cursorHeight);

	// Load a frame of animation from the protagonist
	// Crawling towards the left
	// file.seek(0x000C95A8);
	// Standing towards the right
	// file.seek(0x001CC5D9);
	// Also standing towards the right
	file.seek(0x000D0B26);
	

	_guyWidth = file.readUint16LE();
	_guyHeight = file.readUint16LE();
	_guyData = new byte[_guyWidth * _guyHeight];
	file.read(_guyData, _guyWidth * _guyHeight);

	// Load the shading table
	file.seek(0x00248ECB);
	_shadingTable = new byte[256];
	file.read(_shadingTable, 256);

}

Macs2Engine::Macs2Engine(OSystem *syst, const ADGameDescription *gameDesc) : Engine(syst),
	_gameDescription(gameDesc), _randomSource("Macs2") {
	g_engine = this;
}

Macs2Engine::~Macs2Engine() {


}

bool Macs2Engine::FindGlyph(char c, GlyphData& out) const {
	return false;
}

// Does what 9F23 does
uint16 ScriptReadWord(Common::MemoryReadStream* stream) {
	const int64 pos = stream->pos();
	const uint16 result = stream->readUint16LE();
	debug("Script read (word): %.4x at offset %.4x\n", result, pos);
	return result;
}

// Does pretty much what 9F07 does
byte ScriptReadByte(Common::MemoryReadStream* stream) {

	const int64 pos = stream->pos();
	const byte result = stream->readByte();
	debug("Script read (byte): %.2x at offset %.4x\n", result, pos);
	return result;
}

// #define ScriptNoEntry assert(false);
#define ScriptNoEntry debug("Unhandled case in script handling");

void Func9F4DClean(Common::MemoryReadStream *stream, uint16 &out1, uint16 &out2) {
	// TODO: Implement the actual prelude here correctly, documenting which lables we pass as we go
	debug("-- Entering 9F4D");

	debug("-- Leaving 94FD");
}

void Func9F4D(Common::MemoryReadStream * stream, uint16& out1, uint16& out2) {
	debug("-- Entering 9F4D");
	// Read an opcode (would be 0037:9F07) - [bp-1h]
	byte opcode = ScriptReadByte(stream);
	// debug("Script read (byte): %.2x at offset %.4x\n", opcode, stream->pos());

	uint16 value = ScriptReadWord(stream);

	// TODO: Add the "prelude" before the big switch here, since the logic is more general than what I would implement if I took it case by case

	// debug("Script read (word): %.4x at offset %.4x\n", value, stream->pos());
	// TODO: There is some code required here to follow the logic exactly of going from opcode 1 to 2
	if (opcode == 0x01) {
		if (value == 0x0b) {
			// TODO: Hardcoded combination, the rule is more general
			out1 = 0x03;
			out2 = 0x00;
			debug("-- Leaving 94FD");
			return;
		}
	} else
	if (opcode == 0x02) {
		// TODO: We need to start handling opcode2 in this case
		if (value == 0x0a) {
			// TODO: This is too verbatim, only to get me to pass the script once. In reality, we are accessing a saved variable in this and similar cases based on the second value
			out1 = 0;
			out2 = 0;
			debug("-- Leaving 94FD");
			return;
		}
	}
	else
	if (opcode == 0xFF) {
		// TODO: Long list of opcode handling here
		if (value == 0x26) {
			// TODO: Implement this part:
	/* l0037_A19E:
		cmp	byte ptr[1014h], 0h
			jz	0A1B1h

			l0037_A1A5 :
		mov	word ptr[bp - 4h], 1h
			mov	word ptr[bp - 2h], 0h
			jmp	0A1B9h

			l0037_A1B1 :
		xor ax, ax
			mov[bp - 4h], ax
			mov[bp - 2h], ax */

			// For now just returning 0 by default
			out1 = 0;
			out2 = 0;
			debug("-- Leaving 94FD");
			return;
		}
	}
	else {
		ScriptNoEntry
	}
}



void FuncA3D2(Common::MemoryReadStream* stream) {
	debug("-- Entering A3D2");
	uint16 skipValue = 1; // [bp-4h] - TODO: Better name
	// TODO: Figure out end condition
	for (;;) {
		const byte opcode = ScriptReadByte(stream);
		const byte val = ScriptReadByte(stream);
		if (opcode >= 3) {
			if (opcode <= 6) {
				skipValue++;
			}
		}
		if (opcode == 8) {
			if (skipValue == 1) {
				skipValue--;
			}
		}
		if (opcode == 7) {
			skipValue--;
		}
		// Do the skipping
		stream->seek(val, SEEK_CUR);
		debug("A3D2 skipping %u bytes for opcode %.2x (%u)", val, opcode, skipValue);
			
		// TODO: Add a log here
		if (skipValue != 0) {
			// Continue the loop if there is data left in the stream
			// TODO: Check for remaining script data
		}
		else {
			if (skipValue != 0) {
				// TODO: Implement:
				// mov	word ptr [1028h],1Dh
				// TODO: Add an assert here to see if this ever happens in practice
			}
			break;
		}
		// TODO: Continue here
	}
	debug("-- Leaving A3D2");
}




void Macs2Engine::ExecuteScript(Common::MemoryReadStream* stream) {
	// Implements roughly 01E7:DB56 and friends
	// TODO: Change to a proper end condition
	// TODO: Do some bookkeeping on the pointers into the script
	for (;;) {
		

		// l0037_DB89:


		// Read an opcode (would be 0037:9F07) - [bp-1h]
		byte opcode1 = ScriptReadByte(stream);

		// Read another value - TODO: Not sure yet what this does - [bp-2h]
		byte val1 = ScriptReadByte(stream);

		// TODO: Handle other opcodes above
		if (opcode1 == 0x01) {
			ScriptReadByte(stream);
			ScriptReadWord(stream);
			uint16 result1;
			uint16 result2;

			Func9F4D(stream, result1, result2);
			// TODO: Implement properly - this looks like some kind of bookkeeping since it doesn't determine if we continue or not?
				/*
				call	far 0037h:9F07h
				call	far 0037h : 9F23h
				mov[bp - 11h], ax
				call	far 0037h : 9F4Dh
				mov	cx, ax
				mov	bx, dx
				mov	ax, [bp - 11h]
				shl	ax, 2h
				les	di, [06C6h]
				add	di, ax
				mov	es : [di - 4h] , cx
				mov	es : [di - 2h] , bx
				// This jumps ahead to the end of the loop
				jmp	0E3BAh
				*/
		}
		else if (opcode1 == 0x02 || opcode1 == 0x03) {
			// TODO: Implement
			ScriptNoEntry
		} else
		if (opcode1 == 0x04) {
			// l0037_DC44:
			uint16 v1;
			uint16 v2;
			Func9F4D(stream, v1, v2);
			if ((v1 | v2) == 0) {
				FuncA3D2(stream);
			}
			else {
				// TODO: Implement
				ScriptNoEntry
			}
		}
		else if (opcode1 == 0x05) {
			// TODO: Implement this second opcode fetching:
			// [bp-3h]
			byte opcode2 = ScriptReadByte(stream);

			// [bp-7h]
			uint16 v1;
			// [bp-5h]
			uint16 v2;
			Func9F4D(stream, v1, v2);
			// [bp-0Bh]
			uint16 v3;
			// [bp-9h]
			uint16 v4;
			Func9F4D(stream, v3, v4);
			// TODO: Not yet implemented:
			// mov	byte ptr [bp-12h],0h

			/*
				call	far 0037h:9F07h
	mov	[bp-3h],al
	;; TODO: I just now realized this one - why do we call the same function two times? And does
	;; the result change in between?
	call	far 0037h:9F4Dh
	;; This is the result of the function that mapped the action to a hotspot (0037:9F4D)
	mov	[bp-7h],ax
	mov	[bp-5h],dx
	;; Note: I don't think (TODO: Confirm) that the locals here are accessed in this function
	call	far 0037h:9F4Dh
	mov	[bp-0Bh],ax
	mov	[bp-9h],dx
	
	mov	al,[bp-3h]
	cmp	al,1h
	jnz	0DCA6h

			*/
		}
		// This is where handling of the opcodes > 6 continues
		// TODO: Does it really? To check if I got this right
		// l0037_DD3C
		else if (opcode1 == 0x06) {
			ScriptNoEntry
		}
		else if (opcode1 == 0x07) {
			ScriptNoEntry
		}
		else {
			ScriptNoEntry
			break;
		}

		// TODO: Handle other opcodes below
		

		
	}
}

uint32 Macs2Engine::getFeatures() const {
	return _gameDescription->flags;
}

Common::String Macs2Engine::getGameId() const {
	return _gameDescription->gameId;
}

Common::Error Macs2Engine::run() {

	readResourceFile();

	// Initialize 320x200 paletted graphics mode
	initGraphics(320, 200);

	

	// Set the engine's debugger console
	setDebugger(new Console());

	runGame();

	return Common::kNoError;
}

Common::Error Macs2Engine::syncGame(Common::Serializer &s) {
	// The Serializer has methods isLoading() and isSaving()
	// if you need to specific steps; for example setting
	// an array size after reading it's length, whereas
	// for saving it would write the existing array's length
	int dummy = 0;
	s.syncAsUint32LE(dummy);

	return Common::kNoError;
}

void GlyphData::ReadFromeFile(Common::File &file) {
	// TODO: Implement
	int64 stride = file.pos();
	ASCII = file.readByte();
	Width = file.readUint16LE();
	Height = file.readUint16LE();
	Data = new byte[Width * Height];
	file.read(Data, Width * Height);
	stride = file.pos() - stride;
}

} // End of namespace Macs2
