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

#include "memory_manager.h"

namespace felixdb {

Status SimpleMemoryManager::Open(const std::string& filename,
                                 DataManager* data_manager,
                                 kvsize_t size_data,
                                 offset_t offset_init) {
  filename_ = filename;
  data_manager_ = data_manager;
  file_manager_ = new SimpleFBFileManager(filename, size_data, offset_init);
  Status s = file_manager_->Open(filename);
  if (!s.IsOK()) return s;
  std::map< offset_t, FreeMemoryBlockExtended >::const_iterator it;
  for (it = file_manager_->freememory_map_.begin(); it != file_manager_->freememory_map_.end(); ++it) {
    FreeMemoryBlock fmb = { it->second.offset_data, it->second.size };
    freememory_map_[fmb.offset_data] = fmb;
    freememory_set_.insert(fmb);
  }

  return Status::OK();
}


Status SimpleMemoryManager::Close() {
  Status s = file_manager_->Close();
  if (!s.IsOK()) return s;
  return Status::OK();
}




Status SimpleMemoryManager::AllocateMemory(kvsize_t size,
                                           offset_t* offset,
                                           kvsize_t* size_total) {
  int page_size = getpagesize();
  //TODO: what if size == 0?
  kvsize_t size_aligned = size;
  int rest = size % ALIGNMENT_DATA;
  if (rest > 0) size_aligned += ALIGNMENT_DATA - rest;

  LOG_TRACE(classname(), "size set: %d", (int)freememory_set_.size());
  std::set<FreeMemoryBlock>::const_iterator it2 = freememory_set_.begin();
  for (it2; it2 != freememory_set_.end(); ++it2) {
    LOG_TRACE(classname(), "set item: %llu %llu", it2->offset_data, it2->size);
  }

  Status s;

  FreeMemoryBlock fmb = { UINT64_MAX, size_aligned };
  std::set<FreeMemoryBlock>::const_iterator it;
  it = freememory_set_.upper_bound(fmb);
  if (it == freememory_set_.end()) {
    uint64_t expension_coef = 2;
    uint64_t size_data = data_manager_->GetDataSize();
    uint64_t size_data_new = size_data * expension_coef;
    if (size_aligned > size_data_new - size_data) {
      size_data_new = FitToPageSize(size_data + size_aligned);
    }
    LOG_TRACE(classname(), "AllocateMemory() - trying to expand memory [%llu]->[%llu]", size_data, size_data_new);
    s = data_manager_->ExpandMemory(size_data_new);
    if (!s.IsOK()) {
      LOG_TRACE(classname(), "AllocateMemory() - Error: no more memory");
      return s;
    } else {
      FreeMemory(size_data, size_data_new - size_data);
      LOG_TRACE(classname(), "AllocateMemory() - Expand memory by adding new block");
      it = freememory_set_.upper_bound(fmb);
    }
  }

  LOG_TRACE(classname(), "AGAIN size set: %d", (int)freememory_set_.size());
  it2 = freememory_set_.begin();
  for (it2; it2 != freememory_set_.end(); ++it2) {
    LOG_TRACE(classname(), "set item: %llu %llu", it2->offset_data, it2->size);
  }


  // Re-test after maybe some memory was added
  if (it == freememory_set_.end()) {
    return Status::IOError("Out of memory"); 
  }

  uint64_t offset_out = it->offset_data;
  LOG_TRACE(classname(), "AllocateMemory() - offset_out:%llu", offset_out);

  //printf("SimpleMemoryManager::AllocateMemory() - offset_out:%llu size:%llu\n", offset_out, size_aligned);

  if (   (it->size > size_aligned * 2)
      || (it->size > page_size && it->size - size_aligned > 1024))
  {
    // split if the item to put in block is small than half the block,
    // or if the block is really big and a lot of space will be wasted if no split occurs
    //printf("Get block - split: %d %d | %d\n", it->offset_data, it->size, size_aligned);
    s = file_manager_->UpdateBlock(it->offset_data,
                                   it->offset_data + size_aligned,
                                   it->size - size_aligned);
    if (!s.IsOK()) return s;

    FreeMemoryBlock fmb_new = {it->offset_data + size_aligned,
                               it->size - size_aligned};

    freememory_map_.erase(it->offset_data);
    freememory_map_[fmb_new.offset_data] = fmb_new;

    freememory_set_.erase(it);
    freememory_set_.insert(fmb_new);
  } else {
    //printf("Get block - direct\n");
    s = file_manager_->DeleteBlock(it->offset_data);
    if (!s.IsOK()) return s;
    size_aligned = it->size;
    freememory_map_.erase(it->offset_data);
    freememory_set_.erase(it);
  }


  *offset = offset_out;
  *size_total = size_aligned;
  return Status::OK();
}


Status SimpleMemoryManager::FreeMemory(offset_t offset_in, uint64_t size_in) {
  Status s;

  offset_t offset, offset_prec, offset_succ;
  uint64_t size;
  //printf("SimpleMemoryManager::FreeMemory() - IN [%d] [%d]\n", offset_in, size_in);
  bool has_merged_prec = false, has_merged_succ = false;

  // In order to know if the item being freed can be merged with its predecessor
  // or successor, the find() method of freememory_map_ is used. Thus, the item
  // is inserted into that map just the time to know if it can be merged, and it
  // is later removed.
  FreeMemoryBlock fmb_in = { offset_in, size_in };
  freememory_map_[offset_in] = fmb_in;
  //printf("SimpleMemoryManager::FreeMemory() - current [%llu] [%llu] num_items:[%d]\n", offset_in, size_in, freememory_map_.size());

  // Check if can be merged with predecessor
  std::map<offset_t, FreeMemoryBlock>::iterator it_current, it_prec, it_succ;
  it_current = freememory_map_.find(offset_in);
  it_prec = freememory_map_.find(offset_in);
  it_prec--;
  if (it_current != freememory_map_.begin()) {
    //printf("SimpleMemoryManager::FreeMemory() - prev [%llu] [%llu] num_items:[%d]\n", it_prec->second.offset_data, it_prec->second.size, freememory_map_.size());
  }

  if (   it_current != freememory_map_.begin()
      && it_prec->second.offset_data + it_prec->second.size == offset_in) {
    offset_prec = it_prec->second.offset_data;
    offset = offset_prec;
    size = size_in + it_prec->second.size;
    //printf("SimpleMemoryManager::FreeMemory() - Merging with prev block\n");
    has_merged_prec = true;
  } else {
    offset = offset_in;
    size = size_in;
  }

  // Check if can be merged with successor
  it_succ = freememory_map_.find(offset_in);
  it_succ++;
  if (it_succ != freememory_map_.end()) {
    //printf("SimpleMemoryManager::FreeMemory() - succ [%llu] [%llu] num_items:[%d]\n", it_succ->second.offset_data, it_succ->second.size, freememory_map_.size());
  }

  if (   it_succ != freememory_map_.end()
      && it_succ->second.offset_data == offset_in + size_in) {
    offset_succ = it_succ->second.offset_data;
    size += it_succ->second.size;
    //printf("SimpleMemoryManager::FreeMemory() - Merging with succ block\n");
    has_merged_succ = true;
  }

  if(!has_merged_prec && !has_merged_succ) {
    //printf("SimpleMemoryManager::FreeMemory() - No merging\n");
  }


  // Remove item added for merging checks
  freememory_map_.erase(offset_in);

  // Update the memory map in the Memory Manager
  if (has_merged_prec) {
    freememory_map_.erase(offset_prec);
    FreeMemoryBlock fmb_prec = { it_prec->second.offset_data, it_prec->second.size };
    freememory_set_.erase(freememory_set_.find(fmb_prec));
  }
  if (has_merged_succ) {
    freememory_map_.erase(offset_succ);
    FreeMemoryBlock fmb_succ = { it_succ->second.offset_data, it_succ->second.size };
    freememory_set_.erase(freememory_set_.find(fmb_succ));
  }
  FreeMemoryBlock fmb = { offset, size };
  freememory_map_[offset] = fmb;
  freememory_set_.insert(fmb);

  // Synchronize the memory map with the Free Memory File
  if (has_merged_prec) {
    s = file_manager_->UpdateBlock(offset_prec, offset, size);
    if (!s.IsOK()) return s;
  } else if (has_merged_succ) {
    s = file_manager_->UpdateBlock(offset_succ, offset, size);
    if (!s.IsOK()) return s;
  } else {
    s = file_manager_->AddBlock(offset, size);
    if (!s.IsOK()) return s;
  }

  //printf("SimpleMemoryManager::FreeMemory() OUT num_items[%d] --------\n", freememory_map_.size());
  return Status::OK();
}


Status SimpleMemoryManager::Synchronize() {
  return file_manager_->Synchronize();
}



};


