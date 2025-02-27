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

#ifndef SCI_SCI_H
#define SCI_SCI_H

#include "engines/engine.h"
#include "common/macresman.h"
#include "common/util.h"
#include "common/random.h"
#include "sci/engine/vm_types.h"	// for Selector
#include "sci/debug.h"	// for DebugState
#include "sci/detection.h" // Shared code between detection and engine

struct ADGameDescription;

/**
 * This is the namespace of the SCI engine.
 *
 * Status of this engine: Awesome
 *
 * Games using this engine:
 * - Newer Sierra adventure games (based on FreeSCI)
 *
 * @todo give a concrete list of supported games. Could also
 * list future games, with status for each.
 */
namespace Sci {

struct EngineState;
class Vocabulary;
class ResourceManager;
class Kernel;
class GameFeatures;
class GuestAdditions;
class Console;
class AudioPlayer;
class SoundCommandParser;
class EventManager;
class SegManager;
class ScriptPatcher;
class Sync;

class GfxAnimate;
class GfxCache;
class GfxCompare;
class GfxControls16;
class GfxControls32;
class GfxCoordAdjuster16;
class GfxCursor;
class GfxMacFontManager;
class GfxMacIconBar;
class GfxMenu;
class GfxPaint16;
class GfxPaint32;
class GfxPalette;
class GfxPalette32;
class GfxRemap;
class GfxRemap32;
class GfxPorts;
class GfxScreen;
class GfxText16;
class GfxText32;
class GfxTransitions;
class SciTTS;

#ifdef ENABLE_SCI32
class GfxFrameout;
class Audio32;
class Video32;
class GfxTransitions32;
class GfxCursor32;
#endif

// our engine debug levels
enum kDebugLevels {
	kDebugLevelError         = 1 << 0,
	kDebugLevelNodes         = 1 << 1,
	kDebugLevelGraphics      = 1 << 2,
	kDebugLevelStrings       = 1 << 3,
	kDebugLevelMemory        = 1 << 4,
	kDebugLevelFuncCheck     = 1 << 5,
	kDebugLevelBresen        = 1 << 6,
	kDebugLevelSound         = 1 << 7,
	kDebugLevelBaseSetter    = 1 << 8,
	kDebugLevelParser        = 1 << 9,
	kDebugLevelSaid          = 1 << 10,
	kDebugLevelFile          = 1 << 11,
	kDebugLevelTime          = 1 << 12,
	kDebugLevelRoom          = 1 << 13,
	kDebugLevelAvoidPath     = 1 << 14,
	kDebugLevelDclInflate    = 1 << 15,
	kDebugLevelVM            = 1 << 16,
	kDebugLevelScripts       = 1 << 17,
	kDebugLevelGC            = 1 << 18,
	kDebugLevelResMan        = 1 << 19,
	kDebugLevelOnStartup     = 1 << 20,
	kDebugLevelDebugMode     = 1 << 21,
	kDebugLevelPatcher       = 1 << 22,
	kDebugLevelWorkarounds   = 1 << 23,
	kDebugLevelVideo         = 1 << 24,
	kDebugLevelGame          = 1 << 25
};


/** Supported languages */
enum kLanguage {
	K_LANG_NONE = 0,
	K_LANG_ENGLISH = 1,
	K_LANG_FRENCH = 33,
	K_LANG_SPANISH = 34,
	K_LANG_ITALIAN = 39,
	K_LANG_GERMAN = 49,
	K_LANG_JAPANESE = 81,
	K_LANG_PORTUGUESE = 351
};

class SciEngine : public Engine {
	friend class Console;
public:
	SciEngine(OSystem *syst, const ADGameDescription *desc, SciGameId gameId);
	~SciEngine() override;

	// Engine APIs
	Common::Error run() override;
	bool hasFeature(EngineFeature f) const override;
	void pauseEngineIntern(bool pause) override;
	void severeError();
	Console *getSciDebugger();
	Common::Error loadGameState(int slot) override;
	Common::Error saveGameState(int slot, const Common::String &desc, bool isAutosave = false) override;
	bool canLoadGameStateCurrently() override;
	bool canSaveGameStateCurrently() override;
	void syncSoundSettings() override; ///< from ScummVM to the game
	void updateSoundMixerVolumes();
	uint32 getTickCount();
	void setTickCount(const uint32 ticks);
	void errorString(const char *buf1, char *buf2, int size) override;

