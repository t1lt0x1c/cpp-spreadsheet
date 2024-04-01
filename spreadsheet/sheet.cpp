
#include "sheet.h"

#include "cell.h"
#include "common.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <optional>

using namespace std::literals;

Sheet::~Sheet() {}

void Sheet::SetCell(Position pos, std::string text) {
    if (!pos.IsValid()) throw InvalidPositionException("Invalid position");
    
    const auto& cell = cells.find(pos);

    if (cell == cells.end()) cells.emplace(pos, std::make_unique<Cell>(*this));
    cells.at(pos)->Set(std::move(text));
}


void Sheet::ClearCell(Position pos) {
    if (!pos.IsValid()) throw InvalidPositionException("Invalid position");

    const auto& cell = cells.find(pos);
    if (cell != cells.end() && cell->second != nullptr) {
        cell->second->Clear();
        if (!cell->second->IsReferenced()) {
            cell->second.reset();
        }
    }
}

Size Sheet::GetPrintableSize() const {
    Size result{ 0, 0 };
    
    for (auto it = cells.begin(); it != cells.end(); ++it) {
        if (it->second != nullptr) {
            const int col = it->first.col;
            const int row = it->first.row;
            result.rows = std::max(result.rows, row + 1);
            result.cols = std::max(result.cols, col + 1);
        }
    }

    return { result.rows, result.cols };
}

void Sheet::PrintValues(std::ostream& out) const {
    Size size = GetPrintableSize();
    for (int row = 0; row < size.rows; ++row) {
        for (int col = 0; col < size.cols; ++col) {
            if (col > 0) out << "\t";
            const auto& it = cells.find({ row, col });
            if (it != cells.end() && it->second != nullptr && !it->second->GetText().empty()) {
                std::visit([&](const auto value) { out << value; }, it->second->GetValue());
            }
        }
        out << "\n";
    }
}
void Sheet::PrintTexts(std::ostream& out) const {
    Size size = GetPrintableSize();
    for (int row = 0; row < size.rows; ++row) {
        for (int col = 0; col < size.cols; ++col) {
            if (col > 0) out << "\t";
            const auto& it = cells.find({ row, col });
            if (it != cells.end() && it->second != nullptr && !it->second->GetText().empty()) {
                out << it->second->GetText();
            }
        }
        out << "\n";
    }
}

const CellInterface* Sheet::GetCell(Position pos) const {
    return GetCellPtr(pos);
}

CellInterface* Sheet::GetCell(Position pos) {
    return GetCellPtr(pos);
}

const Cell* Sheet::GetCellPtr(Position pos) const {
    if (!pos.IsValid()) throw InvalidPositionException("Invalid position");

    const auto cell = cells.find(pos);
    if (cell == cells.end()) {
        return nullptr;
    }

    return cells.at(pos).get();
}

Cell* Sheet::GetCellPtr(Position pos) {
    return const_cast<Cell*>(
        static_cast<const Sheet&>(*this).GetCellPtr(pos));
}

std::unique_ptr<SheetInterface> CreateSheet() {
    return std::make_unique<Sheet>();
}
