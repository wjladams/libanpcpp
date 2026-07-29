/**
 * @file json_io.hpp
 * @brief JSON serialization for ANP networks (format v1).
 */

#pragma once

#include <memory>
#include <stdexcept>
#include <string>

#include "anpcpp/network.hpp"

namespace anpcpp {

/**
 * @brief Thrown on JSON parse, validation, or I/O errors.
 */
class JsonIoError : public std::runtime_error {
public:
  /**
   * @param message Description of the error.
   */
  explicit JsonIoError(const std::string& message)
      : std::runtime_error(message) {}
};

/**
 * @brief Serializes a network (including subnetworks and layout hints) to JSON.
 * @param network Network to serialize.
 * @return JSON text (anpcpp format v1).
 */
[[nodiscard]] std::string network_to_json(const AnpNetwork& network);

/**
 * @brief Parses JSON produced by @ref network_to_json.
 * @param json_text JSON document.
 * @return Owned network tree.
 * @throws JsonIoError on invalid or unsupported documents.
 */
[[nodiscard]] std::unique_ptr<AnpNetwork> network_from_json(
    const std::string& json_text);

/**
 * @brief Writes a network to a file.
 * @param network Network to save.
 * @param path Output file path.
 * @throws JsonIoError on write failure.
 */
void save_network_file(const AnpNetwork& network, const std::string& path);

/**
 * @brief Loads a network from a file.
 * @param path Input file path.
 * @return Owned network tree.
 * @throws JsonIoError on read or parse failure.
 */
[[nodiscard]] std::unique_ptr<AnpNetwork> load_network_file(
    const std::string& path);

}  // namespace anpcpp