	/**
	 * Disable support for ScummVM autosaves.
	 *
	 * A lot of SCI games already have an autosaving mechanism.
	 * Also, a lot of games have death screens when the player
	 * does something wrong, and autosaves could kick in when the
	 * death screen is shown, which makes them useless, since
	 * the player can only restore or restart.
	 *
	 * Another place where autosaves could kick in is during
	 * screens with internal loops, e.g. the inventory screen,
	 * where the autosave created would be invalid, as the internal
	 * loop isn't persisted in saved games.
	 *
	 * For now, we allow saving in places where the user has
	 * control via GuestAdditions::userHasControl(), but as
	 * mentioned above, these do not cover cases where the user
	 * does have control, but saving would either be useless (e.g.
	 * in death screens) or invalid saved would be created (e.g.
	 * while the inventory screen is open).
	 *
	 * In the future, if we are able to detect all death screens,
	 * all internal loops and generally all places where saving
	 * shouldn't be allowed, we could re-enable this feature.
	 */
	int getAutosaveSlot() const override { return -1; }

	const SciGameId &getGameId() const { return _gameId; }
	const char *getGameIdStr() const;
	Common::Language getLanguage() const;
	bool isLanguageRTL() const;		// true if language's direction is from Right To Left
	Common::Platform getPlatform() const;
	bool isDemo() const;
	bool isCD() const;
	bool forceHiresGraphics() const;

	/** Returns true if the game's original platform is big-endian. */
	bool isBE() const;

	bool hasParser() const;

	/** Returns true if the game supports native Mac fonts and the fonts are available. */
	bool hasMacFonts() const;
	
	/** Returns true if the game is a Mac version with an icon bar on the bottom. */
	bool hasMacIconBar() const;

	/**
	 * Returns true if the game is a Mac version that used native Mac file dialogs
	 * for saving and restoring. These versions do not include resources for the
	 * normal save and restore screens, so the ScummVM UI must always be used.
	 */
	bool hasMacSaveRestoreDialogs() const;

	inline ResourceManager *getResMan() const { return _resMan; }
	inline ScriptPatcher *getScriptPatcher() const { return _scriptPatcher; }
	inline Kernel *getKernel() const { return _kernel; }
	inline EngineState *getEngineState() const { return _gamestate; }
	inline Vocabulary *getVocabulary() const { return _vocabulary; }
	inline EventManager *getEventManager() const { return _eventMan; }
	inline reg_t getGameObject() const { return _gameObjectAddress; } // Gets the game object VM address

	Common::RandomSource &getRNG() { return _rng; }

	Common::String getSavegameName(int nr) const;
	Common::String getSavegamePattern() const;

	Common::String getFilePrefix() const;

	/** Prepend 'TARGET-' to the given filename. */
	Common::String wrapFilename(const Common::String &name) const;

	/** Remove the 'TARGET-' prefix of the given filename, if present. */
	Common::String unwrapFilename(const Common::String &name) const;

	const char *getGameObjectName(); // Gets the name of the game object (should only be used for identifying fanmade games)

	/**
	 * Checks if we are in a QfG import screen, where special handling
	 * of file-listings is performed.
	 */
	int inQfGImportRoom() const;

	/* Shows a ScummVM message box explaining how to import Qfg saved character files */
	void showQfgImportMessageBox() const;

	void sleep(uint32 msecs);

	void scriptDebug();
	bool checkExportBreakpoint(uint16 script, uint16 pubfunct);
	bool checkSelectorBreakpoint(BreakpointType breakpointType, reg_t send_obj, int selector);
	bool checkAddressBreakpoint(const reg_t &address);

public:
	bool checkKernelBreakpoint(const Common::String &name);

	/**
	 * Processes a multilanguage string based on the current language settings and
	 * returns a string that is ready to be displayed.
	 * @param str		the multilanguage string
	 * @param sep		optional separator between main language and subtitle language,
	 *					if NULL is passed no subtitle will be added to the returned string
	 * @return processed string
	 */
	Common::String strSplitLanguage(const char *str, uint16 *splitLanguage, const char *sep = "\r----------\r");
	Common::String strSplit(const char *str, const char *sep = "\r----------\r") {
		return strSplitLanguage(str, NULL, sep);
	}

	kLanguage getSciLanguage();
	void setSciLanguage(kLanguage lang);
	void setSciLanguage();

	Common::String getSciLanguageString(const Common::String &str, kLanguage requestedLanguage, kLanguage *secondaryLanguage = nullptr, uint16 *languageSplitter = nullptr) const;

	// Check if vocabulary needs to get switched (in multilingual parser games)
	void checkVocabularySwitch();

	// Initializes ports and paint16 for non-sci32 games, also sets default palette
	void initGraphics();

