#ifndef SOURCE_LOCATION_H
#define SOURCE_LOCATION_H

#include <string>
#include <cstdint>

namespace compiler {

struct SourceLocation {
    uint32_t line;
    uint32_t column;
    uint32_t offset;
    const std::string* filename;

    SourceLocation(uint32_t l = 1, uint32_t c = 1, uint32_t o = 0, const std::string* f = nullptr)
        : line(l), column(c), offset(o), filename(f) {}

    std::string toString() const {
        if (filename) {
            return *filename + ":" + std::to_string(line) + ":" + std::to_string(column);
        }
        return "line " + std::to_string(line) + ", col " + std::to_string(column);
    }
};

struct SourceRange {
    SourceLocation start;
    SourceLocation end;

    SourceRange(SourceLocation s, SourceLocation e) : start(s), end(e) {}
    explicit SourceRange(SourceLocation loc) : start(loc), end(loc) {}
};

} // namespace compiler

#endif // SOURCE_LOCATION_H
