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

#ifndef MACS2_H
#define MACS2_H

#include "common/scummsys.h"
#include "common/system.h"
#include "common/error.h"
#include "common/fs.h"
#include "common/hash-str.h"
#include "common/random.h"
#include "common/serializer.h"
#include "common/util.h"
#include "common/file.h"
#include "engines/engine.h"
#include "engines/savestate.h"
#include "graphics/screen.h"

#include "macs2/detection.h"
#include "macs2/events.h"
#include "macs2/script/scriptexecutor.h"
#include <common/memstream.h>
#include "audio/audiostream.h"

namespace Macs2 {

	class MacsAudioStream : public Audio::SeekableAudioStream {
public:
		Common::MemoryReadStream *_fileStream;

		int64 startPosition;
		int64 length;
		int64 pos;

		virtual ~MacsAudioStream() {}

		/**
		 * Fill the given buffer with up to @p numSamples samples.
		 *
		 * Data must be in native endianness, 16 bits per sample, signed. For stereo
		 * stream, the buffer will be filled with interleaved left and right channel
		 * samples, starting with the left sample. Furthermore, the samples in the
		 * left and right are summed up. So if you request 4 samples from a stereo
		 * stream, you will get a total of two left channel and two right channel
		 * samples.
		 *
		 * @return The actual number of samples read, or -1 if a critical error occurred.
		 *
		 * @note You *must* check whether the returned value is less than what you requested.
		 *       This indicates that the stream is fully used up.
		 *
		 */
		virtual int readBuffer(int16 *buffer, const int numSamples);

		/** Check whether this is a stereo stream. */
		virtual bool isStereo() const;

		/** Sample rate of the stream. */
		virtual int getRate() const;

		/**
		 * Check whether end of data has been reached.
		 *
		 * If this returns true, it indicates that at this time there is no data
		 * available in the stream. However, there might be more data in the future.
		 *
		 * This is used by e.g. a rate converter to decide whether to keep on
		 * converting data or to stop.
		 */
		virtual bool endOfData() const;
	
	virtual bool seek(const Audio::Timestamp &where);

	virtual Audio::Timestamp getLength() const;
	};

	struct Macs2GameDescription;

// enum class CursorMode { Talk = 0, Look = 1, Touch = 2, Walk = 3};
class Adlib;



struct GlyphData {
	byte* Data;
	char ASCII;
	uint16 Width;
	uint16 Height;

	void ReadFromeFile(Common::File &file);
	void ReadFromMemory(Common::MemoryReadStream* stream);
};

struct Sprite {
	uint16_t Width;
	uint16_t Height;
	Common::Array<uint8_t> Data;
};

struct AnimFrame {
	byte *Data;
	uint16 Width;
	uint16 Height;

	void ReadFromeFile(Common::File &file);
	void ReadFromStream(Common::MemoryReadStream *stream);
	bool PixelHit(const Common::Point &point) const;
	Common::Point GetBottomMiddleOffset(uint16_t scale = 100) const;
	Sprite AsSprite();
};

struct BackgroundAnimation {
	uint16 numFrames;
	uint16 X;
	uint16 Y;
	AnimFrame *Frames;
	uint32 FrameIndex;
};

struct BackgroundAnimationBlob {
	uint16 X;
	uint16 Y;
	Common::Array<uint8_t> Blob;
	uint32 FrameIndex;
	AnimFrame GetFrame(uint32_t index);
	AnimFrame GetCurrentFrame();
	static uint16_t Func1480(Common::Array<uint8_t>& blob, bool bpp6, uint16_t bpp8);
	static uint16_t Func168C(Common::Array<uint8_t> &blob);
};



enum DebugFlag {
	DEBUG_RLE = 1 << 10,
	DEBUG_SV = 1 << 11
};

struct PathfindingPoint {
	uint8_t Index;
	Common::Point Position;
	Common::Array<uint8_t> adjacentPoints;
};

struct PathfindingAreaOverride {
	bool Active;
	uint16_t Index;
	uint16_t OverrideValue;
};

class Macs2Engine : public Engine, public Events {
private:
	const ADGameDescription *_gameDescription;
	Common::RandomSource _randomSource;
	
