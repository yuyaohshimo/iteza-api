-- name: new-token!
INSERT INTO tokens (token) VALUES (:token);

-- name: verify-token
SELECT * FROM tokens
WHERE token = :token AND DATEDIFF("HOUR",create_timestamp,CURRENT_TIMESTAMP) < :hours;

-- name: get-tokens
-- for debug
SELECT * FROM tokens;

-- name: purge-tokens!
-- for debug
DELETE FROM tokens;
