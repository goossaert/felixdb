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

#include "fb_file_manager.h"

namespace felixdb {

Status SimpleFBFileManager::ExpandFile() {
  munmap(freememory_, size_freememory_);

  uint64_t expansion_coef = 2;
  uint64_t page_size = getpagesize();
  uint64_t num_slots_current = (header_->num_pages * page_size) / sizeof(struct FreeMemoryBlock);

  uint64_t num_slots_new = (header_->num_pages * expansion_coef * page_size) / sizeof(struct FreeMemoryBlock);
  uint64_t size_freememory_new = num_slots_new * sizeof(struct FreeMemoryBlock);

  if (ftruncate(fd_freememory_, page_size + size_freememory_new) != 0) {
    return Status::IOError("Could not expand file", strerror(errno));
  }

  //printf("SimpleFBFileManager::ExpandFile() - Before - %d %d %d %d\n", header_->size_file, header_->num_pages, header_->num_slots_free, header_->num_slots_total);

  // Update file header
  header_->size_file = page_size + size_freememory_new;
  header_->num_pages *= expansion_coef;
  header_->num_slots_free += (num_slots_new - num_slots_current);
  header_->num_slots_total = num_slots_new;
  size_freememory_ = size_freememory_new;

  //printf("SimpleFBFileManager::ExpandFile() - After - %d %d %d %d\n", header_->size_file, header_->num_pages, header_->num_slots_free, header_->num_slots_total);

  // Re-mmap() file
  freememory_ = static_cast<struct FreeMemoryBlock*>(mmap(0,
                                                          size_freememory_,
                                                          PROT_READ | PROT_WRITE,
                                                          MAP_SHARED,
                                                          fd_freememory_,
                                                          page_size));

  // Add new slots to the list of free slots
  for (uint64_t i = num_slots_current; i < num_slots_new; ++i) {
    slots_free_.push_back(i);
  }

  //printf("SimpleFBFileManager mmap addr:[%p]\n", freememory_);

  if (freememory_ == MAP_FAILED) {
    return Status::IOError("mmap() failed", strerror(errno));
  }

  return Status::OK();
}


Status SimpleFBFileManager::CreateFile(const std::string& filename) {
  int page_size = getpagesize();
  char buffer[page_size];

  if ((fd_freememory_ = open(filename.c_str(), O_RDWR | O_CREAT, 0644)) < 0) {
    return Status::IOError("Could not create free memory file", strerror(errno));
  }

  memset(buffer, 0, page_size);
  uint64_t num_slots = page_size / sizeof(struct FreeMemoryBlock);
  size_freememory_ = num_slots * sizeof(struct FreeMemoryBlock);

  struct FreeMemoryHeader *fmh = reinterpret_cast<struct FreeMemoryHeader *>(buffer);
  fmh->size_file = page_size + size_freememory_;
  fmh->num_pages = 1;
  fmh->num_slots_free = num_slots - 1;
  fmh->num_slots_total = num_slots;
  write(fd_freememory_, buffer, page_size);

  memset(buffer, 0, page_size);
  struct FreeMemoryBlock *fmb = reinterpret_cast<struct FreeMemoryBlock *>(buffer);
  fmb->offset_data = offset_init_;
  fmb->size = size_data_ - fmb->offset_data;
  write(fd_freememory_, buffer, size_freememory_);

  close(fd_freememory_);
  return Status::OK();
}


Status SimpleFBFileManager::Open(const std::string& filename) {
  filename_ = filename;
  return OpenFile(filename);
}


Status SimpleFBFileManager::Close() {
  if (close(fd_freememory_) < 0) {
    std::string msg = std::string("Count not close free memory file [") + db_name_ + std::string("]");
    return Status::IOError(msg, strerror(errno));
  }
  return Status::OK();
}


Status SimpleFBFileManager::OpenFile(const std::string& filename) {
  if (access(filename.c_str(), F_OK) == -1) {
    Status s = CreateFile(filename);
    if (!s.IsOK()) return s;
    printf("Free block file created\n");
  } else {
    printf("Free block file already exists\n");
  }
  
  if ((fd_freememory_ = open(filename.c_str(), O_RDWR)) < 0) {
    return Status::IOError(filename, strerror(errno));
  }

  int page_size = getpagesize();

  header_ = static_cast<struct FreeMemoryHeader*>(mmap(0,
                                                       page_size,
                                                       PROT_READ | PROT_WRITE,
                                                       MAP_SHARED,
                                                       fd_freememory_,
                                                       0));

  //printf("SimpleFBFileManager mmap addr:[%p]\n", header_);

  if (header_ == MAP_FAILED) {
    return Status::IOError("mmap() failed for header", strerror(errno));
  }

  // TODO: Move this computation somewhere else
  size_freememory_ = (header_->size_file - page_size);

  freememory_ = static_cast<struct FreeMemoryBlock*>(mmap(0,
                                                          size_freememory_,
                                                          PROT_READ | PROT_WRITE,
                                                          MAP_SHARED,
                                                          fd_freememory_,
                                                          page_size));

  if (freememory_ == MAP_FAILED) {
    return Status::IOError("mmap() failed for memory blocks", strerror(errno));
  }

  int num_slots = (header_->size_file - page_size) / sizeof(struct FreeMemoryBlock);
  int num_free = 0;
  int num_used = 0;
  for (int i=0; i < num_slots; ++i) {
    if (freememory_[i].size == 0) {
      slots_free_.push_back(i);
      num_free++;
    } else {
      FreeMemoryBlockExtended fmbe = { i, freememory_[i].offset_data, freememory_[i].size  };
      freememory_map_[fmbe.offset_data] = fmbe;
      num_used++;
    }
  }

  return Status::OK();
}


Status SimpleFBFileManager::CloseFile() {
  int page_size = getpagesize();
  munmap(header_, page_size);
  munmap(freememory_, size_freememory_);
  close(fd_freememory_);
  return Status::OK();
}



Status SimpleFBFileManager::AddBlock(offset_t off, uint64_t size) {
  if (header_->num_slots_free == 0) {
    Status s = ExpandFile();
    if (!s.IsOK()) return s;
  }

  index_t index = slots_free_.back();
  slots_free_.pop_back();

  FreeMemoryBlockExtended fmbe = { index, off, size };
  freememory_map_[fmbe.offset_data] = fmbe;
  header_->num_slots_free -= 1;
  freememory_[index].offset_data = off;
  freememory_[index].size = size;
  return Status::OK();
}



Status SimpleFBFileManager::DeleteBlock(offset_t off) {
  index_t index = freememory_map_[off].index;
  freememory_map_.erase(off);
  slots_free_.push_back(index);
  header_->num_slots_free += 1;
  freememory_[index].offset_data = 0;
  freememory_[index].size = 0;
  return Status::OK();
}



Status SimpleFBFileManager::UpdateBlock(offset_t off, offset_t off_new, uint64_t size) {
  //printf("UpdateBlock: %d %d %d\n", off, off_new, size);
  FreeMemoryBlockExtended fmbe = freememory_map_[off];
  freememory_map_.erase(off);

  freememory_[fmbe.index].offset_data = off_new;
  freememory_[fmbe.index].size = size;

  fmbe.offset_data = off_new;
  fmbe.size = size;
  freememory_map_[off_new] = fmbe;
  return Status::OK();
}


Status SimpleFBFileManager::Synchronize() {
  int page_size = getpagesize();
  if (msync(header_, page_size, MS_SYNC) != 0) {
    return Status::IOError("Could not synchronize header in FreeBlock file manager", strerror(errno));
  }

  if (msync(freememory_, size_freememory_, MS_SYNC) != 0) {
    return Status::IOError("Could not synchronize blocks in FreeBlock file manager", strerror(errno));
  }

  return Status::OK();
}




};
