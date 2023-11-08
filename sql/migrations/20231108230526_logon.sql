DROP PROCEDURE IF EXISTS add_migration;
delimiter ??
CREATE PROCEDURE `add_migration`()
BEGIN
DECLARE v INT DEFAULT 1;
SET v = (SELECT COUNT(*) FROM `migrations` WHERE `id`='20231108230526');
IF v=0 THEN
INSERT INTO `migrations` VALUES ('20231108230526');
-- Add your query below.


CREATE TABLE `last_played_character` (
	`account_id` INT UNSIGNED NOT NULL,
	`realm_id` INT UNSIGNED NOT NULL,
	`character_name` VARCHAR(16) NOT NULL,
	`character_guid` BIGINT UNSIGNED NOT NULL,
	`last_played_time` INT UNSIGNED NOT NULL,
	PRIMARY KEY (`account_id`, `realm_id`)
)
COLLATE='utf8_unicode_ci'
ENGINE=InnoDB
;


-- End of migration.
END IF;
END??
delimiter ; 
CALL add_migration();
DROP PROCEDURE IF EXISTS add_migration;
