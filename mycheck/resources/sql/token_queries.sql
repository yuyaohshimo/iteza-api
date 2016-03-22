-- name: new-token!
INSERT INTO tokens (account_id) VALUES (:account_id);

-- name: set-token!
UPDATE tokens SET token = :token,create_timestamp = CURRENT_TIMESTAMP WHERE account_id = :account_id;

-- name: verify-token
SELECT * FROM tokens
WHERE token = :token AND DATEDIFF("HOUR",create_timestamp,CURRENT_TIMESTAMP) < :hours;

-- name: delete-token!
DELETE FROM tokens WHERE account_id = :account_id;

-- name: get-token
SELECT * FROM tokens WHERE account_id = :account_id;

-- name: get-tokens
-- for debug
SELECT * FROM tokens;

-- name: purge-tokens!
-- for debug
DELETE FROM tokens;
