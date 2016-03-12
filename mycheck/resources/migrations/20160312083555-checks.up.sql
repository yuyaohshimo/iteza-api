CREATE TABLE checks
(
  id LONG UNIQUE NOT NULL,
  token INT NOT NULL,
  acc_token varchar(255) NOT NULL,
  amount INT NOT NULL,
  status INT NOT NULL,
  dest VARCHAR(255),
  dest_acc_id VARCHAR(20)
);
