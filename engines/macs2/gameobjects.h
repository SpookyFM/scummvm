#pragma once
class gameobjects {
};
#pragma once

#include "common/array.h"
#include "common/singleton.h"
#include "common/rect.h"

namespace Common {
class MemoryReadStream;
}

namespace Macs2 {

	class Scene {
		public:
		Common::MemoryReadStream *Script;

	};

	class Scenes : public Common::Singleton<Scenes> {
		// Data for scenes can be accessed:
		// Script data: Load from objects data [0752]: ID * 0xC, offset at [di-6] and [di-8h]


		public:
		Common::Array<Scene> Scenes;

		// Global [077Ch]
		int CurrentSceneIndex;

		// TODO: Handle properly as a field of the scene 
		class Common::MemoryReadStream* CurrentSceneScript;

		class Common::MemoryReadStream *CurrentSceneStrings;

		class Common::MemoryReadStream *ReadSceneScript(uint16_t sceneIndex, Common::MemoryReadStream *fileStream);
		class Common::MemoryReadStream *ReadSceneStrings(uint16_t sceneIndex, Common::MemoryReadStream *fileStream);
	};

class GameObject {
public:

	// Index of the object, starting at 1
	uint16_t Index;

	// These are the values read by the code around l0037_082D:
	Common::Point Position;
	uint16_t SceneIndex;
	uint16_t Orientation;
	uint16_t Unknown;

	// Each object can have up to 15h blocks of data that are loaded, which can
	// include the animations, the dialogue images, the inventory icons etc.
	Common::Array<Common::Array<uint8_t> > Blobs;

	// The object-specific script
	// TODO: Random thought - do objects have their own space for script variables?
	Common::Array<uint8_t> Script;
};

class GameObjects : public Common::Singleton<GameObjects> {

public:
	// Maximum of 200h objects
	// How to address them in the original code:
	// mov	di,[bp+6h]
	//	shl di, 2h;
	// les di, [di + 77Ch]
	Common::Array<GameObject*> Objects;

	static GameObject *GetProtagonistObject();
};

} // namespace Macs2
