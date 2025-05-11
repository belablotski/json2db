# JSON2DB

## Overview
`JSON2DB` is a C++ application designed to parse JSON files and load their data into a database. It processes mappings defined in a JSON configuration file, iterates through directories or files, and saves the data to the specified database tables.

## Features
- Parses JSON configuration files to define mappings.
- Processes directories and individual JSON files.
- Supports JSON objects and arrays.

## Prerequisites
- C++17 or later.
- [nlohmann/json](https://github.com/nlohmann/json) library installed.

## Installation
1. Clone the repository:
   ```bash
   git clone <repository-url>
   cd json2db
   ```
2. Install dependencies:
   ```bash
   sudo apt-get install nlohmann-json3-dev
   ```

## Build
Use the provided VS Code task or run the following command:
```bash
g++ -g json2db.cpp -o json2db
```

## Run
Execute the application with a JSON mapping file:
```bash
./json2db json2db.json
```

## JSON Configuration
The configuration file should define mappings in the following format:
```json
{
    "mappings": [
        {
            "source": "<source_directory_or_file>",
            "destination_table": "<database_table_name>"
        }
    ]
}
```

## Example
Sample `json2db.json`:
```json
{
    "mappings": [
        {
            "source": "./tests/objects",
            "destination_table": "objects"
        },
        {
            "source": "./tests/arrays",
            "destination_table": "arrays"
        }
    ]
}
```

## TODO
1. Besides of object ID, add hash, load session id, created_at/updated_at/deleted_at timestamps.
2. Data load strategy: insert or upsert.