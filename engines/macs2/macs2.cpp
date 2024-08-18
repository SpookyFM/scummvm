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
#include "common/util.h"
#include "engines/util.h"
#include "graphics/palette.h"
#include "graphics/surface.h"
#include "graphics/pixelformat.h"
#include "audio/fmopl.h"
#include "view1.h"
#include "adlib.h"
#include "gameobjects.h"

namespace Macs2 {

Macs2Engine *g_engine;

Graphics::ManagedSurface Macs2Engine::readRLEImage(int64 offs, Common::MemoryReadStream *stream) {
	// TODO: Should we pass the stream as pointer or reference?
	stream->seek(offs);

	Graphics::ManagedSurface result = new Graphics::Surface();
	result.create(320, 200, Graphics::PixelFormat::createFormatCLUT8());
	

	// TODO: Fix length
	uint8* data = new uint8[1024];

	// TODO: Consider if it can be that the data is more than this. Maybe the tooling of the engine can make bad calls and
	// try to RLE something which would be better not RLE encoded.

	// TODO: Fix start position
	for (int y = 0; y < 200; y++) {
		uint16 length = stream->readUint16LE();
		stream->read(data, length);
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

int previewNumFrames(int64 offs, Common::File& file) {
	file.seek(offs);
	int64 numBytes = file.readUint32LE();
	// byte* data = new byte[numBytes];
	// file.read(data, numBytes);
	// TODO: Extract the frames
	// Skip ahead to the start of the first frame
	file.seek(20, SEEK_CUR);
	numBytes -= 20;
	int numFrames = 0;
	while (numBytes > 0) {
		file.seek(6, SEEK_CUR);
		numBytes -= 6;
		uint16_t x = file.readUint16LE();
		uint16_t y = file.readUint16LE();
		file.seek(x * y, SEEK_CUR);
		numBytes -= 4 + 4 + x * y;
		numFrames++;
	}
	file.seek(offs);
	return numFrames;
}


void Macs2Engine::readResourceFile() {
	{
		// Extra scope in order to make sure no code tries to read from the file directly.
		Common::File file;
		if (!file.open("RESOURCE.MCS"))
			error("readResourceFile(): Error reading MCS file");

		int64 size = file.size();
		byte *fileData = new byte[size];
		file.read(fileData, size);

		_fileStream = new Common::MemoryReadStream(fileData, size);
	}
	
	// Full implementation here

	// We need to skip reading the 776h global which comes first
	_fileStream->seek(0xC + 0x2, SEEK_SET);

	uint16_t firstSceneIndex = _fileStream->readUint16LE();
	Scenes::instance().CurrentSceneIndex = firstSceneIndex;
	Scenes::instance().CurrentSceneScript = Scenes::instance().ReadSceneScript(firstSceneIndex, _fileStream);
	Scenes::instance().CurrentSceneStrings = Scenes::instance().ReadSceneStrings(firstSceneIndex, _fileStream);
	Scenes::instance().CurrentSceneSpecialAnimOffsets = Scenes::instance().ReadSpecialAnimsOffsets(firstSceneIndex, _fileStream);
	_scriptExecutor->SetScript(Scenes::instance().CurrentSceneScript);

	// for (int i = 1; i < 0x200; i++) {
	// TODO: Figure out what happens here
	for (int i = 1; i < 0x100; i++) {
		// The formula for the seek lives at l0037_0936
		// The global [0752h] is loaded with 3000h bytes read from offset Ch + 4h in the file
		// Regarding the 4h offset: Before the 3000h bytes, we have the values of the two globals 0776 and 077C
		uint32 addressOffset = 0x17F4 + (0xC + 0x04) + i * 0xC;
		_fileStream->seek(addressOffset, SEEK_SET);
		uint32 objectOffset = _fileStream->readUint32LE();
		if (objectOffset == 0) {
			break;
		}

		_fileStream->seek(objectOffset, SEEK_SET);
		GameObject *gameObject = new GameObject();
		gameObject->Index = i;

		// This loading happens around the l0037_082D: mark
		uint16_t x = _fileStream->readUint16LE();
		uint16_t y = _fileStream->readUint16LE();
		uint16_t sceneIndex = _fileStream->readUint16LE();
		uint16_t orientation = _fileStream->readUint16LE();
		uint16_t unknown = _fileStream->readUint16LE();
		gameObject->Position = Common::Point(x, y);
		gameObject->SceneIndex = sceneIndex;
		gameObject->Orientation = orientation;
		gameObject->Unknown = unknown;

		for (int j = 1; j < 0x15; j++) {
			// We're at l0037_0A3E here
			// TODO: Compare places and read values with the game
			uint16_t unknown1 = _fileStream->readUint16LE();
			uint16_t unknown2 = _fileStream->readUint16LE();
			uint32_t dataSize = _fileStream->readUint32LE();
			uint8_t *data = new uint8_t[dataSize];
			_fileStream->read(data, dataSize);
			// TODO: Place this data in the game object and create the game object
			gameObject->Blobs.push_back(Common::Array<uint8_t>(data, dataSize));
			// Seek forward for the next 2+1+1 bytes reads
			_fileStream->seek(0x4, SEEK_CUR);
		}
		// Read the object script
		// The offset is calculated at l0037_0C9D - also see above for adjustmnets
		addressOffset = 0x17F8 + (0xC + 0x04) + i * 0xC;
		_fileStream->seek(addressOffset, SEEK_SET);

		objectOffset = _fileStream->readUint32LE();
		// TODO: Not sure if this can happen or should be checked
		if (objectOffset == 0) {
			break;
		}
		_fileStream->seek(objectOffset, SEEK_SET);
		// TODO: We read 80h bytes - to check where these are used - script variables?
		_fileStream->seek(0x80, SEEK_CUR);
		uint16_t scriptLength = _fileStream->readUint16LE();
		gameObject->Script.resize(scriptLength);
		_fileStream->read(gameObject->Script.data(), scriptLength);

		GameObjects::instance().Objects.push_back(gameObject);
	}


	// Test implementations below



	_fileStream->seek(0x23BE09);


	// file.read(data, 320 * 240);

	// uint8 b0 = data[0];

	_bgImageShip.create(320, 200, Graphics::PixelFormat::createFormatCLUT8());

	uint8* lengthData = new uint8[2];
	uint8* data = new uint8[320];

	// TODO: Consider if it can be that the data is more than this. Maybe the tooling of the engine can make bad calls and
	// try to RLE something which would be better not RLE encoded.

	for (int y = 0; y < 200; y++) {
		// TODO: Use the proper read function, it seems to be available
		_fileStream->read(lengthData, 2);
		uint16 length = lengthData[1] << 8 | lengthData[0];
		_fileStream->read(data, length);
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
	_fileStream->seek(0x00248BCB);
	_fileStream->read(_pal, 256 * 3);
	// Make a copy that will not be color corrected, for fading
	memcpy(_palVanilla, _pal, 256 * 3);

	// Adjust the palette
	for (int i = 0; i < 256 * 3; i++) {
		_pal[i] = (_pal[i] * 259 + 33) >> 6;
	}

	// Load the pathfinding points
	// TODO: Figure out how the game knows the length of this data
	_fileStream->seek(0x0024BF72);
	// TODO: Obviously not nice to assume a certain endianness, need to read values invividually
	for (int i = 0; i < 16; i++) {
		_pathfindingPoints[i*2] = _fileStream->readUint16LE();
		_pathfindingPoints[i*2 + 1] = _fileStream->readUint16LE();
		// Need to read 6 more bytes of unknown purpose
		// TODO: Add them when I know what they do
		for (int j = 0; j < 3; j++) {
			_fileStream->readUint16LE();
		}
	}
	

	

	// return check_cast<uint8_t>((c * 259 + 33) >> 6);

	// Iterate over 0 to 199 for each row
	// Load the amount of bytes for the row
	// Load the row data and do run-length decoding
	// Save into the right row of the surface


	// Load the data for a character

	_fileStream->seek(0x6FB2);
	_charASCII = _fileStream->readByte();
	_charWidth = _fileStream->readUint16LE();
	_charHeight = _fileStream->readUint16LE();
	_charData = new byte[_charWidth * _charHeight];
	_fileStream->read(_charData, _charWidth * _charHeight);

	_fileStream->seek(0x00006DFE);
	// Load more characters
	for (int i = 0; i < numGlyphs; i++) {
		_glyphs[i].ReadFromMemory(_fileStream);
	}

	// Load the animation frames
	// TODO: Figure out how the game knows how many there are
	// TODO: Figure out why the frames are not saved sequentially
	// _fileStream->seek(0x0009619E);
	_fileStream->seek(0x006A5941);
	for (int i = 0; i < 6; i++) {
		_animFrames[i].ReadFromStream(_fileStream);
		// There are 6 empty bytes until the next one
		_fileStream->seek(6, SEEK_CUR);
	}

	// Load the data for a border part
	// _fileStream->seek(0x64C6);
	_fileStream->seek(0x0000602A);

	_borderWidth = _fileStream->readUint16LE();
	_borderHeight = _fileStream->readUint16LE();
	_borderData = new byte[_borderWidth * _borderHeight];
	_fileStream->read(_borderData, _borderWidth * _borderHeight);

	// And the highlight part
	_fileStream->seek(0x6962);
	

	_borderHighlightWidth = _fileStream->readUint16LE();
	_borderHighlightHeight = _fileStream->readUint16LE();
	_borderHighlightData = new byte[_borderHighlightWidth * _borderHighlightHeight];
	_fileStream->read(_borderHighlightData, _borderHighlightWidth * _borderHighlightHeight);

	// The flag animation frames
	_fileStream->seek(0x00250D47);
	_flagData = new byte * [3];
	_flagWidths = new uint16[3];
	_flagHeights = new uint16[3];
	_flagWidths[0] = _fileStream->readUint16LE();
	_flagHeights[0] = _fileStream->readUint16LE();
	_flagData[0] = new byte[_flagWidths[0] * _flagHeights[0]];
	_fileStream->read(_flagData[0], _flagWidths[0] * _flagHeights[0]);

	_fileStream->seek(0x00250B3C);
	_flagWidths[1] = _fileStream->readUint16LE();
	_flagHeights[1] = _fileStream->readUint16LE();
	_flagData[1] = new byte[_flagWidths[1] * _flagHeights[1]];
	_fileStream->read(_flagData[1], _flagWidths[1] * _flagHeights[1]);

	_fileStream->seek(0x00250931);
	_flagWidths[2] = _fileStream->readUint16LE();
	_flagHeights[2] = _fileStream->readUint16LE();
	_flagData[2] = new byte[_flagWidths[2] * _flagHeights[2]];
	_fileStream->read(_flagData[2], _flagWidths[2] * _flagHeights[2]);


	// Load the strings for the scene
	_fileStream->seek(0x000D2F22);
	numBytesStrings = _fileStream->readUint16LE();
	stringsData = new byte[numBytesStrings];
	_fileStream->read(stringsData, numBytesStrings);
	// _stringsStream = new Common::MemoryReadStream(stringsData, numBytesStrings);

	// Load the background map
	// _map = readRLEImage(0x0024BD9B, file);
	// _map = readRLEImage(0x00248FCE, file);
	// Next on is the actual map
	_map = readRLEImage(0x0024B0DF, _fileStream);
	// TODO: This is the depth map - TBC that it's actually it
	_depthMap = readRLEImage(0x00248FCE, _fileStream);

	// This is the walkability map - TBC if that's really it and how it works
	_pathfindingMap = readRLEImage(0x00249CC1, _fileStream);

	
	

	// Load the data for the mouse cursor

	// TODO: Figure out which is the correct one
	// _fileStream->seek(0x00003C74);
	// _fileStream->seek(0x0000524A);
	_fileStream->seek(0x000050EC);
	constexpr int numCursors = 5;
	_cursorData = new byte *[numCursors];
	_cursorWidths = new uint16[numCursors];
	_cursorHeights = new uint16[numCursors];
	for (int i = 0; i < numCursors; i++) {
		_cursorWidths[i] = _fileStream->readUint16LE();
		_cursorHeights[i] = _fileStream->readUint16LE();
		_cursorData[i] = new byte[_cursorWidths[i] * _cursorHeights[i]];
		_fileStream->read(_cursorData[i], _cursorWidths[i] * _cursorHeights[i]);
		// Seek forward to skip an entry
		// TODO: Figure out what is skipped there
		_fileStream->seek(0x6, SEEK_CUR);
	}


	// Load a frame of animation from the protagonist
	// Crawling towards the left
	// _fileStream->seek(0x000C95A8);
	// Standing towards the right
	// _fileStream->seek(0x001CC5D9);
	// Also standing towards the right
	// _fileStream->seek(0x000D0B26);

	_fileStream->seek(0x006AB0FD);
	

	_guyWidth = _fileStream->readUint16LE();
	_guyHeight = _fileStream->readUint16LE();
	_guyData = new byte[_guyWidth * _guyHeight];
	_fileStream->read(_guyData, _guyWidth * _guyHeight);

	// Load the shading table
	_fileStream->seek(0x00248ECB);
	_shadingTable = new byte[256];
	_fileStream->read(_shadingTable, 256);

	
	// Load the objects data
	_fileStream->seek(0x6a5913);

	// Load the stick
	_fileStream->seek(0x00708410);
	_stick.ReadFromStream(_fileStream);

	_fileStream->seek(0x000d4fbd);
	// TODO: Figure out where the number comes from
	byte *adlibData = new byte[15610];
	_fileStream->read(adlibData, 15610);
	_adlib->data = new Common::MemoryReadStream(adlibData, 15610);
}

void Macs2Engine::ReadBackgroundAnimations(Common::MemoryReadStream *stream) {
	// Offset 50F5 in scene data
	// TODO: Remove the non-blob implementation
	_numBackgroundAnimations = stream->readUint16LE();

	_backgroundAnimations = new BackgroundAnimation[_numBackgroundAnimations];
	_backgroundAnimationsBlobs.resize(_numBackgroundAnimations);

	for (int i = 0; i < _numBackgroundAnimations; i++) {
		BackgroundAnimationBlob &currentBlob = _backgroundAnimationsBlobs[i];
		
		BackgroundAnimation &current = _backgroundAnimations[i];
		// Local offset +0h
		current.X = stream->readUint16LE();
		currentBlob.X = current.X;
		// Local offset +2n
		current.Y = stream->readUint16LE();
		currentBlob.Y = current.Y;

		// current.numFrames = previewNumFrames(file.pos(), file);
		uint32 numBytes = stream->readUint32LE();

		currentBlob.Blob.resize(numBytes);
		int64 pos = stream->pos();
		stream->read(currentBlob.Blob.data(), numBytes);
		stream->seek(pos, SEEK_SET);

		// Skip to the intermediary data
		// Game loading code puts this at a pointer stored in local offset +8h
		stream->seek(10, SEEK_CUR);
		uint16 nextNumBytes = stream->readUint16LE();
		stream->seek(nextNumBytes, SEEK_CUR);
		current.numFrames = stream->readUint16LE();
		current.FrameIndex = 0;
		current.Frames = new AnimFrame[current.numFrames];
		for (int j = 0; j < current.numFrames; j++) {
			// Skip to width and height
			stream->seek(6, SEEK_CUR);
			current.Frames[j].ReadFromStream(stream);
		}
		// TODO: This allows us to skip over a faulty implementation, but
		// probably missing some valid data or loading wrong data
		// file.seek(endPos);
		// TODO: Figure out the trailing values?
		// Local offset +Ch
		uint16 unknown1 = stream->readUint16LE();
		// Local offset +Eh
		uint16 unknown2 = stream->readByte();
		// Local offset +Fh
		uint16 unknown3 = stream->readByte();
	}
}

Macs2Engine::Macs2Engine(OSystem *syst, const ADGameDescription *gameDesc) : Engine(syst),
	_gameDescription(gameDesc), _randomSource("Macs2") {
	g_engine = this;
	_scriptExecutor = new Script::ScriptExecutor();
	_scriptExecutor->_engine = this;
	_adlib = new Adlib();

	DebugMan.addDebugChannel(DEBUG_RLE, "rle", "Verbose RLE decoding log");
	DebugMan.addDebugChannel(DEBUG_SV, "sv", "Verbose script debugging log");
}

Macs2Engine::~Macs2Engine() {
	_adlib->Deinit(); 

}

void Macs2Engine::changeScene(uint32_t newSceneIndex, bool executeScript) {
	// TODO: Release old resources

	// Background image
	// [0752h] is pointing to 3000h bytes data starting at Ch + 4h in the file
	// Addressing the background image starts at l0037_25A9
	_fileStream->seek(0xC + 0x4 + 0xC * newSceneIndex - 0xC, SEEK_SET);
	uint32_t bgImageOffset = _fileStream->readUint32LE();
	_fileStream->seek(bgImageOffset, SEEK_SET);

	// TODO: Copy-pasted code here
	// _bgImageShip.create(320, 200, Graphics::PixelFormat::createFormatCLUT8());

	uint8 *data = new uint8[0x320];

	// TODO: Consider if it can be that the data is more than this. Maybe the tooling of the engine can make bad calls and
	// try to RLE something which would be better not RLE encoded.

	for (int y = 0; y < 200; y++) {
		// TODO: Use the proper read function, it seems to be available
		uint16 length = _fileStream->readUint16LE();
		_fileStream->read(data, length);
		debugC(DEBUG_RLE, "RLE: Row %.4x with %.4x bytes of data.", y, length);
		uint16 remainingPixels = 320;
		uint8 *dataPointer = data;
		uint16 x = 0;
		while (remainingPixels > 0) {
			const uint8 &value = dataPointer[0];
			dataPointer++;
			if (value != 0xF0) {
				_bgImageShip.setPixel(x, y, value);
				remainingPixels--;
				debugC(DEBUG_RLE, "RLE : Literal pixel % .2x, remaining row data % .4x bytes.", value, remainingPixels);
				x++;
			} else {
				// We need to decode the RLE data
				const uint8 &runlength = dataPointer[0];
				dataPointer++;
				const uint8 &encodedValue = dataPointer[0];
				dataPointer++;
				for (int i = 0; i < runlength; i++) {
					_bgImageShip.setPixel(x++, y, encodedValue);
				}
				remainingPixels -= runlength;
				debugC(DEBUG_RLE, "RLE: Encoded pixel %.2x for %.2x reps, remaining row data %.4x bytes.", encodedValue, runlength, remainingPixels);
			}
		}
	}

	// We load the palette right afterwards - 0x300 is exactly 3 * 256d
	Common::Array<uint8_t> palette;
	palette.resize(0x300);
	_fileStream->read(palette.data(), 0x300);

	// TODO: Copy-pasted code here
	// Make a copy that will not be color corrected, for fading
	memcpy(_palVanilla, palette.data(), 256 * 3);
	memcpy(_pal, palette.data(), 0x300);

	// Adjust the palette
	for (int i = 0; i < 256 * 3; i++) {
		_pal[i] = (_pal[i] * 259 + 33) >> 6;
	}

	// Continuing with data, even if we don't know all uses yet
	//Common::Array<uint8_t> unknownData1;
	//unknownData1.resize(0x100);
	_fileStream->read(_shadingTable, 0x100);

	uint8_t unknownByte1 = _fileStream->readByte();
	uint8_t unknownByte2 = _fileStream->readByte();
	uint8_t unknownByte3 = _fileStream->readByte();

	// Offset 1013h
	Graphics::ManagedSurface unknownRLE1 = readRLEImage(_fileStream->pos(), _fileStream);
	// TODO: Try if this is it
	_depthMap.blitFrom(unknownRLE1);

	// Offset 2017h
	Graphics::ManagedSurface unknownRLE2 = readRLEImage(_fileStream->pos(), _fileStream);
	// TODO: I think I got it wrong that there is only one map, there are several
	_pathfindingMap.blitFrom(unknownRLE2);

	// Offset 301Bh
	Graphics::ManagedSurface unknownRLE3 = readRLEImage(_fileStream->pos(), _fileStream);
	
	// Offset 401Fh
	// This is the first map used in 0037:10C4 for the lookup of interacted hotspots
	Graphics::ManagedSurface bgMap = readRLEImage(_fileStream->pos(), _fileStream);
	_map = bgMap;

	array5023.clear();
	// We will address this array as words, so we are not using a byte array but a word array
	array5023.resize(0xa0);
	_fileStream->read(array5023.data(), 0xa0);

	word50D3 = _fileStream->readUint16LE();


	array50D5.clear();
	array50D5.resize(0x20 / 2);
	_fileStream->read(array50D5.data(), 0x20);

	// TODO: Remove the now superfluous one
	ReadBackgroundAnimations(_fileStream);


	


	// TODO: There are some more data points missing from the function

	// Refresh characters
	View1 *currentView = (View1 *)findView("View1");

	// Refresh the surface
	currentView->_backgroundSurface = _bgImageShip;


	currentView->characters.clear();
	for (auto currentObject : GameObjects::instance().Objects) {
		if (currentObject->SceneIndex == newSceneIndex) {
			Character *c = new Character();
			c->GameObject = currentObject;
			currentView->characters.push_back(c);
		}
	}

	// Load the script and execute it
	Scenes::instance().LastSceneIndex = Scenes::instance().CurrentSceneIndex;
	Scenes::instance().CurrentSceneIndex = newSceneIndex;
	Scenes::instance().CurrentSceneScript = Scenes::instance().ReadSceneScript(newSceneIndex, _fileStream);
	Scenes::instance().CurrentSceneStrings = Scenes::instance().ReadSceneStrings(newSceneIndex, _fileStream);
	Scenes::instance().CurrentSceneSpecialAnimOffsets = Scenes::instance().ReadSpecialAnimsOffsets(newSceneIndex, _fileStream);
	_scriptExecutor->SetScript(Scenes::instance().CurrentSceneScript);

	

	if (executeScript) {
		// Start the execution
		_scriptExecutor->Run(true);
	}


	// TODO: Other important areas
}

bool Macs2Engine::FindGlyph(char c, GlyphData &out) const {
	for (int i = 0; i < numGlyphs; i++) {
		if (_glyphs[i].ASCII == c) {
			out = _glyphs[i];
			return true;
		}
	}
	return false;
}

uint16 Macs2Engine::getWalkabilityAt(uint16 x, uint16 y){
	// TODO: Handle only the basic case, and add an exception handling for the special case
	// Look up the value from the map
	// TODO: Implement the checks for bounds
	uint16 value = _pathfindingMap.getPixel(x, y);
	if (value < 0x0C8 || value > 0x0EF) {
		return value;
	}
	error("Unhandled code in walkability check encountered");
	return 0;
	
};

bool Macs2Engine::isPathWalkable(uint16 x1, uint16 y1, uint16 x2, uint16 y2) {
	// TODO: need to figure out which algorithm the game uses exactly

	// Trace a line between p1 and p2
	// Look up in the map if we are walkable

	// Bresenham's line algorithm, as described by Wikipedia
	const bool steep = ABS(y2 - y1) > ABS(x2 - x1);

	if (steep) {
		SWAP(x1, y1);
		SWAP(x2, y2);
	}

	const int delta_x = ABS(x2 - x1);
	const int delta_y = ABS(y2 - y1);
	const int delta_err = delta_y;
	int x = x1;
	int y = y1;
	int err = 0;

	const int x_step = (x1 < x2) ? 1 : -1;
	const int y_step = (y1 < y2) ? 1 : -1;

	uint16 walkability = 0;
	int stopFlag = 0;

	
	if (steep) {
		walkability = getWalkabilityAt(y, x);
		stopFlag = false; // (*plotProc)(y, x, data);
	}	
	else {
		walkability = getWalkabilityAt(x, y);
		stopFlag = false; // (*plotProc)(x, y, data);
	}
	if (walkability > 0x0C7) {
		return false;
	}
	
	while (x != x2 && !stopFlag) {
		x += x_step;
		err += delta_err;
		if (2 * err > delta_x) {
			y += y_step;
			err -= delta_x;
		}
		if (steep) {
			walkability = getWalkabilityAt(y, x);
			stopFlag = false; // (*plotProc)(y, x, data);
		} else {
			walkability = getWalkabilityAt(x, y);
			stopFlag = false; // (*plotProc)(x, y, data);
		}
		if (walkability > 0x0C7) {
			return false;
		}
	}
	// return stopFlag;

	return true;
}

void Macs2Engine::CalculatePath(const Common::Point& source, const Common::Point& destination) {
	_path.clear();
	// Check if the destination is reachable at first
	if (isPathWalkable(source.x, source.y, destination.x, destination.y)) {
		_path.push_back(source);
		_path.push_back(destination);
	}

}

// Does what 9F23 does
uint16 ScriptReadWord(Common::MemoryReadStream* stream) {
	const int64 pos = stream->pos();
	const uint16 result = stream->readUint16LE();
	debug("Script read (word): %.4x at offset %.4x", result, pos);
	return result;
}

// Does pretty much what 9F07 does
byte ScriptReadByte(Common::MemoryReadStream* stream) {

	const int64 pos = stream->pos();
	const byte result = stream->readByte();
	debug("Script read (byte): %.2x at offset %.4x", result, pos);
	return result;
}

// #define ScriptNoEntry assert(false);
#define ScriptNoEntry debug("Unhandled case in script handling");

void Func9F4DClean(Common::MemoryReadStream *stream, uint16 &out1, uint16 &out2) {
	// TODO: Implement the actual prelude here correctly, documenting which lables we pass as we go
	debug("-- Entering 9F4D");


	/*
		;; Make space for Ch bytes
	;; Note: I don't think this has any inputs, but it does have some state since it returns different (hotspots?)
	;; if called repeatedly
	enter	0Ch,0h
	;; This function has some internal state and iterates over some list
	call	far 0037h:9F07h
	;; For the analysis of the call that returns the correct active hotspot, it returns 1100h, of which only the AL part is taken
	mov	[bp-5h],al
	;; Note: This call will return the right hotspot value
	call	far 0037h:9F23h
	mov	[bp-7h],ax
	cmp	byte ptr [bp-5h],0h
	jnz	9F72h
	*/







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
	if (opcode == 0x00) {
		// l0037_9F67:
		// TODO: Confirm that nothing else will be done in this case
		out1 = value;
		out2 = 0;
		debug("-- Leaving 94FD");
		return;
	}
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
		if (value == 0x02) {
			// TODO: Look up clicked hotspot instead
			out1 = 0x0801;
			out2 = 0x0000;
			return;
		} else
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


void Macs2Engine::PlaySound() {
	OPL::OPL *_opl = OPL::Config::create();
	int status = _opl->init();
	
	#define CALLBACKS_PER_SECOND 10
	_opl->start(new Common::Functor0Mem<void, Macs2Engine>(this, &Macs2Engine::OnTimer), CALLBACKS_PER_SECOND);
	// _opl->write(0x388, 0x00);
	/*  _opl->writeReg(0x388, 0xA0);
	_opl->writeReg(0x389, 0x36);
	_opl->writeReg(0x388, 0x20);
	_opl->writeReg(0x389, 0x20); */
	// initialize();
	/* _opl->writeReg(0x20, 0x01);
	_opl->writeReg(0x40, 0x10);
	_opl->writeReg(0x60, 0xF0);
	_opl->writeReg(0x80, 0x77);
	_opl->writeReg(0xA0, 0x98);
	_opl->writeReg(0x23, 0x01);
	_opl->writeReg(0x43, 0x00);
	_opl->writeReg(0x63, 0xF0);
	_opl->writeReg(0x83, 0x77); */
	// _opl->writeReg(0xB0, 0x31);

	/*
	    REGISTER     VALUE     DESCRIPTION
 |        20          01      Set the modulator's multiple to 1
 |        40          10      Set the modulator's level to about 40 dB
 |        60          F0      Modulator attack:  quick;   decay:   long
 |        80          77      Modulator sustain: medium;  release: medium
 |        A0          98      Set voice frequency's LSB (it'll be a D#)
 |        23          01      Set the carrier's multiple to 1
 |        43          00      Set the carrier to maximum volume (about 47 dB)
 |        63          F0      Carrier attack:  quick;   decay:   long
 |        83          77      Carrier sustain: medium;  release: medium
 |        B0          31      Turn the voice on; set the octave and freq MSB
 |
 |   To turn the voice off, set register B0h to 11h (or, in fact, any value 
 |   which leaves bit 5 clear).  It's generally preferable, of course, to
 |   induce a delay before doing so.
 |*/
}

void Macs2Engine::OnTimer() {
}


void Macs2Engine::NextCursorMode() {
	// TODO: Adjust for final min and max
	// TODO: Properly handle
	if (_scriptExecutor->_mouseMode == Script::MouseMode::UseInventory || _scriptExecutor->_mouseMode == Script::MouseMode::Walk) {
		_scriptExecutor->_mouseMode = Script::MouseMode::Talk;
	} else {
		_scriptExecutor->_mouseMode = static_cast<Script::MouseMode>(static_cast<int>(_scriptExecutor->_mouseMode) + 1);
	}
}

uint16_t Macs2Engine::GetInteractedBackgroundHotspot(const Common::Point &p) {
	uint16_t result = 0;
	// TODO: Abstract the screen sizes
	if (p.x < 0 || p.x > 320 || p.y < 0 || p.y > 200) {
		return result;
	}

	// [bp-8h]
	uint8_t firstLookup = _map.getPixel(p.x, p.y);
	// [bp-10h] - Guess is that this is the number of hotspots
	// TODO: Actually load from file
	uint16_t numHotspots = word50D3;

	uint8_t i = 1;
	if (i > numHotspots) {
		return result;
	}

	// TODO: need to load from the file, and need to change to words
	Common::Array<uint16_t> a = array50D5;

	// TODO: Handle loop properly
	do {
		// TODO: Not sure if this should be a byte or a word
		// TODO: To check if it's important that we clear the first half of the word
		uint16_t lookup = a[i-1];
		if (lookup == firstLookup) {
			// TODO: Add the 5BD1h lookup part
			// This would check for a value other than FFh in that array
			// If there is a different value, we use that, if not, we use the
			// one we have here
			return 0x800 + i;
		}
		i++;
		// TODO: Should it be <= ?
	} while (i <= numHotspots);
	return 0;
}

void Macs2Engine::ScheduleRun(bool initScene) {
	runScheduled = true;
	scheduledRunIsInitScene = initScene;
}

uint16_t Macs2Engine::Func0E8C(const Common::Point &p) {
	// TODO: Check against screen extent
	uint8_t value = _pathfindingMap.getPixel(p.x, p.y);
	if (value < 0xC8 || value > 0xEF) {
		return value;
	}

	uint16_t lookupIndex = value;
	lookupIndex << 1;
	lookupIndex << 1;
	lookupIndex += value;
	// TODO: We should look up based on byte ptr es:[di+4EA5h] here
	bool lookedUpValue = false;
	if (!lookedUpValue) {
		return 0xFF;
	} else {
		// TODO: We need to look up based on es:[di+4EA6h]
		return 0xAB;
	}
}

int Macs2Engine::MeasureString(Common::String &s) {
	int sum = 0;
	GlyphData currentGlyph;
	bool found = false;
	for (auto current = s.begin(); current != s.end(); current++) {
		if (*current == ' ') {
			// TODO: Check if we hit this one
			sum += 8 + 1;
		} else {
			found = FindGlyph(*current, currentGlyph);
			// TODO: Check if found
			sum += currentGlyph.Width + 1;
			// TODO: Check the rules for adding a 1
		}
	}
	return sum;
}

int Macs2Engine::MeasureStrings(Common::StringArray sa) {
	int max = -1;
	for (auto iter = sa.begin(); iter != sa.end(); iter++) {
		max = MAX(MeasureString(*iter), max);
	}
	return max;
}

Common::StringArray Macs2Engine::DecodeStrings(Common::MemoryReadStream *stream, int offset, int numStrings) {
	Common::StringArray result(numStrings);
	stream->seek(offset);

	byte x;
	byte y;
	byte r;
	
	for (int i = 0; i < numStrings; i++) {
		Common::String currentLine;
		uint16 length = stream->readUint16LE();
		byte currentByte; 
		for (int index = 1; index < length+1; index++) {
			currentByte = stream->readByte();
			x = (byte)(index * index * 0x0c);
			y = (byte)(currentByte ^ index);
			r = (byte)(x ^ y);
			currentLine += (char)r;
			
		}
		result[i] = currentLine;
	}

	return result;
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

	// Initialize Adlib
	_adlib->Init();

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

	// Some assumptions
	// We assume that the objects data array has been loaded completely before we ever load or save
	// If we make changes to any game object data we have to save it

	s.syncAsSint32LE(Scenes::instance().CurrentSceneIndex);
	View1 *currentView = (View1 *)findView("View1");
	if (s.isLoading()) {
		currentView->started = true;
		changeScene(Scenes::instance().CurrentSceneIndex, false);
	}


	// Sync script variables
	int32_t numVariables = _scriptExecutor->_variables.size();
	// TODO: Assuming that this array will always be the same size
	for (int i = 0; i < numVariables; i++) {
		s.syncAsUint16LE(_scriptExecutor->_variables[i].a);
		s.syncAsUint16LE(_scriptExecutor->_variables[i].b);
	}
	
	// Iterate over objects
	// Iterate over characters?
	// TODO: Why save the indices? Would only make sense if we saved other data as well
	uint32_t numObjects = GameObjects::instance().Objects.size();
	for (auto currentObject : GameObjects::instance().Objects) {
		s.syncAsUint16LE(currentObject->Index);
		s.syncAsSint16LE(currentObject->Position.x);
		s.syncAsSint16LE(currentObject->Position.y);
		s.syncAsUint16LE(currentObject->SceneIndex);
	}

	// Handle the view
	uint32_t numCharacters = 0;
	if (s.isSaving()) {
		numCharacters = currentView->characters.size();
	} else {
		currentView->characters.clear();
	}
	uint32_t bytesSynced = s.bytesSynced();
	s.syncAsUint32LE(bytesSynced);
	assert(bytesSynced + 4 == s.bytesSynced());
	s.syncAsUint32LE(numCharacters);
	for (int i = 0; i < numCharacters; i++) {
		uint32_t characterIndex;
		if (s.isSaving()) {
			characterIndex = currentView->characters[i]->GameObject->Index;
		}
		s.syncAsUint32LE(characterIndex);
		Character *currentCharacter;
		if (s.isSaving()) {
			currentCharacter = currentView->characters[i];
		} else {
			currentCharacter = new Character();
			currentCharacter->GameObject = GameObjects::instance().Objects[characterIndex - 1];
			currentView->characters.push_back(currentCharacter);
		}		
	}

	/* uint32_t numInventoryItems;
	if (s.isSaving()) {
		numInventoryItems = currentView->inventoryItems.size();
	} else {
		currentView->inventoryItems.clear();
	}
	s.syncAsUint32LE(numInventoryItems);
	for (int i = 0; i < numInventoryItems; i++) {
		uint32_t objectIndex;
		if (s.isSaving()) {
			objectIndex = currentView->inventoryItems[i]->Index;
		}
		s.syncAsUint32LE(objectIndex);
		GameObject *currentItem;
		if (s.isLoading()) {
			currentItem = GameObjects::instance().Objects[objectIndex - 1];
			currentView->inventoryItems.push_back(currentItem);
		}
	}
	*/

	return Common::kNoError;
}

bool Macs2Engine::tick() {
	_scriptExecutor->tick();
	if (runScheduled) {
		runScheduled = false;
		bool shouldRunInit = scheduledRunIsInitScene;
		scheduledRunIsInitScene = false;
		_scriptExecutor->Run(shouldRunInit);
	}
	return Events::tick();
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

void GlyphData::ReadFromMemory(Common::MemoryReadStream *stream) {
	int64 stride = stream->pos();
	ASCII = stream->readByte();
	Width = stream->readUint16LE();
	Height = stream->readUint16LE();
	Data = new byte[Width * Height];
	stream->read(Data, Width * Height);
	stride = stream->pos() - stride;
}

void AnimFrame::ReadFromeFile(Common::File &file) {
	Width = file.readUint16LE();
	Height = file.readUint16LE();
	Data = new byte[Width * Height];
	file.read(Data, Width * Height);
}

void AnimFrame::ReadFromStream(Common::MemoryReadStream *stream) {
	Width = stream->readUint16LE();
	Height = stream->readUint16LE();
	Data = new byte[Width * Height];
	stream->read(Data, Width * Height);
}

bool AnimFrame::PixelHit(const Common::Point& point) const {
	// TODO: We are ignoring z painting for now
	if (point.x < 0 || point.x >= Width || point.y < 0 || point.y >= Height) {
		return false;
	}
	return Data[point.y * Width + point.x] != 0;
}

Common::Point AnimFrame::GetBottomMiddleOffset() const {
	return Common::Point(Width / 2, Height);
}

AnimFrame BackgroundAnimationBlob::GetFrame(uint32_t index) {
	AnimationReader animReader(Blob);
	uint16_t numAnimations = animReader.readNumAnimations();
	debug("Number of animation frames for background object: %.4", numAnimations);

	// TODO: Check consistency between 0 and 1 based indexing
	animReader.SeekToAnimation((index - 1) % numAnimations);
	// testReader.SeekToAnimation(0);
	// Skip ahead to the width and height
	animReader.readStream->seek(6, SEEK_CUR);

	AnimFrame result;

	result.ReadFromStream(animReader.readStream);
	return result;
	// TODO: Think about proper memory management
}

bool BackgroundAnimationBlob::GetIsAutoUpdated() const {
	Common::MemoryReadStreamEndian stream(Blob.data(), Blob.size(), false);


	/* Arguments:
	Argument - Byte value: [bp+06]: 00 - Determines if we save the result
	Argument - Value between 0 and something around A4: [bp+08]: 0000 - The important one
	Argument - Address: [bp+0a]: 99e0:022f
	Argument - Address: [bp+0e]: 99e2:022f
	Argument - Address: [bp+12]: 0000:04bf
	*/

	
	// bp-22h
	uint16_t bp22 = stream.readUint16();
	// bp-6h
	uint16_t bp6 = stream.readUint16();
	// bp-8h
	uint16 bp8 = stream.readUint16();
	// bp-0Ah
	uint16 bp0A = stream.readUint16();
	// bp-10h
	uint16 bp10 = stream.readUint16();
	// bp-0Eh
	uint16_t bp0E = stream.readUint16() + 1;
	if (bp6 >= bp0E) {
		bp6 = 1;
	}

	stream.seek(bp6 - 1, SEEK_CUR);
	uint8_t bp0C = stream.readByte();
	if (bp0C == 0x1) {
		// l00B7_14B4:
		// TODO: Continue here
	}


	// l00B7_14FD:
	// This would be a loop, but we will stop after the first round anyway

	

	return false;
}

void BackgroundAnimationBlob::Func1480(bool writeResult, Common::MemoryReadStream *stream) {
	/*
	;; Arguments
;; [bp+6h]
;; [bp+8h]
;; [bp+Ah]
;; [bp+Ch]
;; [bp+Eh]
;; [bp+10h]
;; [bp+12h] - Pointer to the segment in which the animation data lives
fn00B7_1480 proc
	enter	24h,0h
	push	ds
	;; Load DS:SI from [bp+12h] argument - presumably the address of the animation blob data
	lds	si,[bp+12h]
	;; Load the first words in to -22h, -6h, -8h, -0Ah, -10h, -0Eh (note: last one increased by 1)
	;; TODO: Could this be an animation counter?
	lodsw
	mov	[bp-22h],ax
	lodsw
	mov	[bp-6h],ax
	lodsw
	mov	[bp-8h],ax
	lodsw
	mov	[bp-0Ah],ax
	lodsw
	mov	[bp-10h],ax
	lodsw
	inc	ax
	mov	[bp-0Eh],ax
	;; PRINT: Print locals here
	;; Add [bp-6h] - 1(second read word decremented by 1) to SI
	add	si,[bp-6h]
	dec	si
	xor	ah,ah
	;; Load a byte from the adjusted SI and save it to [bp-0Ch]
	lodsb
	mov	[bp-0Ch],ax
	pop	ds
	;; Compare argument [bp+8h] with 1 (can be several values, not just a bool)
	mov	ax,[bp+8h]
	;; PRINT: Print control flow here
	;; 14AC
	cmp	ax,1h
	jnz	14C5h

l00B7_14B4:
	;; if [bp+8h] == 1
	xor	ax,ax
	;; Reset [bp-8h] and [bp-10h] (both read at the start) to 0
	mov	[bp-8h],ax
	xor	ax,ax
	mov	[bp-10h],ax
	;; [bp-6h] is set to 1 (was originally read from the data)
	mov	word ptr [bp-6h],1h
	jmp	14EFh

l00B7_14C5:
	cmp	ax,65h
	jl	14EFh

l00B7_14CA:
	;; if [bp+8h] >= 0x65
	cmp	ax,0A4h
	jg	14EFh

l00B7_14CF:
	;; [bp+8h] >= 0x65 && <= 0xA4
	;; Bring it back to starting at 0 by subtracting 0x64
	mov	ax,[bp+8h]
	sub	ax,64h
	;; Set [bp-6h] to the adjusted value (seems to be always set from this source)
	mov	[bp-6h],ax
	;; Reset [bp-8h] and [bp-10h]
	xor	ax,ax
	mov	[bp-8h],ax
	xor	ax,ax
	mov	[bp-10h],ax
	;; Compare the adjusted value with [bp-0Eh] (the last read value which was incremented by 1)
	mov	ax,[bp-6h]
	cmp	ax,[bp-0Eh]
	jbe	14EFh

l00B7_14EA:
	;; PRINT: Consider printing this part
	;; if [bp-6h] > [bp-0Eh]: Set [bp-6h] to 1
	mov	word ptr [bp-6h],1h

l00B7_14EF:
	;; [bp+8h] > 0xA4 or >= 0x65 and was adjusted
	;; Prep of the loop - reset to 1 in case we need to wrap
	mov	ax,[bp-6h]
	cmp	ax,[bp-0Eh]
	;; Jump if below
	jc	14FCh

l00B7_14F7:
	;; PRINT: Consider this part
	;; Do the check again (probably for the case where we skipped the last one)
	mov	word ptr [bp-6h],1h

l00B7_14FC:
	push	ds

l00B7_14FD:
	;; Start of a loop over [bp-6h]
	;; This gets us to the right SI position which will be returned
	;; Which SI do we start from here?
	mov	ax,[bp-6h]
	cmp	ax,[bp-0Eh]
	jc	150Ah

l00B7_1505:
	;; PRINT: Consider this part
	;; if [bp-6h] >= [bp-0Eh] set it to 1
	mov	word ptr [bp-6h],1h

l00B7_150A:
	;; Load DS:SI again from [bp+12h]
	lds	si,[bp+12h]
	;; Skip 0Bh bytes ahead - puts us on the address behind the "header" read above
	add	si,0Bh
	;; Skip ahead by [bp-6h] and load the byte at that point, save it to [bp-0Ch]
	add	si,[bp-6h]
	xor	ah,ah
	lodsb
	mov	[bp-0Ch],ax
	;; Check if the read value is 1
	;; PRINT: Print the read value and the control flow afterwards
	cmp	word ptr [bp-0Ch],1h
	jnz	1533h

l00B7_151F:
	;; If the read value == 1
	;; Jump ahead by 1, load a byte, and jump ahead again (are these bytes in a word array?)
	inc	word ptr [bp-6h]
	lodsb
	inc	word ptr [bp-6h]
	;; Save result to [bp-8h]
	mov	[bp-8h],ax
	;; Save [bp-6h] to [bp-0Ah]
	mov	ax,[bp-6h]
	mov	[bp-0Ah],ax
	xor	ax,ax
	;; Loop
	jmp	14FDh

l00B7_1533:
	;; Check for the read value being 2h
	cmp	word ptr [bp-0Ch],2h
	jnz	1545h

l00B7_1539:
	;; if the read value == 0x2h
	;; Load a byte and save it to [bp-10h]
	inc	word ptr [bp-6h]
	lodsb
	mov	[bp-10h],ax
	inc	word ptr [bp-6h]
	;; Loop
	jmp	14FDh

l00B7_1545:
	;; Check if the read value is 3h
	cmp	word ptr [bp-0Ch],3h
	jnz	1554h

l00B7_154B:
	;; if [bp-0Ch] is 3h - this means we are wrapping around or rather that we skip to a specific
	;; place read from the data
	;; Load a byte and save to to [bp-6h]
	inc	word ptr [bp-6h]
	lodsb
	mov	[bp-6h],ax
	;; Loop
	;; PRINT: Print the read value and that we are looping
	jmp	14FDh

l00B7_1554:
	;; if read value was not one of 1,2 or 3
	;; And the first instruction after the loop
	;; Load a value from the [bp-0Eh] additional offset and save it to [bp-24h]
	;; The result = cx is the number of animation frames to skip forward
	;; E.g. for opening the box bg anim, this is 0xC and is pulled back to 0x2
	sub	ax,0Ah
	mov	cx,ax
	lds	si,[bp+12h]
	;; Load start address again - this is the last time we do this,
	;; we navigate to the right frame afterwards
	add	si,0Bh
	add	si,[bp-0Eh]
	lodsw
	mov	[bp-24h],ax
	;; PRINT: Print values here
	;; Check if CX (set from AX - TODO which value remains there after the loop?) is <= [bp-24h]
	cmp	cx,ax
	jbe	156Dh

l00B7_156A:
	;; if cx > [bp-24h], set cx to 1
	;; This seems to be the point where we wrap around
	mov	cx,1h

l00B7_156D:
	;; Loop determined by the count in CX
	;; This one moves us ahead to the right offset of the animation frame we want to get to,
	;; so we continue up here to see where it is set initially
	;; We read words into [bp-1A], [bp-1C], skip 2 bytes, [bp-16]=bx, [bp-18]
	lodsw
	mov	[bp-1Ah],ax
	lodsw
	mov	[bp-1Ch],ax
	add	si,2h
	lodsw
	mov	[bp-16h],ax
	mov	bx,ax
	lodsw
	mov	[bp-18h],ax
	mul	bx
	;; Multiply [bp-16] and [bp-18], result goes to bx
	;; TODO: Very probably these are the width and height
	mov	bx,ax
	dec	cx
	jz	158Dh

l00B7_1589:
	;; If cx != 0, add bx = [bp-16] * [bp-18h] to si, and loop
	add	si,bx
	jmp	156Dh

l00B7_158D:
	;; We are done with the loop
	;; #path_anim_result: Subtract 6 from SI
	sub	si,6h
	;; #path_anim_result: This is the offset for the animation, so it is set from SI
	mov	[bp-12h],si
	pop	ds
	;; Skip if a lot if [bp+8h] != 2
	mov	ax,[bp+8h]
	cmp	ax,2h
	jnz	15C3h

l00B7_159C:
	;; if [bp+8h] == 2
	cmp	word ptr [bp-0Ch],0Ah
	;; SKip a lot of [bp-0Ch] >= 0Ah
	jc	15C3h

l00B7_15A2:
	;; [bp-0Ch] < 0xAh
	;; Increase [bp-6h]
	inc	word ptr [bp-6h]
	;; Compare [bp-10h] with 0
	cmp	word ptr [bp-10h],0h
	jbe	15AEh

l00B7_15AB:
	// if [bp-10h] > 0, decrease by one
	dec	word ptr [bp-10h]

l00B7_15AE:
	cmp	word ptr [bp-10h],0h
	;; If [bp-10h] != 0, skip ahead
	jnz	15C3h

l00B7_15B4:
	;; if [bp-10h] == 0
	cmp	word ptr [bp-8h],0h
	;; Skip ahead if [bp-8h] <= 0
	jbe	15C3h

l00B7_15BA:
	;; Decrease [bp-8h]
	dec	word ptr [bp-8h]
	;; Copy [bp-0Ah] value to [bp-6h]
	mov	ax,[bp-0Ah]
	mov	[bp-6h],ax

l00B7_15C3:
	;; Compare [bp-6] with [bp-0Eh]
	mov	ax,[bp-6h]
	cmp	ax,[bp-0Eh]
	jc	15D0h

l00B7_15CB:
	;; if [bp-6] >= [bp-0E], reset it to 1
	mov	word ptr [bp-6h],1h

l00B7_15D0:
	cmp	byte ptr [bp+6h],0h
	jz	15EFh

l00B7_15D6:
	;; if [bp+6h] != 0
	;; [bp+6h] is one of the separating variables between an auto-animated background animation
	;; and a manually changed background animation like a door
	push	es
	les	di,[bp+12h]
	;; TODO: This is where we write the data into the anim frame header
	mov	ax,[bp-22h]
	stosw
	;; The AH value is the second one read
	mov	ax,[bp-6h]
	stosw
	mov	ax,[bp-8h]
	stosw
	mov	ax,[bp-0Ah]
	stosw
	mov	ax,[bp-10h]
	stosw
	pop	es

l00B7_15EF:
	;; fallthrough and case [bp+6h] == 0
	;; Save some more data
	mov	ax,[bp-1Ah]
	les	di,[bp+0Eh]
	mov	es:[di],ax
	mov	ax,[bp-1Ch]
	les	di,[bp+0Ah]
	mov	es:[di],ax
	;; #path_anim_result: The offset comes from [bp-12h]
	mov	ax,[bp-12h]
	les	di,[bp+12h]
	;; #path_anim_result: The segment comes from [bp+12h] - so already decided before?
	mov	dx,es
	mov	[bp-4h],ax
	mov	[bp-2h],dx
	;; #path_anim_result: These are the segment and offset where the animation data is loaded from
	;; ax is the offset
	;; dx is the segment
	mov	ax,[bp-4h]
	mov	dx,[bp-2h]
	leave
	retf	10h
	*/
}

} // End of namespace Macs2

/*
private void TextBox_TextChanged(object sender, TextChangedEventArgs e)
		{
			string[] hexValuesSplit = InputTextBox.Text.Split(' ');
			string result = String.Empty;
			byte index = 1;
			foreach (string hex in hexValuesSplit)
			{
				byte value;
				// Convert the number expressed in base-16 to an integer.
				try
				{
					value = Convert.ToByte(hex, 16);
				}
				catch
				{
					return;
				}
				byte x = (byte)(index * index * 0x0c);
				byte y = (byte)(value ^ index);
				byte r = (byte)(x ^ y);
				result += (char)r;
				index++;
			}
			OutputTextBlock.Text = result;
		}

*/
