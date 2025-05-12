CREATE TABLE objects (
    id TEXT PRIMARY KEY,
    data JSONB NOT NULL,
    hash TEXT NOT NULL,
    load_id TEXT NOT NULL,
    created_at TIMESTAMP NOT NULL,
    updated_at TIMESTAMP NOT NULL
);

CREATE USER test_user WITH PASSWORD 'test_password';

GRANT ALL PRIVILEGES ON TABLE objects TO test_user;
