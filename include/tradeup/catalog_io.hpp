#pragma once

#include "tradeup/reference.hpp"

#include <filesystem>
#include <map>

namespace tradeup {
[[nodiscard]] std::string sha256(std::string_view bytes);
[[nodiscard]] std::filesystem::path utf8_path(std::string_view text);
[[nodiscard]] std::string path_text(const std::filesystem::path& path);
[[nodiscard]] std::string read_local_file(const std::filesystem::path& path, std::size_t max_bytes = 8 * 1024 * 1024);
[[nodiscard]] std::vector<std::string> split_fields(std::string_view line, char separator = '\t');
[[nodiscard]] std::string_view quote_kind_name(QuoteKind kind);
[[nodiscard]] QuoteKind parse_quote_kind(std::string_view kind);

struct LoadedCatalog {
    Dataset dataset;
    std::string source_path, source_format, source_sha256, content_sha256;
    std::string scenario_label, mapping_version;
    std::string source_binding;
    bool authentic_preserved_values = false;
    std::map<std::string, std::string> collection_labels, bucket_labels;
    std::vector<Rational> matrix; // Original row-major values, including all structural padding.
};
void finalize_identity(LoadedCatalog& catalog, std::string_view source_binding = {});
[[nodiscard]] LoadedCatalog load_catalog_v1(const std::filesystem::path& path);
} // namespace tradeup
