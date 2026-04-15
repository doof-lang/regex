#pragma once

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "doof_runtime.hpp"

#ifndef PCRE2_CODE_UNIT_WIDTH
#define PCRE2_CODE_UNIT_WIDTH 8
#endif

#ifndef PCRE2_STATIC
#define PCRE2_STATIC
#endif

#include "pcre2.h"

struct NativeRegexCapture {
    bool found;
    std::string text;
    int32_t start;
    int32_t end;
};

class NativeRegexMatch {
public:
    NativeRegexMatch(
        bool found,
        std::string value,
        int32_t start,
        int32_t end,
        std::vector<NativeRegexCapture> captures,
        std::unordered_map<std::string, int32_t> namedGroups
    )
        : found_(found),
          value_(std::move(value)),
          start_(start),
          end_(end),
          captures_(std::move(captures)),
          namedGroups_(std::move(namedGroups)) {}

    bool found() const {
        return found_;
    }

    std::string value() const {
        return value_;
    }

    int32_t start() const {
        return start_;
    }

    int32_t end() const {
        return end_;
    }

    int32_t nextSearchStart(int32_t previousStart) const {
        if (!found_) {
            return previousStart;
        }
        if (end_ > previousStart) {
            return end_;
        }
        return previousStart + 1;
    }

    int32_t captureCount() const {
        return static_cast<int32_t>(captures_.size());
    }

    std::string captureText(int32_t index) const {
        const auto* capture = captureAt(index);
        if (capture == nullptr || !capture->found) {
            return "";
        }
        return capture->text;
    }

    int32_t captureStart(int32_t index) const {
        const auto* capture = captureAt(index);
        if (capture == nullptr || !capture->found) {
            return -1;
        }
        return capture->start;
    }

    int32_t captureEnd(int32_t index) const {
        const auto* capture = captureAt(index);
        if (capture == nullptr || !capture->found) {
            return -1;
        }
        return capture->end;
    }

    bool hasNamedCapture(const std::string& name) const {
        const auto captureIndex = captureIndexForName(name);
        return captureIndex > 0 && captureStart(captureIndex) >= 0;
    }

    std::string namedCaptureText(const std::string& name) const {
        const auto captureIndex = captureIndexForName(name);
        return captureText(captureIndex);
    }

    int32_t namedCaptureStart(const std::string& name) const {
        const auto captureIndex = captureIndexForName(name);
        return captureStart(captureIndex);
    }

    int32_t namedCaptureEnd(const std::string& name) const {
        const auto captureIndex = captureIndexForName(name);
        return captureEnd(captureIndex);
    }

private:
    const NativeRegexCapture* captureAt(int32_t index) const {
        if (index <= 0 || index > static_cast<int32_t>(captures_.size())) {
            return nullptr;
        }
        return &captures_[static_cast<std::size_t>(index - 1)];
    }

    int32_t captureIndexForName(const std::string& name) const {
        const auto it = namedGroups_.find(name);
        if (it == namedGroups_.end()) {
            return -1;
        }
        return it->second;
    }

    bool found_;
    std::string value_;
    int32_t start_;
    int32_t end_;
    std::vector<NativeRegexCapture> captures_;
    std::unordered_map<std::string, int32_t> namedGroups_;
};

class NativeRegex {
public:
    static doof::Result<std::shared_ptr<NativeRegex>, std::string> compile(
        const std::string& pattern,
        bool ignoreCase,
        bool multiline,
        bool dotAll,
        bool extended
    ) {
        uint32_t options = 0;
        if (ignoreCase) options |= PCRE2_CASELESS;
        if (multiline) options |= PCRE2_MULTILINE;
        if (dotAll) options |= PCRE2_DOTALL;
        if (extended) options |= PCRE2_EXTENDED;

        int errorCode = 0;
        PCRE2_SIZE errorOffset = 0;
        pcre2_code* code = pcre2_compile(
            reinterpret_cast<PCRE2_SPTR>(pattern.data()),
            static_cast<PCRE2_SIZE>(pattern.size()),
            options,
            &errorCode,
            &errorOffset,
            nullptr
        );
        if (code == nullptr) {
            return doof::Result<std::shared_ptr<NativeRegex>, std::string>::failure(
                formatCompileError(errorCode, errorOffset)
            );
        }

        uint32_t captureCount = 0;
        if (pcre2_pattern_info(code, PCRE2_INFO_CAPTURECOUNT, &captureCount) != 0) {
            pcre2_code_free(code);
            return doof::Result<std::shared_ptr<NativeRegex>, std::string>::failure(
                "Failed to inspect compiled regex"
            );
        }

        auto namedGroups = extractNamedGroups(code);

        return doof::Result<std::shared_ptr<NativeRegex>, std::string>::success(
            std::shared_ptr<NativeRegex>(new NativeRegex(code, captureCount, std::move(namedGroups)))
        );
    }

