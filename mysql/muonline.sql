/*
SQLyog Community v13.1.7 (64 bit)
MySQL - 8.0.21 : Database - emu
*********************************************************************
*/

/*!40101 SET NAMES utf8 */;

/*!40101 SET SQL_MODE=''*/;

/*!40014 SET @OLD_UNIQUE_CHECKS=@@UNIQUE_CHECKS, UNIQUE_CHECKS=0 */;
/*!40014 SET @OLD_FOREIGN_KEY_CHECKS=@@FOREIGN_KEY_CHECKS, FOREIGN_KEY_CHECKS=0 */;
/*!40101 SET @OLD_SQL_MODE=@@SQL_MODE, SQL_MODE='NO_AUTO_VALUE_ON_ZERO' */;
/*!40111 SET @OLD_SQL_NOTES=@@SQL_NOTES, SQL_NOTES=0 */;
CREATE DATABASE /*!32312 IF NOT EXISTS*/`emu` /*!40100 DEFAULT CHARACTER SET latin1 */ /*!80016 DEFAULT ENCRYPTION='N' */;

USE `emu`;

/*Table structure for table `accounts` */

DROP TABLE IF EXISTS `accounts`;

CREATE TABLE `accounts` (
  `id` varchar(11) NOT NULL DEFAULT '',
  `password` varchar(11) NOT NULL DEFAULT '',
  `status` tinyint(1) unsigned zerofill DEFAULT NULL,
  `ban` tinyint(1) unsigned zerofill DEFAULT NULL,
  `pin` varchar(8) NOT NULL,
  `ipAddress` varchar(17) DEFAULT NULL,
  `loginDate` datetime DEFAULT NULL,
  `logoutDate` datetime DEFAULT NULL,
  UNIQUE KEY `account_id` (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

/*Data for the table `accounts` */

/*Table structure for table `characters` */

DROP TABLE IF EXISTS `characters`;

CREATE TABLE `characters` (
  `created` timestamp NULL DEFAULT CURRENT_TIMESTAMP,
  `accountId` varchar(11) DEFAULT '',
  `name` varchar(11) NOT NULL DEFAULT '',
  `race` tinyint unsigned DEFAULT '0',
  `level` smallint unsigned NOT NULL DEFAULT '1',
  `levelUpPoints` smallint unsigned DEFAULT '0',
  `experience` int unsigned DEFAULT '0',
  `strength` smallint unsigned DEFAULT NULL,
  `agility` smallint unsigned DEFAULT NULL,
  `vitality` smallint unsigned DEFAULT NULL,
  `energy` smallint unsigned DEFAULT NULL,
  `command` smallint unsigned DEFAULT NULL,
  `money` int unsigned DEFAULT '0',
  `health` int DEFAULT NULL,
  `maxHealth` int DEFAULT NULL,
  `mana` int DEFAULT NULL,
  `maxMana` int DEFAULT NULL,
  `mapId` tinyint unsigned DEFAULT NULL,
  `posX` tinyint unsigned DEFAULT NULL,
  `posY` tinyint unsigned DEFAULT NULL,
  `direction` tinyint(1) DEFAULT NULL,
  `controlCode` tinyint unsigned DEFAULT '0',
  UNIQUE KEY `name` (`name`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

/*Data for the table `characters` */

/* Function  structure for function  `eMU_AccountCheck` */

/*!50003 DROP FUNCTION IF EXISTS `eMU_AccountCheck` */;
DELIMITER $$

/*!50003 CREATE DEFINER=`root`@`localhost` FUNCTION `eMU_AccountCheck`(
        _id VARCHAR(11),
        _password VARCHAR(11),
        _ipAddress VARCHAR(16)
    ) RETURNS tinyint
BEGIN
	DECLARE `exists_` TINYINT;
    DECLARE `passwordCheck_` TINYINT;
    DECLARE `status_` TINYINT;
    DECLARE `ban_` TINYINT;
    
    SELECT
    	COUNT(*)
    INTO
    	exists_ 
    FROM
    	`accounts`
    WHERE
    	`id` = `_id`;
    
    IF(exists_ > 0)
    THEN
    	SELECT
        	COUNT(*)
        INTO
        	passwordCheck_
        FROM
        	`accounts`
        WHERE
        	`id` = `_id`
        AND
        	`password` = `_password`;
		
        IF(passwordCheck_ > 0)
        THEN        
    		SELECT
            	`ban`
            INTO
            	`ban_`
            FROM
            	`accounts`
            WHERE
            	`id` = `_id`;
        
        	IF(`ban_` = 0)
        	THEN
        		SELECT
                	`status`
                INTO
                	`status_`
                FROM
                	`accounts`
                WHERE
                	`id` = `_id`;
        	
         		IF(`status_` = 0)
         		THEN
            		UPDATE
                    	`accounts`
                    SET
                    	`status` = 1,
                        `ipAddress` = _ipAddress,
                        `loginDate` = NOW()
                    WHERE
                    	`id` = _id;
                        
            		RETURN 1; -- ok.
           		END IF;
                
                RETURN 3; -- in use.
            END IF;
            
            RETURN 5; -- banned.
        END IF;
        
        RETURN 0; -- invalid password.
    END IF;

  RETURN 2; -- not exists.
END */$$
DELIMITER ;

/* Function  structure for function  `eMU_AccountCreate` */

/*!50003 DROP FUNCTION IF EXISTS `eMU_AccountCreate` */;
DELIMITER $$

/*!50003 CREATE DEFINER=`root`@`localhost` FUNCTION `eMU_AccountCreate`(
        _id VARCHAR(11),
        _password VARCHAR(11),
        _pin INTEGER(4)
    ) RETURNS tinyint(1)
BEGIN
	DECLARE `exists_` TINYINT;
    
   	SELECT
    	COUNT(*)
    INTO
    	`exists_`
    FROM
    	`accounts`
    WHERE
    	`id` = _id;
    
    IF(`exists_` = 0)
    THEN
    	INSERT INTO
        	`accounts`
        		(`id`, `password`, `status`, `ban`, `pin`) VALUES(_id, _password, '0', '0', _pin);
        RETURN 0;
    END IF;
    
  RETURN 1;
END */$$
DELIMITER ;

/* Function  structure for function  `eMU_CharacterCreate` */

/*!50003 DROP FUNCTION IF EXISTS `eMU_CharacterCreate` */;
DELIMITER $$

/*!50003 CREATE DEFINER=`root`@`localhost` FUNCTION `eMU_CharacterCreate`(
        _accountId VARCHAR(11),
        _name VARCHAR(11),
        _race TINYINT
    ) RETURNS tinyint
BEGIN
    DECLARE count_ 		TINYINT;
	DECLARE exists_ 	TINYINT;
    DECLARE strength_	SMALLINT;
    DECLARE agility_	SMALLINT;
    DECLARE vitality_	SMALLINT;
    DECLARE energy_		SMALLINT;
    DECLARE command_	SMALLINT;
    DECLARE health_		FLOAT;
    DECLARE mana_		FLOAT;
    DECLARE mapId_		SMALLINT;
    DECLARE posX_		SMALLINT;
    DECLARE posY_		SMALLINT;
    
    SELECT
    	COUNT(*)
    INTO
    	`count_`
    FROM
    	`characters`
    WHERE
    	`accountId` = `_accountId`;
    
	SELECT
       	COUNT(*)
    INTO
       	`exists_`
    FROM
       	`characters`
    WHERE 
       	`name` = `_name`;
        
    IF(`exists_` = 0)
    THEN           
    	SET `mapId_` = 0;
        SET `posX_` = 182;
        SET `posY_` = 128;
        SET `command_` = 0;
            
        CASE `_race`
	    WHEN 0 THEN
           	SET `strength_` = 18;
            SET `agility_` = 18;
            SET `vitality_` = 15;
            SET `energy_` = 30;
            SET `health_` = 60.0;
            SET `mana_` = 60.0;
                
        WHEN 16 THEN
          	SET `strength_` = 28;
            SET `agility_` = 20;
            SET `vitality_` = 25;
            SET `energy_` = 10;
            SET `health_` = 110.0;
            SET `mana_` = 20.0;
                
        WHEN 32 THEN
          	SET `strength_` = 22;
            SET `agility_` = 25;
            SET `vitality_` = 20;
            SET `energy_` = 15;
            SET `health_` = 80.0;
            SET `mana_` = 30.0;
            SET `mapId_` = 3;
            SET `posX_` = 175;
            SET `posY_` = 110;
                
        WHEN 48 THEN
          	SET `strength_` = 26;
            SET `agility_` = 26;
            SET `vitality_` = 26;
            SET `energy_` = 26;
            SET `health_` = 110.0;
            SET `mana_` = 60.0;
                
        WHEN 64 THEN
          	SET `strength_` = 26;
            SET `agility_` = 20;
            SET `vitality_` = 20;
            SET `energy_` = 15;
            SET `command_` = 25;
            SET `health_` = 90.0;
            SET `mana_` = 40.0;
        ELSE
          	BEGIN
          	END;
        END CASE;
            
        INSERT INTO
	    	`characters` (`accountId`, `name`, `race`,
            				`strength`, `agility`, `vitality`,
                            `energy`, `command`, `health`,
                            `maxHealth`, `mana`, `maxMana`, 
                            `mapId`, `posX`, `posY`) 
        VALUES
           	(`_accountId`, `_name`, `_race`,
          		`strength_`, `agility_`, `vitality_`,
                `energy_`, `command_`, `health_`,
                `health_`, `mana_`, `mana_`,
              	`mapId_`, `posX_`, `posY_`);                     
        RETURN 1;
    END IF;
        
    RETURN 0;
END */$$
DELIMITER ;

/* Function  structure for function  `eMU_CharacterDelete` */

/*!50003 DROP FUNCTION IF EXISTS `eMU_CharacterDelete` */;
DELIMITER $$

/*!50003 CREATE DEFINER=`root`@`localhost` FUNCTION `eMU_CharacterDelete`(
        _accountId VARCHAR(11),
        _name VARCHAR(11),
        _pin VARCHAR(8)
    ) RETURNS tinyint
BEGIN
	DECLARE exists_ TINYINT;
	DECLARE pinCheck_ TINYINT;
    
    SELECT
    	COUNT(*)
    INTO
    	`pinCheck_`
    FROM
    	`accounts`
    WHERE
    	`id` = `_accountId`
    AND
    	`pin` = `_pin`;
        
    SELECT
    	COUNT(*)
    INTO
    	`exists_`
    FROM
    	`characters`
    WHERE
    	`name` = `_name`
    AND
    	`accountId` = `_accountId`;
    
    IF(exists_ > 0)
    THEN
    	IF(pinCheck_ > 0)
        THEN
        	DELETE FROM
	            `characters`
           	WHERE
            	name = `_name`;
        	RETURN 1;
        END IF;
        
        RETURN 2;
    END IF;
    
	RETURN 0;
END */$$
DELIMITER ;

/*!40101 SET SQL_MODE=@OLD_SQL_MODE */;
/*!40014 SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS */;
/*!40014 SET UNIQUE_CHECKS=@OLD_UNIQUE_CHECKS */;
/*!40111 SET SQL_NOTES=@OLD_SQL_NOTES */;
