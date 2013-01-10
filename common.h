/*******************************************************************************
 * Copyright (c) 2012-2013 Emmanuel Goossaert
 * This file is part of FelixDB.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or any later
 * version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 ******************************************************************************/

#ifndef HERMESDB_COMMON
#define HERMESDB_COMMON

#include <stdint.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>

#include <string>

#include "status.h"
#include "logger.h"

#define index_t uint64_t
#define offset_t uint64_t
#define kvsize_t uint32_t
#define num_t uint64_t
#define ALIGNMENT_DATA 8 // in bytes

uint64_t FitToPageSize(uint64_t size);


struct FreeMemoryBlock {
  offset_t offset_data;
  uint64_t size;

  bool operator <(const FreeMemoryBlock& fmb) const {
    if (size < fmb.size || (size == fmb.size && offset_data > fmb.offset_data)) {
      return true;
    }
    return false;
  }
};

struct FreeMemoryBlockExtended {
  index_t index;
  offset_t offset_data;
  uint64_t size;
};


struct FreeMemoryHeader {
  num_t size_file;
  num_t num_pages;
  num_t num_slots_free;
  num_t num_slots_total;
};


#endif
