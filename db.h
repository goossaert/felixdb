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

#ifndef HERMESDB_DB
#define HERMESDB_DB

#include <map>
#include <string>
#include <iostream>

#include "hashdb.h"

namespace felixdb
{

class DB
{
public:
    DB( const std::string& filename ) {}
    ~DB() {}

    void Get( const std::string& key, std::string* value );
    void Put( const std::string& key, const std::string& value );

private:
    std::map< std::string, std::string > data_;
    std::string filename;
};


#endif // HERMESDB_DB

}; // end namespace felixdb
