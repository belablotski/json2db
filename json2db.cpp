#include <iostream>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>    // sudo apt-get install nlohmann-json3-dev
#include <string>
#include <vector>
#include <pqxx/pqxx>            // sudo apt-get install libpqxx-dev
#include <unordered_map>
#include <memory>

struct Mapping {
    std::string description;
    std::string source;
    std::string destination_table;
    std::string id_expr;
    std::string connection;
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
                mapping.contains("description") ? mapping["description"].get<std::string>() : "",
                mapping["source"].get<std::string>(),
                mapping["destination_table"].get<std::string>(),
                mapping["id_expr"].get<std::string>(),
                mapping["connection"].get<std::string>()
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

class Session {
public:
    explicit Session(const std::string& connectionString)
        : _connection(connectionString) {
        std::cout << "Connected to PostgreSQL database " << connectionString << std::endl;
    }

    ~Session() {
        std::cout << "Closing PostgreSQL connection." << std::endl;
        _connection.disconnect();
    }

    pqxx::result executeQuery(const std::string& query) {
        try {
            pqxx::work transaction(_connection);
            pqxx::result result = transaction.exec(query);
            transaction.commit();
            return result;
        } catch (const std::exception& e) {
            throw std::runtime_error("Database query failed: " + std::string(e.what()));
        }
    }

private:
    pqxx::connection _connection;
};

class ConnectionFactory {
public:
    explicit ConnectionFactory() : _sessionCache() {}

    std::shared_ptr<Session> getSession(const std::string& connectionString) {
        auto it = _sessionCache.find(connectionString);
        if (it != _sessionCache.end()) {
            std::cout << "Reusing existing session for connection string: " << connectionString << std::endl;
            return it->second;
        }

        std::cout << "Creating new session for connection string: " << connectionString << std::endl;
        auto session = std::make_shared<Session>(connectionString);
        _sessionCache[connectionString] = session;
        return session;
    }

private:
    std::unordered_map<std::string, std::shared_ptr<Session>> _sessionCache;
};

class Loader {
public:
    explicit Loader(const std::vector<Mapping>& mappings, ConnectionFactory& connectionFactory)
        : _mappings(mappings), _connectionFactory(connectionFactory) {}

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
    int load(const std::string& loadId) {
        std::cout << "Starting data loading process " << loadId << " for " << _mappings.size() << " mappings." << std::endl;

        // TODO: this makes it not thread safe.
        _loadId = loadId;

        int mappingIndex = 0;
        int processedFilesCount = 0;

        for (const auto& mapping : _mappings) {
            std::cout << "*** Processing mapping " << ++mappingIndex << " of " << _mappings.size() << std::endl
                      << "Description: " << mapping.description << std::endl
                      << "Loading data from '" << mapping.source
                      << "' into table '" << mapping.destination_table << "' in '" << mapping.connection
                      << "' with id expression '" << mapping.id_expr << "'" << std::endl;

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

        std::string jsonStr = jsonData.dump();

        std::string hash = std::to_string(std::hash<std::string>{}(jsonStr));
        std::string load_id = _loadId;
        std::string created_at = "NOW()";
        std::string updated_at = "NOW()";

        std::shared_ptr<Session> session = _connectionFactory.getSession(mapping.connection);
        
        size_t pos = 0;
        while ((pos = jsonStr.find("'", pos)) != std::string::npos) {
            jsonStr.replace(pos, 1, "''");
            pos += 2;
        }
        
        // TODO: Use prepared statements to prevent SQL injection
        std::string query = "INSERT INTO " + mapping.destination_table + 
                            " (id, data, hash, load_id, created_at, updated_at) VALUES ('" + 
                            id + "', '" + jsonStr + "', '" + hash + "', '" + load_id + "', " + created_at + ", " + updated_at + ")";
        session->executeQuery(query);
        std::cout << "Data saved successfully to table: " << mapping.destination_table << std::endl;
    }

private:
    std::vector<Mapping> _mappings;
    ConnectionFactory& _connectionFactory;
    std::string _loadId;
};

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <mapping_file_path> <load id>" << std::endl;
        return 1;
    }

    std::filesystem::path mappingFilePath = argv[1];
    std::string loadId = argv[2];

    if (!std::filesystem::exists(mappingFilePath)) {
        std::cerr << "Error: The specified mapping file does not exist." << std::endl;
        return 1;
    }

    try {
        std::cout << "Starting load session " << loadId << "..." << std::endl;
        MappingParser parser(mappingFilePath.string());
        nlohmann::json jsonData = parser.parse();

        Mappings mappings(jsonData);

        ConnectionFactory connectionFactory;
        Loader loader(mappings.getMappings(), connectionFactory);
        int fileCount = loader.load(loadId);
        std::cout << "Total files processed in load session " << loadId << ": " << fileCount << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error in load session " << loadId << ": " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
