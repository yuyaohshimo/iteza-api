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

-- name: create-check!
-- create a check. status = 0
INSERT INTO checks
(id, account_id, token, acc_token, amount, status)
VALUES (:id,:account_id, :token,:acc_token, :amount, 0)

-- name: get-check-by-key
-- get check by id and token
SELECT * FROM checks
WHERE id = :id

-- name: receive-check!
-- receive a check. change status by id
-- status 1: ISSUED
UPDATE checks
SET status = 1, dest = :dest
WHERE id = :id and status = 0

-- name: confirm-check!
-- update a check with dest and dest_acc_id
-- status 2: CONFIRMING
UPDATE checks
SET status = 2, dest_acc_id = :acc_id
WHERE dest = :dest and status = 1

-- name: done-check!
-- update status complete
-- status 3: DONE
UPDATE checks
SET status = 3
WHERE dest = :dest and status = 2

-- name: get-checks
SELECT * FROM checks;

-- name: get-checks-by-status
SELECT * FROM checks WHERE status = :status;

-- name: get-check-for-confirm
SELECT * FROM checks
WHERE dest = :dest and status = 2

-- name: delete-check!
-- for debug
DELETE from checks WHERE id = :id;
