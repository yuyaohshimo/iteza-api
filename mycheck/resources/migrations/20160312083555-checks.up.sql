CREATE TABLE checks
(
  id VARCHAR(14) PRIMARY KEY,
  account_id VARCHAR(10) NOT NULL,
  token VARCHAR(6) NOT NULL,
  acc_token VARCHAR(255) NOT NULL,
  amount INT NOT NULL,
  status INT NOT NULL,
  dest VARCHAR(255),
  dest_acc_id VARCHAR(20)
);
