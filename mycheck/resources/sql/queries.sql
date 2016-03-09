-- name: create-accounts!
-- creates a new check account
INSERT INTO accounts
(bk_user_id, bk_user_name, bk_account_id, balance)
VALUES (:user_id, :user_name, :account_id, :balance)

-- name: update-balance!
-- update check
UPDATE accounts
SET balance = :balance
WHERE id = :id

-- name: get-account-by-user
-- retrieve a account by bk_user_id
SELECT * FROM accounts
WHERE bk_user_id = :user_id
