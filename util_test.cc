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

#include "util_test.h"

namespace felixdb {

void CleanUpFiles() {
  char** files = (char *[]){"/tmp/felixdb/freememory",
                            "/tmp/felixdb/index",
                            "/tmp/felixdb/data"};

  printf("CleanUpFiles()\n");

  for (int i=0; i<3; ++i) {
    if (remove(files[i]) != 0)
      perror ("Error deleting file");
    else
      puts ("File successfully deleted");
  }
}

}


