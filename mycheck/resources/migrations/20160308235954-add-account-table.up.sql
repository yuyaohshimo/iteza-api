CREATE TABLE accounts
(
    id IDENTITY,
    bk_user_id VARCHAR(20) NOT NULL UNIQUE,
    bk_user_name VARCHAR(100) NOT NULL,
    bk_account_id VARCHAR(20) NOT NULL,
    balance INT NOT NULL
);
