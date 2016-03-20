-- name: new-token!
INSERT INTO tokens (token) VALUES (:token);

-- name: verify-token
SELECT * FROM tokens
WHERE token = :token AND DATEDIFF("HOUR",create_timestamp,CURRENT_TIMESTAMP) < :hours;

-- name: purge-tokens!
DELETE FROM tokens;