    ~NativeRegex() {
        if (code_ != nullptr) {
            pcre2_code_free(code_);
        }
    }

    bool test(const std::string& input) const {
        MatchDataPtr matchData = createMatchData();
        const int rc = pcre2_match(
            code_,
            reinterpret_cast<PCRE2_SPTR>(input.data()),
            static_cast<PCRE2_SIZE>(input.size()),
            0,
            0,
            matchData.get(),
            nullptr
        );
        if (rc == PCRE2_ERROR_NOMATCH) {
            return false;
        }
        if (rc < 0) {
            doof::panic("Regex match failed: " + errorMessage(rc));
        }
        return true;
    }

    std::shared_ptr<NativeRegexMatch> find(const std::string& input, int32_t startOffset) const {
        if (startOffset < 0 || startOffset > static_cast<int32_t>(input.size())) {
            return noMatch();
        }

        MatchDataPtr matchData = createMatchData();
        const int rc = pcre2_match(
            code_,
            reinterpret_cast<PCRE2_SPTR>(input.data()),
            static_cast<PCRE2_SIZE>(input.size()),
            static_cast<PCRE2_SIZE>(startOffset),
            0,
            matchData.get(),
            nullptr
        );
        if (rc == PCRE2_ERROR_NOMATCH) {
            return noMatch();
        }
        if (rc < 0) {
            doof::panic("Regex match failed: " + errorMessage(rc));
        }

        PCRE2_SIZE* ovector = pcre2_get_ovector_pointer(matchData.get());
        const int32_t valueStart = toInt32(ovector[0]);
        const int32_t valueEnd = toInt32(ovector[1]);

        std::vector<NativeRegexCapture> captures;
        captures.reserve(captureCount_);
        for (uint32_t index = 1; index <= captureCount_; ++index) {
            const PCRE2_SIZE captureStart = ovector[index * 2];
            const PCRE2_SIZE captureEnd = ovector[index * 2 + 1];
            if (captureStart == PCRE2_UNSET || captureEnd == PCRE2_UNSET) {
                captures.push_back(NativeRegexCapture{false, "", -1, -1});
                continue;
            }

            const int32_t start = toInt32(captureStart);
            const int32_t end = toInt32(captureEnd);
            captures.push_back(NativeRegexCapture{
                true,
                input.substr(static_cast<std::size_t>(start), static_cast<std::size_t>(end - start)),
                start,
                end,
            });
        }

        return std::make_shared<NativeRegexMatch>(
            true,
            input.substr(static_cast<std::size_t>(valueStart), static_cast<std::size_t>(valueEnd - valueStart)),
            valueStart,
            valueEnd,
            std::move(captures),
            namedGroups_
        );
    }

    std::string replaceFirst(const std::string& input, const std::string& replacement) const {
        return substitute(input, replacement, false);
    }

    std::string replaceAll(const std::string& input, const std::string& replacement) const {
        return substitute(input, replacement, true);
    }

private:
    using MatchDataPtr = std::unique_ptr<pcre2_match_data, decltype(&pcre2_match_data_free)>;

    NativeRegex(pcre2_code* code, uint32_t captureCount, std::unordered_map<std::string, int32_t> namedGroups)
        : code_(code), captureCount_(captureCount), namedGroups_(std::move(namedGroups)) {}

