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
TRUNCATE `creature_linking`;
TRUNCATE `creature_linking_template`;
TRUNCATE `creature_movement`;
TRUNCATE `pool_creature`;
TRUNCATE `pool_creature_template`;
TRUNCATE `pool_gameobject`;
TRUNCATE `pool_gameobject_template`;
TRUNCATE `pool_pool`;
TRUNCATE `pool_template`;
TRUNCATE `script_waypoint`;
TRUNCATE `game_event_creature`;
TRUNCATE `game_event_creature_data`;
TRUNCATE `game_event_gameobject`
DELETE FROM `creature_ai_scripts` WHERE `target_type` IN (9, 12);
DELETE FROM `creature_movement_scripts` WHERE `target_type` IN (9, 12);
DELETE FROM `creature_spells_scripts` WHERE `target_type` IN (9, 12);
DELETE FROM `event_scripts` WHERE `target_type` IN (9, 12);
DELETE FROM `gameobject_scripts` WHERE `target_type` IN (9, 12);
DELETE FROM `generic_scripts` WHERE `target_type` IN (9, 12);
DELETE FROM `gossip_scripts` WHERE `target_type` IN (9, 12);
DELETE FROM `quest_end_scripts` WHERE `target_type` IN (9, 12);
DELETE FROM `quest_start_scripts` WHERE `target_type` IN (9, 12);
UPDATE `creature_template` SET `movement_type`=0, `ai_name`='NullAI', `script_name`='';
UPDATE `map_template` SET `linked_zone`=0, `level_min`=0, `level_max`=0, `player_limit`=0, `reset_delay`=0, `ghost_entrance_map`=0, `ghost_entrance_x`=0, `ghost_entrance_y`=0, `map_type`=0, `script_name`='';


-- End of migration.
END IF;
END??
delimiter ; 
CALL add_migration();
DROP PROCEDURE IF EXISTS add_migration;
