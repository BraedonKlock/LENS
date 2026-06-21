-- Run once against your MySQL database to upgrade the LENS schema

ALTER TABLE stores ADD COLUMN store_name   VARCHAR(255) NOT NULL DEFAULT '';
ALTER TABLE stores ADD COLUMN store_number VARCHAR(50)  NOT NULL DEFAULT '';

CREATE TABLE IF NOT EXISTS servers (
  id         VARCHAR(50) PRIMARY KEY,
  in_use     TINYINT(1)  NOT NULL DEFAULT 0,
  store_id   VARCHAR(36) NULL,
  created_at TIMESTAMP   NOT NULL DEFAULT CURRENT_TIMESTAMP,
  FOREIGN KEY (store_id) REFERENCES stores(id)
);