	Adlib *_adlib;

protected:
	// Engine APIs
	Common::Error run() override;

	/**
	 * Returns true if the game should quit
	 */
	bool shouldQuit() const override {
		return Engine::shouldQuit();
	}

	// TODO: Switch stream to an LE stream
	Graphics::ManagedSurface readRLEImage(int64 offs, Common::MemoryReadStream *stream);

	void readResourceFile();

	// We also need some data from the executable, specifically embedded
	// Adlib data
	void readExecutable();

	// Assumes that the stream is at the location of the number of background animations
	void ReadBackgroundAnimations(Common::MemoryReadStream *stream);

	// Assumes that the stream is at the start of the right section
	void ReadImageResources(Common::MemoryReadStream *stream);

public:
	Macs2Engine(OSystem *syst, const ADGameDescription *gameDesc);
	~Macs2Engine() override;

	void changeScene(uint32_t newSceneIndex, bool executeScript = true);

	Script::ScriptExecutor *_scriptExecutor;
	struct Graphics::ManagedSurface _bgImageShip;
	Graphics::ManagedSurface _map;
	// Note: This is used both for pathfinding as well as for area IDs
	Graphics::ManagedSurface _pathfindingMap;

	// This is the depth map
	Graphics::ManagedSurface _depthMap;

	byte _pal[256 * 3] = { 0 };
	byte _palVanilla[256 * 3] = { 0 };

	Common::Array<Common::String> debugOutput;

	Common::Array<PathfindingAreaOverride> PathfindingOverrides;

	// This is the override list living at [5BD1]
	Common::Array<uint16_t> HotspotOverrides;

	bool GetPathfindingOverride(uint16_t index, uint16_t& result);
	void SetPathfindingOverride(uint16_t index, uint16_t overrideValue);

	// This one implements the lookup relative to es:[di+4EA8h] vs. the other one at es:[di+4EA5h] and es:[di+4EA6h]
	uint8_t GetPathfindingOverride2(uint16_t index);
	void RemovePathfindingOverride(uint16_t index);

	// fn0037_0E8C proc
	uint16 getWalkabilityAt(uint16 x, uint16 y);

	// fn0037_1196
	bool isPathWalkable(uint16 x1, uint16 y1, uint16 x2, uint16 y2);

	uint16 _pathfindingPoints[32];
	Common::Array<PathfindingPoint> pathfindingPoints;

	Common::Array<Common::Point> _path;

	Common::Array<Macs2::AnimFrame> imageResources;

	void CalculatePath(const Common::Point &source, const Common::Point &destination);

	byte* _charData;
	char _charASCII;
	uint16 _charWidth;
	uint16 _charHeight;

	GlyphData _glyphs[256];
	// TODO: THis count could be read from the file as well
	uint16 numGlyphs = 79;
	uint16 maxGlyphHeight;

	AnimFrame _animFrames[6];
	// TODO: Figure out how the game knows that there are 6 frames - and confirm that there are only 6 frames

	bool FindGlyph(char c, GlyphData &out) const;

	byte** _cursorData;
	uint16* _cursorWidths;
	uint16* _cursorHeights;

	// TODO: Need a data structure for this by now or check if a bitmap with transparent pixels for blitting exists in ScummVM
	byte* _guyData;
	uint16 _guyWidth;
	uint16 _guyHeight;

	Sprite _borderSprite;
	byte* _borderData;
	uint16 _borderWidth;
	uint16 _borderHeight;

	byte* _shadingTable;

	Sprite _borderHighlightSprite;
	byte* _borderHighlightData;
	uint16 _borderHighlightWidth;
	uint16 _borderHighlightHeight;

