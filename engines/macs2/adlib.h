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

#ifndef MACS2_ADLIB_H
#define MACS2_ADLIB_H

#include "common/scummsys.h"
#include "common/array.h"
#include "common/stream.h"

namespace Common {
class MemoryReadStream;
class MemorySeekableReadWriteStream;

}

namespace OPL {
	class OPL;
}

namespace Macs2 {

	class StreamHandler : public Common::SeekableReadStream {
		private:
			Common::MemorySeekableReadWriteStream *_stream;
			int64 _pos;
		public:
			StreamHandler(Common::MemorySeekableReadWriteStream* s) : _stream(s) {
			}

	
		bool eos() const override;
		uint32 read(void *dataPtr, uint32 dataSize) override;
		int64 pos() const override;
		int64 size() const override;
		bool seek(int64 offset, int whence) override;
		byte peekByte();
	};

	class Adlib {

	private:
		OPL::OPL *_opl;

		void Func1A03();

		// 01D7:1AA7
		void OnTimer();

		uint16 Func19BE(uint8 offset);

		// TODO: Function should take a pointer to data
		uint16 Func19BE_TODO(uint8 offset);

		Common::MemorySeekableReadWriteStream Func19BE_2(Common::MemorySeekableReadWriteStream &inStream, uint8 seekDelta);

		// TODO: Consider pointer vs. passing by value
		StreamHandler* Func19BE_SH(StreamHandler* inHandler, uint8 seekDelta);

		// TODO: Maybe need to add the caller
		void Func24FD();

		// TODO: Maybe we need to add the caller, fn0017_24FD proc
		void Func2686();

		uint8_t Func2779(uint8_t arg1);

		// Writes a value to the target register
		void Func2792(byte registerIndex, byte value);
		void Func2792r(byte value, byte registerIndex) {
			Func2792(registerIndex, value);
		}
		
		// TODO: Consider adding the caller
		// TODO: Mocked input parameters so far, should be an 8 bit value
		// and a pointer into memory
		void Func2839(uint8_t bpp0A, uint32_t bpp06);

		void Func294E(uint16_t bpp6, uint8_t bpp8, uint16_t bppA);

		// Array accessed as [di + 37h]
		Common::Array<uint8_t> gArray37;

		// Array accessed as [di + 8dh]
		Common::Array<uint8_t> gArray8d;

		// Array accesed as [di + 57]
		Common::Array<uint8_t> gArray57;

		// Array accessed as [di + 5C]
		Common::Array<uint8_t> gArray5C;

		// Array accesed as [di + 96]
		Common::Array<uint8_t> gArray96;

		// Array accessed as [di+9Fh]
		Common::Array<uint8_t> gArray9F;

		// Array accessed as [di+11Fh];
		Common::Array<uint8_t> gArray11F;

		// [223Eh] - Seems to be a 16 bit value - Initial value?
		uint16_t g223E;

		// Memory being pointed to by [2248] and [224A]
		StreamHandler* shMem2248;

		// 	[2254h] and [2256h]
		uint32 _nextEventTimer;

		// [2258h] - TODO: Not sure about size - Initialization
		uint8_t g2258;

		// [225Eh] - TODO: Not sure about size - Initialization
		uint8_t g225E;

		// Array at [di + 226F] - TODO: Initialization, values, ...?
		Common::Array<uint8_t> gArray226F;

		// [2291h] - Not sure how used - initialization?
		uint16_t g2291;

		// [2296h] - TODO: Initialization
		uint16_t g2296;

		// [2298h] - TODO: Initialization
		uint16_t g2298;

		// [225Ah] - TODO: Initial value?
		uint16_t g225A;

		// Array at [222Ch] - accessed with byte values
		// TODO: Initialization, access pattern
		Common::Array<uint8_t> gArray222C;

		// Array at [225Fh] - accessed with byte values
		// TODO: Initialization, access pattern
		Common::Array<uint8_t> gArray225F;

		// Array at [227Fh] - accessed with byte values
		// TODO: Initialization, access pattern, size
		Common::Array<uint8_t> gArray227F;

		// Array at [2288h] - accessed with byte values
		// TODO: Initialization, access pattern, size
		Common::Array<uint8_t> gArray2288;

		// Array at [229Ch] - accessed with byte values
		// TODO: Initialization, access pattern, size
		Common::Array<uint8_t> gArray229C;

		// Array at [2235h] - accessed with bytes values
		// TODO: Initialization, access pattern, size
		Common::Array<uint8_t> gArray2235;

		bool g229A;

		// [229Bh] - seems to contain song byte - TODO: Initial value?
		uint8_t g229B;

		// Reads a byte without advancing the read stream
		uint8_t peekByte();

		// Reads a byte from the specified offset without changing the position in the stream
		uint8_t peekByteAt(uint16_t offset);

		// fn0017_2A80: 0017:2A80
		uint8_t FuncA280();
	

	public:
	void Init();

	void Deinit();

		// TODO: Check where these contents live
		// Memory pointed to by [2250] global
		Common::MemoryReadStream *data = nullptr;
	};

} // namespace Macs2

#endif
