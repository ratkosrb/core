DROP PROCEDURE IF EXISTS add_migration;
delimiter ??
CREATE PROCEDURE `add_migration`()
BEGIN
DECLARE v INT DEFAULT 1;
SET v = (SELECT COUNT(*) FROM `migrations` WHERE `id`='20200909063758');
IF v=0 THEN
INSERT INTO `migrations` VALUES ('20200909063758');
-- Add your query below.


TRUNCATE `areatrigger_bg_entrance`;
TRUNCATE `battlemaster_entry`;
UPDATE `map_template` SET `linked_zone`=0, `level_min`=0, `level_max`=0, `player_limit`=0, `reset_delay`=0, `ghost_entrance_map`=0, `ghost_entrance_x`=0, `ghost_entrance_y`=0, `map_type`=0, `script_name`='';


-- End of migration.
END IF;
END??
delimiter ; 
CALL add_migration();
DROP PROCEDURE IF EXISTS add_migration;
