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

/* C API to the decompressor interpreter */

#ifndef DECOMPRESSOR_SRC_INTERP_DECOMPRESS_H
#define DECOMPRESSOR_SRC_INTERP_DECOMPRESS_H

#include <stdint.h>

#ifdef _cpluscplus
extern "C" {
#endif

struct Decompressor;

/* Returns an allocated and initialized decompressor. */
Decompressor* create_decompressor();

/* Requestes a buffer of Size bytes. Assumes the lifetime of the
 * buffer is to the next call to get_next_decompressor_buffer() or
 * destroy_decompressor() (which ever comes first).
 */
uint8_t* get_next_decompressor_input_buffer(Decompressor* D, int32_t Size);

int32_t resume_decompression(Decompressor* D);

uint8_t* get_next_decompressor_output_buffer(Decompressor* D, int32_t Size);

/* Clean up D and then deallocates. */
void destroy_decompressor(Decompressor* D);

#ifdef _cpluscplus
}
#endif

#endif  // DECOMPRESSOR_SRC_INTERP_DECOMPRESS_H