	Sprite _borderShadowSprite;

	byte** _flagData;
	uint16* _flagWidths;
	uint16* _flagHeights;

	uint16 _numBackgroundAnimations;
	BackgroundAnimation *_backgroundAnimations;
	Common::Array<BackgroundAnimationBlob> _backgroundAnimationsBlobs;

	byte* mapData;

	Common::MemoryReadStream *_fileStream;

	// Common::MemoryReadStream* _stringsStream;
	uint16 numBytesStrings;
	byte *stringsData;

	// CursorMode _cursorMode = CursorMode::Touch;

	void NextCursorMode();

	void SetCursorMode(Script::MouseMode newMode);

	void DumpStream(Common::MemoryReadStream *s, uint16_t len);

	// Offset 5023h of current scene data
	// TODO: Consider moving somewhere else
	Common::Array<uint8_t> array5023;

	Common::Array<uint16_t> array50D5;

	// [51F7h]
	uint16_t numPathfindingPoints;

	// [51FDh]
	uint16_t word51FD;

	// [51FFh]
	uint16_t word51FF;

	// [5201h]
	uint16_t word5201;

	uint16_t word5203;
	uint16_t word5205;

	Common::Array<uint32_t> array520D;

	void loadAnimationFromSceneData(uint16_t objectIndex, uint16_t slotIndex, uint8_t arrayIndex);

	// TODO: Arguments
	void loadSongFromSceneData(uint8_t dataIndex);

	void playTestSound();

	// Offset 50D3h - This is used in 0037:10C4 to terminate the loop
	uint16_t word50D3;

	uint16_t GetInteractedBackgroundHotspot(const Common::Point &p);



	
	AnimFrame _stick;
	
	Common::Array<uint16_t> inventoryIconIndices;
	
	void RunScriptExecutor(bool firstRun = false) {
		_scriptExecutor->Run(firstRun);
	}

	bool runScheduled = false;
	// TODO: Feels like this should be more elegantly solved, also check how the game does this
	// Is required for example after a scene change
	bool scheduledRunIsInitScene = false;

	// Schedules a run of the script the next time the executor is ticked
	void ScheduleRun(bool initScene = false);

	uint16_t Func0E8C(const Common::Point &p);

	int MeasureString(Common::String &s);

	int MeasureStrings(Common::StringArray sa);
	int MeasureStringsVertically(Common::StringArray sa);

	Common::StringArray DecodeStrings(Common::MemoryReadStream *stream, int offset, int numStrings);

	void PlaySound();

	void OnTimer();

	uint32 getFeatures() const;

	/**
	 * Returns the game Id
	 */
	Common::String getGameId() const;

	/**
	 * Gets a random number
	 */
	uint32 getRandomNumber(uint maxNum) {
		return _randomSource.getRandomNumber(maxNum);
	}

	bool hasFeature(EngineFeature f) const override {
		return
		    (f == kSupportsLoadingDuringRuntime) ||
		    (f == kSupportsSavingDuringRuntime) ||
		    (f == kSupportsReturnToLauncher);
	};

	bool canLoadGameStateCurrently() override {
		return true;
	}
	bool canSaveGameStateCurrently() override {
		return true;
	}

	/**
	 * Uses a serializer to allow implementing savegame
	 * loading and saving using a single method
	 */
	Common::Error syncGame(Common::Serializer &s);

	Common::Error saveGameStream(Common::WriteStream *stream, bool isAutosave = false) override {
		Common::Serializer s(nullptr, stream);
		return syncGame(s);
	}
	Common::Error loadGameStream(Common::SeekableReadStream *stream) override {
		Common::Serializer s(stream, nullptr);
		return syncGame(s);
	}

	virtual bool tick();
};

extern Macs2Engine *g_engine;
#define SHOULD_QUIT ::Macs2::g_engine->shouldQuit()

} // End of namespace Macs2

#endif // MACS2_H