	// Suggest to download the GK2 subtitles patch
	// in the future, we might refactor it to something more generic, if needed
	void suggestDownloadGK2SubTitlesPatch();

public:
	GfxAnimate *_gfxAnimate; // Animate for 16-bit gfx
	GfxCache *_gfxCache;
	GfxCompare *_gfxCompare;
	GfxControls16 *_gfxControls16; // Controls for 16-bit gfx
	GfxCoordAdjuster16 *_gfxCoordAdjuster;
	GfxCursor *_gfxCursor;
	GfxMenu *_gfxMenu; // Menu for 16-bit gfx
	GfxPalette *_gfxPalette16;
	GfxRemap *_gfxRemap16;	// Remapping for the QFG4 demo
	GfxPaint16 *_gfxPaint16; // Painting in 16-bit gfx
	GfxPorts *_gfxPorts; // Port management for 16-bit gfx
	GfxScreen *_gfxScreen;
	GfxText16 *_gfxText16;
	GfxTransitions *_gfxTransitions; // transitions between screens for 16-bit gfx
	GfxMacIconBar *_gfxMacIconBar; // Mac Icon Bar manager
	GfxMacFontManager *_gfxMacFontManager; // null when not applicable
	SciTTS *_tts;

#ifdef ENABLE_SCI32
	GfxControls32 *_gfxControls32; // Controls for 32-bit gfx
	GfxPalette32 *_gfxPalette32; // Palette for 32-bit gfx
	GfxRemap32 *_gfxRemap32; // Remapping for 32-bit gfx
	GfxPaint32 *_gfxPaint32; // Painting in 32-bit gfx
	GfxText32 *_gfxText32;

	Audio32 *_audio32;
	Video32 *_video32;
	GfxFrameout *_gfxFrameout; // kFrameout and the like for 32-bit gfx
	GfxTransitions32 *_gfxTransitions32;
	GfxCursor32 *_gfxCursor32;
#endif

	AudioPlayer *_audio;
	Sync *_sync;
	SoundCommandParser *_soundCmd;
	GameFeatures *_features;
	GuestAdditions *_guestAdditions;

	opcode_format (*_opcode_formats)[4];

	DebugState _debugState;
	uint32 _speedThrottleDelay; // kGameIsRestarting maximum delay

	Common::MacResManager *getMacExecutable() { return &_macExecutable; }

private:
	/**
	 * Initializes a SCI game
	 * This function must be run before script_run() is executed. Graphics data
	 * is initialized iff s->gfx_state != NULL.
	 * @param[in] s	The state to operate on
	 * @return		true on success, false if an error occurred.
	 */
	bool initGame();

	/**
	 * Runs a SCI game
	 * This is the main function for SCI games. It takes a valid state, loads
	 * script 0 to it, finds the game object, allocates a stack, and runs the
	 * init method of the game object. In layman's terms, this runs a SCI game.
	 * @param[in] s	Pointer to the pointer of the state to operate on
	  */
	void runGame();

	/**
	 * Uninitializes an initialized SCI game
	 * This function should be run after each script_run() call.
	 * @param[in] s	The state to operate on
	 */
	void exitGame();

	/**
	 * Loads the Mac executable for SCI1/1.1 games.
	 * This function should only be called on Mac games.
	 * If the executable isn't used, or is missing but optional,
	 * then this function does nothing.
	 */
	void loadMacExecutable();

	/**
	 * Loads Mac native fonts for SCI1/1.1 games that support them.
	 * This function should only be called on Mac games after loadMacExecutable()
	 * has been called. Depending on the game, fonts are loaded from either the
	 * executable or from classicmacfonts.dat. If fonts are not present, then a
	 * warning is logged and SCI fonts are used instead.
	 */
	void loadMacFonts();

	void initStackBaseWithSelector(Selector selector);

	bool gameHasFanMadePatch();
	void setLauncherLanguage();

	const ADGameDescription *_gameDescription;
	const SciGameId _gameId;
	ResourceManager *_resMan; /**< The resource manager */
	ScriptPatcher *_scriptPatcher; /**< The script patcher */
	EngineState *_gamestate;
	Kernel *_kernel;
	Vocabulary *_vocabulary;
	int16 _vocabularyLanguage;
	EventManager *_eventMan;
	reg_t _gameObjectAddress; /**< Pointer to the game object */
	Console *_console;
	Common::RandomSource _rng;
	Common::MacResManager _macExecutable;
	bool _forceHiresGraphics; // user-option for GK1, KQ6, PQ4
};


/**
 * Global instance of the SciEngine class, similar to g_engine.
 * This is a hackish way to make all central components available
 * everywhere. Ideally, we would get rid of this again in the future,
 * but for now it's a pragmatic and simple way to achieve the goal.
 */
extern SciEngine *g_sci;

} // End of namespace Sci

#endif // SCI_SCI_H
