#include <iostream>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>    // sudo apt-get install nlohmann-json3-dev
#include <string>
#include <vector>

struct Mapping {
    std::string source;
    std::string destination_table;
    std::string id_expr;
};

class Mappings {
public:
    explicit Mappings(const nlohmann::json& jsonData) {
        if (jsonData.is_null() || !jsonData.contains("mappings")) {
            throw std::runtime_error("Invalid JSON data: missing 'mappings' key");
        }
        for (const auto& mapping : jsonData["mappings"]) {
            std::cout << "Parsing mapping: " << mapping.dump() << std::endl;

            Mapping m = Mapping {
                mapping["source"].get<std::string>(),
                mapping["destination_table"].get<std::string>(),
                mapping["id_expr"].get<std::string>()
            };

            if (m.source.empty() || m.destination_table.empty()) {
                throw std::runtime_error("Invalid mapping data: source or destination_table is empty");
            }

            _mappings.emplace_back(m);
        }
    }

    const std::vector<Mapping>& getMappings() const {
        return _mappings;
    }

private:
    std::vector<Mapping> _mappings;
};

class MappingParser {
public:
    explicit MappingParser(const std::string& filePath) : _filePath(filePath) {}

    nlohmann::json parse() {
        std::ifstream file(_filePath);
        if (!file.is_open()) {
            throw std::runtime_error("Unable to open mapping file: " + _filePath);
        }

        std::cout << "Parsing mapping file: " << _filePath << std::endl;
        nlohmann::json jsonData;
        file >> jsonData;
        if (jsonData.is_null()) {
            throw std::runtime_error("Failed to parse JSON data from file: " + _filePath);
        }
        std::cout << "Successfully parsed mapping data." << std::endl;
        return jsonData;
    }

private:
    std::string _filePath;
};

class Loader {
public:
    explicit Loader(const std::vector<Mapping>& mappings) : _mappings(mappings) {}

    /**
     * @brief Loads data from the specified mappings into the database.
     * 
     * This function iterates over the provided mappings, processes the source paths,
     * and loads the data into the corresponding database tables. It supports both
     * directories and individual files as source paths. If the source path is a directory,
     * all regular files within the directory are processed. If the source path is a single
     * file, only that file is processed.
     * 
     * @return int The number of processed files.
     * 
     * @throws std::runtime_error If the source path does not exist, is of an unknown type,
     *                            or if there is an error opening or processing a file.
     */
    int load() {
        std::cout << "Starting data loading process for " << _mappings.size() << " mappings." << std::endl;

        int mappingIndex = 0;
        int processedFilesCount = 0;

        for (const auto& mapping : _mappings) {
            std::cout << "Processing mapping " << ++mappingIndex << " of " << _mappings.size() << std::endl
                      << "Loading data from " << mapping.source
                      << " into table " << mapping.destination_table
                      << " id expression " << mapping.id_expr << std::endl;

            std::filesystem::path sourcePath = mapping.source;

            if (!std::filesystem::exists(sourcePath)) {
                throw std::runtime_error("Source path does not exist: " + sourcePath.string());
            }

            if (std::filesystem::is_directory(sourcePath)) {
                int fileCount = std::distance(std::filesystem::directory_iterator(sourcePath), std::filesystem::directory_iterator{});
                std::cout << "Processing directory: " << sourcePath.filename().string() << " with " << fileCount << " files" << std::endl;
                int fileIndex = 0;
                for (const auto& entry : std::filesystem::directory_iterator(sourcePath)) {
                    if (std::filesystem::is_regular_file(entry)) {
                        std::cout << "Processing file " << ++fileIndex << " of " << fileCount << ": " << entry.path().filename().string() << std::endl;
                        loadFile(entry, mapping);
                    }
                }
                processedFilesCount += fileCount;
            } else if (std::filesystem::is_regular_file(sourcePath)) {
                loadFile(sourcePath, mapping);
                processedFilesCount++;
            } else {
                throw std::runtime_error("Unknown file system object type: " + sourcePath.string());
            }
        }

        return processedFilesCount;
    }

protected:
    void loadFile(const std::filesystem::path& filePath, const Mapping& mapping) {
        std::cout << "Loading file: " << filePath.filename().string() << " for mapping: " << mapping.destination_table << std::endl;
        if (filePath.extension() != ".json") {
            std::cerr << "Warning: The file " << filePath.filename().string() 
                      << " does not have a .json extension. Skipping it..." << std::endl;
            return;
        }
        std::ifstream file(filePath);
        if (!file.is_open()) {
            throw std::runtime_error("Unable to open file: " + filePath.string());
        }

        nlohmann::json jsonData;
        file >> jsonData;

        if (jsonData.is_object()) {
            saveData(filePath, mapping, jsonData);
        } else if (jsonData.is_array()) {
            int elementCount = jsonData.size();
            std::cout << "Parsed JSON array with " << elementCount << " elements:" << std::endl;
            int elementIndex = 0;
            for (const auto& element : jsonData) {
                std::cout << "Processing element " << ++elementIndex << " of " << elementCount << std::endl;
                saveData(filePath, mapping, element);
            }
        } else {
            throw std::runtime_error("Invalid JSON data: Expected object or array in file: " + filePath.string());
        }
    }

    std::string generateId(const std::string& idExpr, const std::filesystem::path& filePath, const nlohmann::json& jsonData) {
        std::string id = idExpr;
        size_t startPos = 0;

        while ((startPos = id.find("${", startPos)) != std::string::npos) {
            size_t endPos = id.find("}", startPos);

            if (endPos == std::string::npos) {
                throw std::runtime_error("Invalid id expression: missing closing '}' in " + idExpr);
            }

            std::string key = id.substr(startPos + 2, endPos - startPos - 2);

            if (jsonData.contains(key)) {
                if (!jsonData[key].is_string() && !jsonData[key].is_number_integer()) {
                    throw std::runtime_error("Invalid value type for key '" + key + "' in JSON data. Expected string or integer.");
                }
                std::string value = jsonData[key].is_string() ? jsonData[key].get<std::string>() : std::to_string(jsonData[key].get<long>());
                id.replace(startPos, endPos - startPos + 1, value);
                startPos += value.length();
            } else {
                throw std::runtime_error("Key '" + key + "' not found in JSON data for id expression: " + idExpr);
            }
        }

        return id;
    }

    void saveData(const std::filesystem::path& filePath, const Mapping& mapping, const nlohmann::json& jsonData) {
        std::cout << "Saving data from file: " << filePath.filename().string() << " to table: " << mapping.destination_table << std::endl;
        std::string id = generateId(mapping.id_expr, filePath, jsonData);
        std::cout << "Generated ID: " << id << std::endl;
        std::cout << "Data: " << jsonData.dump(4) << std::endl;
        // Placeholder for actual database saving logic
    }

private:
    std::vector<Mapping> _mappings;
};

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <mapping_file_path>" << std::endl;
        return 1;
    }

    std::filesystem::path mappingFilePath = argv[1];

    if (!std::filesystem::exists(mappingFilePath)) {
        std::cerr << "Error: The specified mapping file does not exist." << std::endl;
        return 1;
    }

    try {
        MappingParser parser(mappingFilePath.string());
        nlohmann::json jsonData = parser.parse();

        Mappings mappings(jsonData);

        Loader loader(mappings.getMappings());
        int fileCount = loader.load();
        std::cout << "Total files processed: " << fileCount << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
