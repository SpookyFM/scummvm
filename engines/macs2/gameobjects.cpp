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

Common::Array<uint32_t> Macs2::Scenes::ReadSpecialAnimsOffsets(uint16_t sceneIndex, Common::MemoryReadStream *fileStream) {
	Common::Array<uint32_t> result;
	result.resize(0x80 / 4);


	// Calculate the offset of the script data offset
	// This addressing can be found in the l0037_2856 code block
	// TODO: This part is copy-pasted and could be refactored into one proper loading function


	uint16_t sceneDataOffset = sceneIndex * 0xC;
	// Offset of the data in [0752h] global
	constexpr uint16_t globalDataOffset = 0xC + 0x4;
	sceneDataOffset += globalDataOffset;
	fileStream->seek(sceneDataOffset - 0x8);
	uint32_t sceneDataOffset2 = fileStream->readUint32LE();
	fileStream->seek(sceneDataOffset2, SEEK_SET);

	// Read the script from there
	// We read 80h bytes for the special animations offsets
	fileStream->read(result.data(), 0x80);
	
	return result;
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

Common::Array<uint8_t> Macs2::Scenes::ReadSpecialAnimBlob(uint16_t index, Common::MemoryReadStream *fileStream) {
	uint32_t offset = CurrentSceneSpecialAnimOffsets[index - 1];
	fileStream->seek(offset, SEEK_SET);
	uint32_t length = fileStream->readUint32LE();
	// Skip a string - note the original code adds 0x4 for the previously read size since
	// it does not use the stream position
	fileStream->seek(0xC, SEEK_CUR);
	Common::Array<uint8_t> result(length);
	fileStream->read(result.data(), length);
	return result;
}

void Macs2::GameObjects::Init() {
	ObjectNames.resize(0xFF);
	// TODO: Make a text file instead
	ObjectNames[0x10] = "Holzfass"; // leer
	ObjectNames[0x11] = "Bowiemesser";
	ObjectNames[0x17] = "Hutschachtel"; // leer und offen
	ObjectNames[0x18] = "Damenhut"; // Mit Schleier
	ObjectNames[0x1a] = "Eimer"; // leer
	ObjectNames[0x1b] = "Feuerhaken";
	ObjectNames[0x23] = "Tasse"; // leer
	ObjectNames[0x24] = "Axt";
	ObjectNames[0x24] = "Whiskyglas"; // leer
	ObjectNames[0x2E] = "Whiykglas";
	ObjectNames[0x30] = "Sch�rhaken";
	ObjectNames[0x43] = "Koffer"; // geschlossen
	ObjectNames[0x47] = "Messer";
	ObjectNames[0x49] = "Kartonschachtel";
	ObjectNames[0x52] = "Stoffbeutel"; // Mit Murmeln
	ObjectNames[0x53] = "Blasebalg";
	ObjectNames[0x54] = "Brennholz";
	ObjectNames[0x55] = "Brot";
	ObjectNames[0x56] = "Brotmesser";
	ObjectNames[0x57] = "Brot";
	ObjectNames[0x58] = "B�cher";
	ObjectNames[0x59] = "Clownpuppe";
	ObjectNames[0x5B] = "Blechdose";
	ObjectNames[0x5D] = "Papierdrache";
	ObjectNames[0x5E] = "Blecheimer";
	ObjectNames[0x61] = "Schnapsflasche";
	ObjectNames[0x62] = "Kerze"; // mit Flamme
	ObjectNames[0x65] = "Spitzhacke";
	ObjectNames[0x66] = "Kerze";
	ObjectNames[0x67] = "Kerzen";
	ObjectNames[0x6B] = "Korkenzieher";
	ObjectNames[0x6A] = "Holzkohle";
	ObjectNames[0x6B] = "Korkenzieher";
	ObjectNames[0x6C] = "Kronleuchter"; // ohne Schirme
	ObjectNames[0x6D] = "Kronleuchter"; // mit Schirmen
	ObjectNames[0x70] = "Musketen";
	ObjectNames[0x72] = "Rad";
	ObjectNames[0x74] = "Schaufel";
	ObjectNames[0x70] = "Musketen";
	ObjectNames[0x73] = "Salpeter";
	ObjectNames[0x74] = "Sch�ssel"; // mit Teig
	ObjectNames[0x76] = "Lampenschirm";
	ObjectNames[0x77] = "Lampenschirme";
	ObjectNames[0x78] = "Messingschl�ssel";
	ObjectNames[0x7C] = "Sch�ssel"; // mit Schwefel
	ObjectNames[0x7F] = "Schwefel";
	ObjectNames[0x82] = "Seil";
	ObjectNames[0x83] = "Socken";
	ObjectNames[0x84] = "Spachtel";
	ObjectNames[0x85] = "Holzente";
	ObjectNames[0x86] = "Streichholz";
	ObjectNames[0x82] = "Seil";
	ObjectNames[0x88] = "Tasse"; // mit �l
	ObjectNames[0x89] = "Teig";
	ObjectNames[0x8A] = "Topflappen";
	ObjectNames[0x8C] = "Wolle";
	ObjectNames[0x8D] = "Wolle"; // mit Alkohol
	ObjectNames[0x8E] = "Holzw�rfel";
	ObjectNames[0x8F] = "Brief";
}

// TODO: Add all to namespace?
Macs2::GameObject *Macs2::GameObjects::GetProtagonistObject() {
	return instance().GetObjectByIndex(1);
}

Macs2::GameObject *Macs2::GameObjects::GetObjectByIndex(uint16_t index) {
	if (index - 1 >= instance().Objects.size()) {
		return nullptr;
	}
	return instance().Objects[index - 1];
}

Common::MemoryReadStream *Macs2::GameObjects::ReadGameObjectStrings(uint16_t index, Common::MemoryReadStream *fileStream) {
	// TODO: Copy&Pasted code from ReadSceneStrings
	// Calculate the offset of the script data offset
	// This addressing can be found in the l0037_2856 code block

	uint16_t gameObjectDataOffset = index * 0xC;
	// Offset of the data in [0752h] global
	constexpr uint16_t globalDataOffset = 0xC + 0x4;
	gameObjectDataOffset += globalDataOffset;
	fileStream->seek(gameObjectDataOffset + 0x17FC);
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

Common::MemoryReadStream *Macs2::GameObject::GetScriptStream() {
	return new Common::MemoryReadStream(Script.data(), Script.size());
}

Macs2::AnimationReader::AnimationReader(const Common::Array<uint8_t> &blob) {
	readStream = new Common::MemoryReadStreamEndian(blob.data(), blob.size(), false);

}

uint16_t Macs2::AnimationReader::readNumAnimations() {
	// Read the header

	readStream->seek(0, SEEK_SET);

	// bp-22h
	readStream->readUint16();
	// bp-6h
	readStream->readUint16();
	// bp-8h
	readStream->readUint16();
	// bp-0Ah
	readStream->readUint16();
	// bp-10h
	readStream->readUint16();

	// bp-0Eh
	// TODO: Naming probably off
	uint16_t firstFrameAdditionalOffset = readStream->readUint16() + 1;

	// Navigate to where bp-24h is saved
	readStream->seek(0x0B + firstFrameAdditionalOffset);

	// bp-24h
	uint16_t result = readStream->readUint16();
	return result;
}

void Macs2::AnimationReader::SeekToAnimation(uint16_t index) {
	// Read bp-0Eh directly
	readStream->seek(0xA, SEEK_SET);
	// bp-0Eh
	// TODO: Naming probably off
	uint16_t firstFrameAdditionalOffset = readStream->readUint16() + 1;
	// Skip reading bp-24h
	readStream->seek(0x0B + firstFrameAdditionalOffset + 0x2, SEEK_SET);
	// TODO: Check for one-off errors
	for (int i = 0; i < index; i++) {
		SkipCurrentAnimationFrame();
	}
	
}

void Macs2::AnimationReader::SkipCurrentAnimationFrame() {
	uint16_t value1 = readStream->readUint16();
	uint16_t value2 = readStream->readUint16();
	readStream->seek(2, SEEK_CUR);
	uint16_t width = readStream->readUint16();
	uint16_t height = readStream->readUint16();
	readStream->seek(width * height, SEEK_CUR);
}
