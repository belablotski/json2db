brew install postgresql

# Start PostgreSQL service
brew services start postgresql@14

# Or start PostgreSQL server manually
# /opt/homebrew/opt/postgresql@14/bin/postgres -D /opt/homebrew/var/postgresql@14

# Verify PostgreSQL installation
# psql postgres
