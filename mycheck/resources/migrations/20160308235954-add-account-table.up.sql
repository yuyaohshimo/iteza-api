CREATE TABLE accounts
(
    id IDENTITY,
    user_id VARCHAR(100) NOT NULL UNIQUE,
    password VARCHAR(512) NOT NULL,
    user_name VARCHAR(100) NOT NULL,
    account_id VARCHAR(20) NOT NULL,
    phone_number VARCHAR(20),
    balance INT NOT NULL
);
