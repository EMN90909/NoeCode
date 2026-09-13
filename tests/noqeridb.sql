CREATE TABLE users (
    id INT PRIMARY KEY,
    name TEXT NOT NULL,
    active BOOL NOT NULL
);

INSERT INTO users (id, name, active) VALUES (2, 'Linus', true);
INSERT INTO users (id, name, active) VALUES (1, 'Ada', true);
UPDATE users SET name = 'Ada Lovelace' WHERE id = 1;
SELECT id, name FROM users WHERE active = true ORDER BY id ASC;
