#include "gameobjects.h"
#include "common/memstream.h"

namespace Common {

DECLARE_SINGLETON(Macs2::GameObjects);
DECLARE_SINGLETON(Macs2::Scenes);

} // namespace Common

Common::MemoryReadStream *Macs2::Scenes::ReadSceneScript(uint16_t sceneIndex, Common::MemoryReadStream *fileStream) {
	// Calculate the offset of the script data offset
	// This addressing can be found in the l0037_2856 code block

	uint16_t sceneDataOffset = sceneIndex * 0xC;
	// Offset of the data in [0752h] global
	constexpr uint16_t globalDataOffset = 0xC + 0x4;
	sceneDataOffset += globalDataOffset;
	fileStream->seek(sceneDataOffset - 0x8);
	uint32_t sceneDataOffset2 = fileStream->readUint32LE();
	fileStream->seek(sceneDataOffset2, SEEK_SET);
	

	// Read the script from there
	// We read 80h bytes
	fileStream->seek(0x80, SEEK_CUR);
	uint16_t scriptSize = fileStream->readUint16LE();
	uint8_t *scriptData = new uint8_t[scriptSize];
	fileStream->read(scriptData, scriptSize);
	// TODO: Consider using the endian version for all the memoryReadStreams
	return new Common::MemoryReadStream(scriptData, scriptSize);
}

Common::MemoryReadStream *Macs2::Scenes::ReadSceneStrings(uint16_t sceneIndex, Common::MemoryReadStream *fileStream) {
	// Calculate the offset of the script data offset
	// This addressing can be found in the l0037_2856 code block

	uint16_t sceneDataOffset = sceneIndex * 0xC;
	// Offset of the data in [0752h] global
	constexpr uint16_t globalDataOffset = 0xC + 0x4;
	sceneDataOffset += globalDataOffset;
	fileStream->seek(sceneDataOffset - 0x4);
	uint32_t sceneDataOffset2 = fileStream->readUint32LE();
	fileStream->seek(sceneDataOffset2, SEEK_SET);

	// TODO: Continue here:
	// This lives in l0037_A4FC:

	// Read the script from there
	// Size - lives in global [0F84h]
	uint16_t size = fileStream->readUint16LE();

	// TODO: More things to document from l0037_A532:

	// Stringdata lives in the pointer [0F80h]
	uint8_t *stringData = new uint8_t[size];
	fileStream->read(stringData, size);
	// TODO: Consider using the endian version for all the memoryReadStreams
	return new Common::MemoryReadStream(stringData, size);
	// Note: We save the current scene number to [0F86h] - maybe "scene we have strings loaded for"?
}

// TODO: Add all to namespace?
Macs2::GameObject *Macs2::GameObjects::GetProtagonistObject() {
	return instance().Objects[0];
}

Macs2::GameObject *Macs2::GameObjects::GetObjectByIndex(uint16_t index) {
	if (index - 1 >= instance().Objects.size()) {
		return nullptr;
	}
	return instance().Objects[index - 1];
}
