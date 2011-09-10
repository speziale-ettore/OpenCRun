
//===- Table.h - A table pretty printer -----------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file defines contains the definition of the Table pretty printer. It
// allows to print tables with right-justified columns.
//
//===----------------------------------------------------------------------===//

#ifndef OPENCRUN_UTIL_TABLE_H
#define OPENCRUN_UTIL_TABLE_H

#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/raw_ostream.h"

namespace opencrun {
namespace util {

//===----------------------------------------------------------------------===//
/// Column - Helper class to keep track of the contents of a column.
//===----------------------------------------------------------------------===//
class Column {
public:
  typedef llvm::SmallVector<std::string, 4> RowsContainer;

public:
  Column() : Width(0) { }

public:
  size_t size() const { return Rows.size(); }

  size_t GetWidth() const { return Width; }

public:
  Column &operator<<(llvm::StringRef Str) {
    Width = std::max(Width, Str.size());
    Rows.push_back(Str);
    return *this;
  }

  llvm::StringRef operator[](unsigned I) const {
    if(I < Rows.size())
      return Rows[I];
    else
      return "";
  }

private:
  RowsContainer Rows;
  size_t Width;
};

//===----------------------------------------------------------------------===//
/// Table - Table printer. Use C++ stream writer-like methods to insert fields
///  into the table. Once the table is filled, call the Dump method to write its
///  contents to a stream. Columns are right-justified.
//===----------------------------------------------------------------------===//
class Table {
public:
  class NotAvailable { };
  class EndOfLine { };

public:
  static const NotAvailable NA;
  static const EndOfLine EOL;

public:
  typedef llvm::SmallVector<Column, 8> ColumnsContainer;

public:
  Table() : CurCol(0) { }

public:
  unsigned RowsCount() const {
    if(Columns.empty())
      return 0;
    else
      return Columns.front().size();
  }

  unsigned ColsCount() const { return Columns.size(); }

public:
  Table &operator<<(llvm::StringRef Str) {
    if(CurCol >= Columns.size())
      Columns.push_back(Column());

    Columns[CurCol++] << Str;
    return *this;
  }

  Table &operator<<(double Num) { return operator<<(llvm::ftostr(Num)); }

  Table &operator<<(const NotAvailable &NA) { return operator<<("N/A"); }
  Table &operator<<(const EndOfLine &EOL) { CurCol = 0; return *this; }

  void Dump(llvm::raw_ostream &OS, llvm::StringRef Prefix) const;

private:
  ColumnsContainer Columns;
  unsigned CurCol;
};

} // End namespace util.
} // End namespace opencrun.

#endif // OPENCRUN_UTIL_TABLE_H
