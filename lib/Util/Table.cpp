
#include "opencrun/Util/Table.h"

using namespace opencrun::util;

const Table::NotAvailable Table::NA = Table::NotAvailable();
const Table::EndOfLine Table::EOL = Table::EndOfLine();
  
void Table::Dump(llvm::raw_ostream &OS, llvm::StringRef Prefix) const {
  for(unsigned I = 0, E = RowsCount(); I < E; ++I) {
    if(!Prefix.empty()) {
      OS.changeColor(llvm::raw_ostream::GREEN) << Prefix << ": ";
      OS.resetColor();
    }

    for(unsigned J = 0, K = ColsCount(); J < K; ++J) {
      llvm::StringRef Cell = Columns[J][I];
      OS.indent(Columns[J].GetWidth() - Cell.size()) << Cell;
      if(J != K - 1)
        OS << " ";
    }
    OS << "\n";
  }
}
