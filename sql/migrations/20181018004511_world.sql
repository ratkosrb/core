DROP PROCEDURE IF EXISTS add_migration;
delimiter ??
CREATE PROCEDURE `add_migration`()
BEGIN
DECLARE v INT DEFAULT 1;
SET v = (SELECT COUNT(*) FROM `migrations` WHERE `id`='20181018004511');
IF v=0 THEN
INSERT INTO `migrations` VALUES ('20181018004511');
-- Add your query below.


-- CUSTOM CHANGES - NOT BLIZZLIKE


-- Change patch of Soul Shard bags. Original is 1.9.
UPDATE `item_template` SET `patch`=0 WHERE `entry` IN (21371,21342,22244,30063,22243,21340,21341,21358);

-- Change patch of Enchanting and herb bags. Original is 1.10.
UPDATE `item_template` SET `patch`=0 WHERE `entry` IN (22246,22248,22249,22250,22251,22252,22308,22307,22309,22310,22312);

-- Change patch of Ratchet flight master. Originally added in 1.11.
UPDATE `creature_template` SET `patch`=0 WHERE `entry`=16227;
UPDATE `creature` SET `patch_min`=0 WHERE `guid`=14323;

-- Change patch of Un'goro flight master. Originally added in 1.11.
UPDATE `creature_template` SET `patch`=0 WHERE `entry`=10583;
UPDATE `creature` SET `patch_min`=0 WHERE `guid`=23723;

-- Reduce spawn time for Pitted Iron Chest.
UPDATE `gameobject` SET `spawntimesecsmin`=10, `spawntimesecsmax`=10 WHERE `id`=13949;


-- End of migration.
END IF;
END??
delimiter ; 
CALL add_migration();
DROP PROCEDURE IF EXISTS add_migration;
