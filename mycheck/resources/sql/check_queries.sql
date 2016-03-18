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

-- name: ready-check!
-- update status ready to authorize
-- status 3: READY
UPDATE checks
SET status = 3
WHERE dest = :dest and status = 2

-- name: done-check!
-- update status complete
-- satus 4: COMPLETE
UPDATE checks
SET status = 4
WHERE id = :id

-- name: get-checks
SELECT * FROM checks;

-- name: get-checks-by-status
SELECT * FROM checks WHERE status = :status;

-- name: get-check-by-dest
SELECT * FROM checks
WHERE dest = :dest and status = :status

-- name: delete-check!
-- for debug
DELETE from checks WHERE id = :id;
