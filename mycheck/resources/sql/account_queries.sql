-- name: create-accounts!
-- create a new check account
INSERT INTO accounts
(user_id, user_name, account_id, phone_number, balance)
VALUES (:user_id, :user_name, :account_id, :phone_number, :balance)

-- name: update-balance!
-- update a check
UPDATE accounts
SET balance = :balance
WHERE id = :id

-- name: get-account-by-user
-- retrieve a account by bk_user_id
SELECT * FROM accounts
WHERE user_id = :user_id

-- name: get-account-by-accid
-- retrieve a account by account_id
SELECT * FROM accounts
WHERE account_id = :account_id

-- name: get-accounts
-- retrieve all accounts for debug
SELECT * FROM accounts
