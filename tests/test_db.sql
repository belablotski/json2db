CREATE TABLE objects (
    id TEXT PRIMARY KEY,
    data JSONB NOT NULL,
    hash TEXT NOT NULL,
    load_id TEXT NOT NULL,
    created_at TIMESTAMP NOT NULL,
    updated_at TIMESTAMP NOT NULL
);
