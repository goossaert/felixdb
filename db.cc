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

#include "hashdb.h"
#include "index.h"
#include "db.h"


namespace felixdb
{

void DB::Get( const std::string& key, std::string* value )
{
    //std::cout << key << std::endl;
    *value = data_[ key ];
}

void DB::Put( const std::string& key, const std::string& value )
{
    data_[ key ] = value;
    std::cout << data_.size() << std::endl;
}


};


int main()
{
    /*
    std::string value_out("value_out");
    bool ret;
    printf("blah!\n");

    felixdb::HashMap hm;
    hm.salut();

    felixdb::DB db("file.db");

    hm.Put("key1", "value1");
    hm.Put("key2", "value2");
    hm.Put("key31", "qwertyuiopasdfghjklzxcvbnm1234qwertyuiopasdfghjklzxcvbnm1234qwertyuiopasdfghjklzxcvbnm1234qwertyuiopasdfghjklzxcvbnm1234qwertyuiopasdfghjklzxcvbnm1234");
    hm.Put("key13", "qwertyuiopasdfghjklzxcvbnm1234qwertyuiopasdfghjklzxcvbnm1234qwertyuiopasdfghjklzxcvbnm1234qwertyuiopasdfghjklzxcvbnm1234qwertyuiopasdfghjklzxcvbnm1234");
    hm.Get("key1", &value_out );
    std::cout << "out: " << value_out << std::endl;
    hm.Get("key31", &value_out );
    std::cout << "out: " << value_out << std::endl;
    ret = hm.Get("key13", &value_out );
    std::cout << "out: " << value_out << std::endl;
    std::cout << "ret: " << ret << std::endl;
    ret = hm.Get("ke1y3", &value_out );
    std::cout << "ret: " << ret << std::endl;
    ret = hm.Get("ke1y4", &value_out );
    std::cout << "ret: " << ret << std::endl;

    hm.Delete("key13");

    ret = hm.Get("key13", &value_out );
    std::cout << "out: " << value_out << std::endl;
    std::cout << "ret: " << ret << std::endl;
    */

    /*
    db.Put("key1", "value1");
    db.Put("key2", "value2");
    db.Put("key1", "value1b");
    db.Get("key1", &value_out );
    std::cout << "out: " << value_out << std::endl;
    std::cout << hm.HashFunction( "blah1" ) << std::endl;
    std::cout << hm.HashFunction( "blah2" ) << std::endl;
    */

    felixdb::IndexList( "list", 100000 );


    return 0;
}
