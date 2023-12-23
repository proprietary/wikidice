/************************************************************************************
   Copyright (C) 2020 MariaDB Corporation AB

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with this library; if not see <http://www.gnu.org/licenses>
   or write to the Free Software Foundation, Inc.,
   51 Franklin St., Fifth Floor, Boston, MA 02110, USA
*************************************************************************************/


#ifndef _CALLABLESTATEMENTCACHEKEY_H_
#define _CALLABLESTATEMENTCACHEKEY_H_

#include "StringImp.h"

namespace sql
{
namespace mariadb
{

class CallableStatementCacheKey
{
  const std::string database;
  const std::string query;

public:
  CallableStatementCacheKey(const SQLString& database, const SQLString& query);
  size_t hashCode() const;
  bool operator== (const CallableStatementCacheKey& other) const;
};

}
}

namespace std
{
  template <>
  struct hash<sql::mariadb::CallableStatementCacheKey>
  {
    std::size_t operator()(const sql::mariadb::CallableStatementCacheKey& key) const
    {
      return key.hashCode();
    }
  };
}

#endif
