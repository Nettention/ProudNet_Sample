
DELIMITER $$

CREATE DATABASE `ProudAdo-Test`$$

USE `ProudAdo-Test`$$

CREATE TABLE `UserData`(
	`UserID` VARCHAR(50) NOT NULL,
	`Password` VARCHAR(50) NULL,
	`Country` INT NULL,
	PRIMARY KEY (`UserID`)
)$$

DELIMITER ;
