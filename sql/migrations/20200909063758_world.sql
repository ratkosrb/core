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
TRUNCATE `battleground_events`;
TRUNCATE `battleground_template`;
TRUNCATE `gameobject_battleground`;
TRUNCATE `creature_battleground`;
TRUNCATE `creature_groups`;
TRUNCATE `creature_linking`;
TRUNCATE `creature_linking_template`;
TRUNCATE `creature_movement`;
TRUNCATE `creature_movement_special`;
TRUNCATE `creature_movement_template`;
TRUNCATE `creature_spells`;
TRUNCATE `pool_creature`;
TRUNCATE `pool_creature_template`;
TRUNCATE `pool_gameobject`;
TRUNCATE `pool_gameobject_template`;
TRUNCATE `pool_pool`;
TRUNCATE `pool_template`;
TRUNCATE `script_waypoint`;
TRUNCATE `game_event`;
TRUNCATE `game_event_creature`;
TRUNCATE `game_event_creature_data`;
TRUNCATE `game_event_gameobject`;
TRUNCATE `game_event_quest`;
TRUNCATE `gameobject_requirement`;
TRUNCATE `scripted_event_id`;
TRUNCATE `scripted_areatrigger`;
TRUNCATE `script_escort_data`;
TRUNCATE `creature_ai_events`;
TRUNCATE `creature_ai_scripts`;
TRUNCATE `creature_movement_scripts`;
TRUNCATE `creature_spells_scripts`;
TRUNCATE `event_scripts`;
TRUNCATE `gameobject_scripts`;
TRUNCATE `generic_scripts`;
TRUNCATE `gossip_scripts`;
TRUNCATE `quest_end_scripts`;
TRUNCATE `quest_start_scripts`;
TRUNCATE `creature_loot_template`;
TRUNCATE `item_loot_template`;
TRUNCATE `reference_loot_template`;
TRUNCATE `gameobject_loot_template`;
TRUNCATE `pickpocketing_loot_template`;
TRUNCATE `skinning_loot_template`;
TRUNCATE `fishing_loot_template`;
UPDATE `gossip_menu_option` SET `action_script_id`=0, `condition_id`=0;
UPDATE `areatrigger_teleport` SET `required_level`=0, `required_condition`=0;
UPDATE `creature_template` SET `loot_id`=0, `skinning_loot_id`=0, `pickpocket_loot_id`=0, `spell_list_id`=0, `movement_type`=0, `regeneration`=0, `ai_name`='NullAI', `script_name`='';
UPDATE `gameobject_template` SET `script_name`='';
UPDATE `quest_template` SET `StartScript`=0, `CompleteScript`=0;
UPDATE `map_template` SET `linked_zone`=0, `level_min`=0, `level_max`=0, `player_limit`=0, `reset_delay`=0, `ghost_entrance_map`=0, `ghost_entrance_x`=0, `ghost_entrance_y`=0, `map_type`=0, `script_name`='';


-- End of migration.
END IF;
END??
delimiter ; 
CALL add_migration();
DROP PROCEDURE IF EXISTS add_migration;
