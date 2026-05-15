#ifndef STRING_TABLE_H
#define STRING_TABLE_H

#include <string>
#include <unordered_set>
#include <memory>

namespace compiler {

class StringTable {
public:
    static StringTable& instance() {
        static StringTable instance;
        return instance;
    }

    const std::string* intern(const std::string& str) {
        auto it = strings_.find(str);
        if (it != strings_.end()) {
            return &*it;
        }
        auto result = strings_.insert(str);
        return &*result.first;
    }

    const std::string* intern(const char* str) {
        return intern(std::string(str));
    }

private:
    std::unordered_set<std::string> strings_;
};

} // namespace compiler

#endif // STRING_TABLE_H
