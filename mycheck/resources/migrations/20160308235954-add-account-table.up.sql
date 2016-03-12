CREATE TABLE accounts
(
    id IDENTITY,
    user_id VARCHAR(20) NOT NULL UNIQUE,
    user_name VARCHAR(100) NOT NULL,
    account_id VARCHAR(20) NOT NULL,
    phone_number VARCHAR(20),
    balance INT NOT NULL
);
