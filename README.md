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

## TODO
1. Besides of object ID, add hash, load session id, created_at/updated_at/deleted_at timestamps.
2. Data load strategy: insert or upsert.
3. Implement more EL substitutions in id generation, like ${__dirName}, ${__fileName}, ${__UUID}, etc.
4. Generalize Session class from PostgreSQL to arbitrary DB.
5. Password management for DB connections (externalize password from config file - allow to use ${password1} placeholders and pass passwords via command line or environment variables).
6. Use prepared statements to prevent SQL injection.

## See also
1. [My First Steps into Agentic Coding with VS Code](https://belablotski.github.io/engineering/ai/agentic_coding/first_steps/)
