/* -*- C++ -*- */
/*
 * Copyright 2016 WebAssembly Community Group participants
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// Implements defaults for stream writers.

#include "interp/ByteWriteStream.h"
#include "stream/ReadCursor.h"

#include <iostream>

namespace wasm {

using namespace decode;
using namespace filt;

namespace interp {

namespace {

// Define LEB128 writers.
#ifdef LEB128_LOOP_UNTIL
#error("LEB128_LOOP_UNTIL already defined!")
#else
#define LEB128_LOOP_UNTIL(EndCond) \
  do {                             \
    uint8_t Byte = Value & 0x7f;   \
    Value >>= 7;                   \
    if (EndCond) {                 \
      Pos.writeByte(Byte);         \
      break;                       \
    } else {                       \
      Pos.writeByte(Byte | 0x80);  \
    }                              \
  } while (true)

template <class Type>
void writeLEB128(Type Value, WriteCursor& Pos) {
  LEB128_LOOP_UNTIL(Value == 0);
}

template <class Type>
void writePositiveLEB128(Type Value, WriteCursor& Pos) {
  LEB128_LOOP_UNTIL(Value == 0 && !(Byte & 0x40));
}

template <class Type>
void writeNegativeLEB128(Type Value, WriteCursor& Pos) {
  LEB128_LOOP_UNTIL(Value == -1 && (Byte & 0x40));
}

template <class Type>
void writeFixedLEB128(Type Value, WriteCursor& Pos) {
  constexpr uint32_t BitsInWord = sizeof(Type) * CHAR_BIT;
  constexpr uint32_t ChunkSize = CHAR_BIT - 1;
  constexpr uint32_t ChunksInWord = (BitsInWord + ChunkSize - 1) / ChunkSize;
  uint32_t Count = 0;
  LEB128_LOOP_UNTIL(++Count == ChunksInWord);
}

#undef LEB128_LOOP_UNTIL

#endif

template <class Type>
void writeFixed(Type Value, WriteCursor& Pos) {
  constexpr uint32_t WordSize = sizeof(Type);
  constexpr Type Mask = (Type(1) << CHAR_BIT) - 1;
  for (uint32_t i = 0; i < WordSize; ++i) {
    Pos.writeByte(uint8_t(Value & Mask));
    Value >>= CHAR_BIT;
  }
}

}  // end of anonymous namespace

void ByteWriteStream::writeUint8Bits(uint8_t Value,
                                     WriteCursor& Pos,
                                     uint32_t /*NumBits*/) {
  Pos.writeByte(uint8_t(Value));
}

void ByteWriteStream::writeUint32Bits(uint32_t Value,
                                      WriteCursor& Pos,
                                      uint32_t /*NumBits*/) {
  writeFixed<uint32_t>(Value, Pos);
}

void ByteWriteStream::writeUint64Bits(uint64_t Value,
                                      WriteCursor& Pos,
                                      uint32_t /*NumBits*/) {
  writeFixed<uint64_t>(Value, Pos);
}

void ByteWriteStream::writeVarint32Bits(int32_t Value,
                                        WriteCursor& Pos,
                                        uint32_t /*NumBits*/) {
  if (Value < 0)
    writeNegativeLEB128<int32_t>(Value, Pos);
  else
    writePositiveLEB128<int32_t>(Value, Pos);
}

void ByteWriteStream::writeVarint64Bits(int64_t Value,
                                        WriteCursor& Pos,
                                        uint32_t /*NumBits*/) {
  if (Value < 0)
    writeNegativeLEB128<int64_t>(Value, Pos);
  else
    writePositiveLEB128<int64_t>(Value, Pos);
}

void ByteWriteStream::writeVaruint32Bits(uint32_t Value,
                                         WriteCursor& Pos,
                                         uint32_t /*NumBits*/) {
  writeLEB128<uint32_t>(Value, Pos);
}

void ByteWriteStream::writeVaruint64Bits(uint64_t Value,
                                         WriteCursor& Pos,
                                         uint32_t /*NumBits*/) {
  writeLEB128<uint64_t>(Value, Pos);
}

bool ByteWriteStream::writeValue(IntType Value,
                                 WriteCursor &Pos,
                                 const Node* Format) {
  switch (Format->getType()) {
    case OpUint8:
      writeUint8(Value, Pos);
      return true;
    case OpUint32:
      writeUint32(Value, Pos);
      return true;
    case OpUint64:
      writeUint64(Value, Pos);
      return true;
    case OpVarint32:
      writeVarint32(Value, Pos);
      return true;
    case OpVarint64:
      writeVarint64(Value, Pos);
      return true;
    case OpVaruint32:
      writeVaruint32(Value, Pos);
      return true;
    case OpVaruint64:
      writeVaruint64(Value, Pos);
    default:
      return false;
  }
}

bool ByteWriteStream::writeAction(WriteCursor& Pos,
                                  const CallbackNode* Action) {
  return true;
}

void  ByteWriteStream::alignToByte(decode::WriteCursor& Pos) {
}

size_t ByteWriteStream::getStreamAddress(WriteCursor& Pos) {
  return Pos.getCurByteAddress();
}

void ByteWriteStream::writeFixedBlockSize(WriteCursor& Pos, size_t BlockSize) {
  writeFixedLEB128<uint32_t>(BlockSize, Pos);
}

void ByteWriteStream::writeVarintBlockSize(decode::WriteCursor& Pos,
                                           size_t BlockSize) {
  writeVaruint32(BlockSize, Pos);
}

size_t ByteWriteStream::getBlockSize(decode::WriteCursor& StartPos,
                                     decode::WriteCursor& EndPos) {
  return EndPos.getCurByteAddress() -
         (StartPos.getCurByteAddress() + ChunksInWord);
}

void ByteWriteStream::moveBlock(decode::WriteCursor& Pos,
                                size_t StartAddress,
                                size_t Size) {
  ReadCursor CopyPos(Pos, StartAddress);
  for (size_t i = 0; i < Size; ++i)
    Pos.writeByte(CopyPos.readByte());
}

}  // end of namespace decode

}  // end of namespace wasm
