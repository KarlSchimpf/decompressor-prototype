// -*- C++ -*-
//
// Copyright 2016 WebAssembly Community Group participants
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// Defines a pointer into a byte stream.

#ifndef DECOMPRESSOR_SRC_STREAM_CURSOR_H
#define DECOMPRESSOR_SRC_STREAM_CURSOR_H

#include "stream/Queue.h"
#include "utils/Trace.h"

namespace wasm {

namespace decode {

class Cursor;

class Cursor : public PageCursor {
  Cursor& operator=(const Cursor&) = delete;

 public:
  class TraceContext : public utils::TraceClass::Context {
    TraceContext() = delete;
    TraceContext(const TraceContext&) = delete;
    TraceContext& operator=(const TraceContext&) = delete;

   public:
    TraceContext(Cursor& Pos) : Pos(Pos) {}
    ~TraceContext() OVERRIDE;
    void describe(FILE* File) OVERRIDE;

   private:
    Cursor& Pos;
  };
  ~Cursor() {}

  void swap(Cursor& C) {
    std::swap(Type, C.Type);
    std::swap(Que, C.Que);
    std::swap(static_cast<PageCursor&>(*this), static_cast<PageCursor&>(C));
    std::swap(EobPtr, C.EobPtr);
    std::swap(CurByte, C.CurByte);
    std::swap(CurByte, C.CurByte);
    std::swap(GuaranteedBeforeEob, C.GuaranteedBeforeEob);
  }

  void assign(const Cursor& C) {
    Type = C.Type;
    Que = C.Que;
    static_cast<PageCursor&>(*this) = static_cast<const PageCursor&>(C);
    EobPtr = C.EobPtr;
    CurByte = C.CurByte;
    GuaranteedBeforeEob = C.GuaranteedBeforeEob;
  }

  StreamType getType() const { return Type; }

  bool isQueueGood() const { return Que->isGood(); }

  bool isBroken() const { return Que->isBroken(*this); }

  std::shared_ptr<Queue> getQueue() { return Que; }

  bool isEofFrozen() const { return Que->isEofFrozen(); }

  bool atEof() const { return CurAddress == Que->getEofAddress(); }

  size_t getEofAddress() const { return Que->getEofAddress(); }

  BitAddress& getEobAddress() const { return EobPtr->getEobAddress(); }

  void freezeEof() { Que->freezeEof(CurAddress); }

  void close();

  size_t fillSize() { return Que->fillSize(); }

  // ------------------------------------------------------------------------
  // The following methods assume that the cursor is accessing a byte stream.
  // ------------------------------------------------------------------------

  size_t getAddress() const { return CurAddress; }

  // For debugging.
  FILE* describe(FILE* File, bool IncludeDetail = false, bool AddEoln = false);

  utils::TraceClass::ContextPtr getTraceContext();

 protected:
  StreamType Type;
  // The byte queue the cursor points to.
  std::shared_ptr<Queue> Que;
  // End of block address.
  std::shared_ptr<BlockEob> EobPtr;
  uint8_t CurByte;
  size_t GuaranteedBeforeEob;

  Cursor(StreamType Type, std::shared_ptr<Queue> Que)
      : PageCursor(Que->FirstPage, Que->FirstPage->getMinAddress()),
        Type(Type),
        Que(Que),
        EobPtr(Que->getEofPtr()) {
    updateGuaranteedBeforeEob();
  }

  explicit Cursor(const Cursor& C)
      : PageCursor(C),
        Type(C.Type),
        Que(C.Que),
        EobPtr(C.EobPtr),
        CurByte(C.CurByte) {
    updateGuaranteedBeforeEob();
  }

  Cursor(const Cursor& C, size_t StartAddress, bool ForRead)
      : PageCursor(C),
        Type(C.Type),
        Que(C.Que),
        EobPtr(C.EobPtr),
        CurByte(C.CurByte) {
    CurPage = ForRead ? Que->getReadPage(StartAddress)
                      : Que->getWritePage(StartAddress);
    CurAddress = StartAddress;
    updateGuaranteedBeforeEob();
  }

  // Note: The nullary cursor should not be used until it has been assigned
  // a valid value.
  Cursor() : PageCursor(), Type(StreamType::Byte) {}

  void updateGuaranteedBeforeEob() {
    GuaranteedBeforeEob =
        CurPage ? std::min(CurPage->getMaxAddress(), EobPtr->getEobAddress())
                : 0;
  }

  // Returns true if able to fill the buffer with at least one byte.
  bool readFillBuffer();

  // Creates new pages in buffer so that writes can occur. WantedSize is
  // a hint of the expecte growth.
  void writeFillBuffer(size_t WantedSize = Page::Size);

  void fail();
};

}  // end of namespace decode

}  // end of namespace wasm

#endif  // DECOMPRESSOR_SRC_STREAM_CURSOR_H
