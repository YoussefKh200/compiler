#ifndef ERROR_REPORTER_H
#define ERROR_REPORTER_H

#include "source_location.h"
#include <vector>
#include <string>
#include <sstream>
#include <iostream>
#include <memory>

namespace compiler {

enum class DiagnosticLevel {
    Note,
    Warning,
    Error,
    Fatal
};

struct Diagnostic {
    DiagnosticLevel level;
    SourceLocation location;
    std::string message;
    std::string sourceLine;
    std::vector<std::pair<SourceLocation, std::string>> notes;

    Diagnostic(DiagnosticLevel lvl, SourceLocation loc, std::string msg)
        : level(lvl), location(loc), message(std::move(msg)) {}
};

class ErrorReporter {
public:
    static ErrorReporter& instance() {
        static ErrorReporter instance;
        return instance;
    }

    void report(DiagnosticLevel level, const SourceLocation& loc, const std::string& message) {
        diagnostics_.emplace_back(level, loc, message);
        
        if (level == DiagnosticLevel::Error || level == DiagnosticLevel::Fatal) {
            errorCount_++;
        } else if (level == DiagnosticLevel::Warning) {
            warningCount_++;
        }

        if (level == DiagnosticLevel::Fatal || immediateExit_) {
            printAll();
            std::exit(1);
        }
    }

    void error(const SourceLocation& loc, const std::string& msg) {
        report(DiagnosticLevel::Error, loc, msg);
    }

    void warning(const SourceLocation& loc, const std::string& msg) {
        report(DiagnosticLevel::Warning, loc, msg);
    }

    void note(const SourceLocation& loc, const std::string& msg) {
        report(DiagnosticLevel::Note, loc, msg);
    }

    void fatal(const SourceLocation& loc, const std::string& msg) {
        report(DiagnosticLevel::Fatal, loc, msg);
    }

    void setImmediateExit(bool val) { immediateExit_ = val; }
    bool hasErrors() const { return errorCount_ > 0; }
    size_t getErrorCount() const { return errorCount_; }
    size_t getWarningCount() const { return warningCount_; }

    void printAll(std::ostream& out = std::cerr) {
        for (const auto& diag : diagnostics_) {
            printDiagnostic(out, diag);
        }
    }

    void clear() {
        diagnostics_.clear();
        errorCount_ = 0;
        warningCount_ = 0;
    }

private:
    ErrorReporter() : errorCount_(0), warningCount_(0), immediateExit_(false) {}

    void printDiagnostic(std::ostream& out, const Diagnostic& diag) {
        const char* levelStr = "";
        const char* color = "";
        
        switch (diag.level) {
            case DiagnosticLevel::Error:   levelStr = "error"; color = "\033[31m"; break;
            case DiagnosticLevel::Warning: levelStr = "warning"; color = "\033[33m"; break;
            case DiagnosticLevel::Note:    levelStr = "note"; color = "\033[36m"; break;
            case DiagnosticLevel::Fatal:   levelStr = "fatal error"; color = "\033[35m"; break;
        }

        out << color << levelStr << ": \033[0m" << diag.message << "\n";
        out << "   --> " << diag.location.toString() << "\n";
        
        if (!diag.sourceLine.empty()) {
            out << "    | \n";
            out << " " << diag.location.line << "  | " << diag.sourceLine << "\n";
            out << "    | ";
            for (uint32_t i = 1; i < diag.location.column; ++i) out << " ";
            out << "^\n";
        }

        for (const auto& note : diag.notes) {
            out << "   = note: " << note.second << " at " << note.first.toString() << "\n";
        }
    }

    std::vector<Diagnostic> diagnostics_;
    size_t errorCount_;
    size_t warningCount_;
    bool immediateExit_;
};

#define REPORT_ERROR(loc, msg) compiler::ErrorReporter::instance().error(loc, msg)
#define REPORT_WARNING(loc, msg) compiler::ErrorReporter::instance().warning(loc, msg)
#define REPORT_FATAL(loc, msg) compiler::ErrorReporter::instance().fatal(loc, msg)

} // namespace compiler

#endif // ERROR_REPORTER_H
