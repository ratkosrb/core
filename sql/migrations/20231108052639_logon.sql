DROP PROCEDURE IF EXISTS add_migration;
delimiter ??
CREATE PROCEDURE `add_migration`()
BEGIN
DECLARE v INT DEFAULT 1;
SET v = (SELECT COUNT(*) FROM `migrations` WHERE `id`='20231108052639');
IF v=0 THEN
INSERT INTO `migrations` VALUES ('20231108052639');
-- Add your query below.


-- Add bnet login ticket to db.
ALTER TABLE `account`
	CHANGE COLUMN `token_key` `login_ticket` VARCHAR(64) NULL AFTER `s`,
	ADD COLUMN `login_ticket_expiry` INT UNSIGNED NOT NULL DEFAULT '0' AFTER `login_ticket`;


-- End of migration.
END IF;
END??
delimiter ; 
CALL add_migration();
DROP PROCEDURE IF EXISTS add_migration;
