#include "cell.h"
#include "sheet.h"

#include <cassert>
#include <iostream>
#include <string>
#include <stack>

// Реализуйте следующие методы
class Cell::Impl {
    public:
        virtual ~Impl() = default;
        virtual std::string GetText() const = 0;
        virtual Value GetValue() const = 0;
        virtual std::vector<Position> GetReferencedCells() const { return {}; }
        virtual bool IsCacheValid() const { return true; }
        virtual void InvalidateCache() {}
    };
    
class Cell::EmptyImpl : public Impl {
    public:
        Value GetValue() const override { return ""; }
        std::string GetText() const override { return ""; }
    };
    
class Cell::TextImpl : public Impl {
    public:
        TextImpl(std::string text)
            : text_(std::move(text)) {
            if (text_.empty()) { throw std::logic_error(""); }
        }

        Value GetValue() const override {
            if (text_[0] == ESCAPE_SIGN) return text_.substr(1);
            return text_;
        }

        std::string GetText() const override {
            return text_;
        }
    private:
        std::string text_;
    };
    
class Cell::FormulaImpl : public Impl {
    public:
        explicit FormulaImpl(std::string expression, const SheetInterface& sheet)
            : sheet_(sheet) {
            if (expression.empty() || expression[0] != FORMULA_SIGN) throw std::logic_error("");

            formula = ParseFormula(expression.substr(1));
        }

        Value GetValue() const override {
            if (!cache) {
                cache = formula->Evaluate(sheet_);
            }

            auto value = formula->Evaluate(sheet_);
            if (std::holds_alternative<double>(value)) return std::get<double>(value);

            return std::get<FormulaError>(value);
        }

        std::string GetText() const override {
            return FORMULA_SIGN + formula->GetExpression();
        }
    
        bool IsCacheValid() const override {
            return cache.has_value();
        }
    
        void InvalidateCache() override {
            cache.reset();
        }
           
        std::vector<Position> GetReferencedCells() const {
            return formula->GetReferencedCells();
        }
            
    private:
        std::unique_ptr<FormulaInterface> formula;
        mutable std::optional<FormulaInterface::Value> cache;
        const SheetInterface& sheet_;
    };

bool Cell::SearchCircularDependency(const Impl& new_impl) const {
    if (new_impl.GetReferencedCells().empty()) {
        return false;
    }

    std::unordered_set<const Cell*> referenced;
    for (const auto& pos : new_impl.GetReferencedCells()) {
        referenced.insert(sheet_.GetCellPtr(pos));
    }

    std::unordered_set<const Cell*> visited;
    std::stack<const Cell*> need_visit;
    need_visit.push(this);
    while (!need_visit.empty()) {
        const Cell* current = need_visit.top();
        need_visit.pop();
        visited.insert(current);

        if (referenced.find(current) != referenced.end()) return true;

        for (const Cell* incoming : current->l_nodes_) {
            if (visited.find(incoming) == visited.end()) need_visit.push(incoming);
        }
    }

    return false;
}

void Cell::InvalidateCacheRecursive(bool need_swap) {
    if (impl_->IsCacheValid() || need_swap) {
        impl_->InvalidateCache();
        for(Cell* incoming : l_nodes_) {
            incoming ->InvalidateCacheRecursive();
        }
    }
}

Cell::Cell(Sheet& sheet)
    : impl_(std::make_unique<EmptyImpl>())
    , sheet_(sheet) {}

Cell::~Cell() {}

void Cell::Set(std::string text) {
    std::unique_ptr<Impl> impl;
    
	if (text.size() == 0) {
        impl = std::make_unique<EmptyImpl>();
    } else if (text[0] == '=' && text.size() > 1) {
        impl = std::make_unique<FormulaImpl>(std::move(text), sheet_);
    } else {
        impl = std::make_unique<TextImpl>(std::move(text));
    }
    
    if(SearchCircularDependency(*impl)) {
        throw CircularDependencyException("");
    }
    
    impl_ = std::move(impl);
    
    for(Cell* out : r_nodes_) {
        out->l_nodes_.erase(this);
    }
    
    r_nodes_.clear();
    
    for(const auto& pos: impl_->GetReferencedCells()) {
        Cell* out = sheet_.GetCellPtr(pos);
        if(!out) {
            sheet_.SetCell(pos, "");
            out = sheet_.GetCellPtr(pos);
        }
        r_nodes_.insert(out);
        out->l_nodes_.insert(this);
    }
    InvalidateCacheRecursive(true);
}

void Cell::Clear() {
	impl_ = std::make_unique<EmptyImpl>();

}

Cell::Value Cell::GetValue() const {
	return impl_->GetValue();
}

std::string Cell::GetText() const {
	return impl_->GetText();
}

std::vector<Position> Cell::GetReferencedCells() const {
    return impl_->GetReferencedCells();
}

bool Cell::IsReferenced() const {
    return !l_nodes_.empty();
}