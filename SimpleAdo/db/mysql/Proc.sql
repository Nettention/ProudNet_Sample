DELIMITER $$

USE `ProudAdo-Test`$$

DROP PROCEDURE IF EXISTS `AddUserData`$$

CREATE DEFINER=`proud`@`localhost` PROCEDURE `AddUserData`(
	IN InUserID VARCHAR(50),
	IN InPassword VARCHAR(50),
	IN InCountry INT
	)
BEGIN
	IF (SELECT COUNT(UserID) FROM UserData WHERE UserID=InUserID <> 0) THEN
		SELECT -1;
	ELSE
	    	INSERT INTO UserData(UserID,PASSWORD,Country) VALUES(InUserID,InPassword,InCountry);
		SELECT 0;
	END IF;
END$$

DROP PROCEDURE IF EXISTS `GetUserData`$$

CREATE DEFINER=`proud`@`localhost` PROCEDURE `GetUserData`(
	IN InUserID varchar(50)
)
BEGIN
	SELECT UserID, Password, Country FROM UserData WHERE UserID = InUserID;
END$$

DELIMITER ;