    static int32_t toInt32(PCRE2_SIZE value) {
        return value > static_cast<PCRE2_SIZE>(INT32_MAX)
            ? INT32_MAX
            : static_cast<int32_t>(value);
    }

    static std::string errorMessage(int errorCode) {
        PCRE2_UCHAR buffer[256];
        const int length = pcre2_get_error_message(errorCode, buffer, sizeof(buffer));
        if (length < 0) {
            return "PCRE2 error " + std::to_string(errorCode);
        }
        return std::string(reinterpret_cast<const char*>(buffer), static_cast<std::size_t>(length));
    }

    static std::string formatCompileError(int errorCode, PCRE2_SIZE errorOffset) {
        return errorMessage(errorCode) + " at byte " + std::to_string(static_cast<std::size_t>(errorOffset));
    }

    static std::unordered_map<std::string, int32_t> extractNamedGroups(pcre2_code* code) {
        uint32_t nameCount = 0;
        if (pcre2_pattern_info(code, PCRE2_INFO_NAMECOUNT, &nameCount) != 0 || nameCount == 0) {
            return {};
        }

        uint32_t entrySize = 0;
        PCRE2_SPTR nameTable = nullptr;
        if (pcre2_pattern_info(code, PCRE2_INFO_NAMEENTRYSIZE, &entrySize) != 0) {
            return {};
        }
        if (pcre2_pattern_info(code, PCRE2_INFO_NAMETABLE, &nameTable) != 0 || nameTable == nullptr) {
            return {};
        }

        std::unordered_map<std::string, int32_t> namedGroups;
        namedGroups.reserve(nameCount);
        for (uint32_t index = 0; index < nameCount; ++index) {
          const PCRE2_SPTR entry = nameTable + (index * entrySize);
          const uint16_t groupNumber = static_cast<uint16_t>((entry[0] << 8) | entry[1]);
          const char* groupName = reinterpret_cast<const char*>(entry + 2);
          namedGroups.emplace(groupName, static_cast<int32_t>(groupNumber));
        }

        return namedGroups;
    }

    MatchDataPtr createMatchData() const {
        pcre2_match_data* matchData = pcre2_match_data_create_from_pattern(code_, nullptr);
        if (matchData == nullptr) {
            doof::panic("Failed to allocate regex match data");
        }
        return MatchDataPtr(matchData, &pcre2_match_data_free);
    }

    std::string substitute(const std::string& input, const std::string& replacement, bool global) const {
        MatchDataPtr matchData = createMatchData();
        uint32_t options = PCRE2_SUBSTITUTE_UNSET_EMPTY | PCRE2_SUBSTITUTE_OVERFLOW_LENGTH;
        if (global) {
            options |= PCRE2_SUBSTITUTE_GLOBAL;
        }

        PCRE2_SIZE outputLength = input.size() + replacement.size() + 32;
        std::vector<PCRE2_UCHAR> output(outputLength == 0 ? 1 : outputLength);

        while (true) {
            PCRE2_SIZE actualLength = output.size();
            const int rc = pcre2_substitute(
                code_,
                reinterpret_cast<PCRE2_SPTR>(input.data()),
                static_cast<PCRE2_SIZE>(input.size()),
                0,
                options,
                matchData.get(),
                nullptr,
                reinterpret_cast<PCRE2_SPTR>(replacement.data()),
                static_cast<PCRE2_SIZE>(replacement.size()),
                output.data(),
                &actualLength
            );

            if (rc == PCRE2_ERROR_NOMEMORY) {
                output.resize(static_cast<std::size_t>(actualLength));
                continue;
            }
            if (rc < 0) {
                doof::panic("Regex replacement failed: " + errorMessage(rc));
            }

            return std::string(reinterpret_cast<const char*>(output.data()), static_cast<std::size_t>(actualLength));
        }
    }

    std::shared_ptr<NativeRegexMatch> noMatch() const {
        return std::make_shared<NativeRegexMatch>(
            false,
            "",
            -1,
            -1,
            std::vector<NativeRegexCapture>{},
            namedGroups_
        );
    }

    pcre2_code* code_;
    uint32_t captureCount_;
    std::unordered_map<std::string, int32_t> namedGroups_;
};