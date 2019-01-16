DROP PROCEDURE IF EXISTS add_migration;
delimiter ??
CREATE PROCEDURE `add_migration`()
BEGIN
DECLARE v INT DEFAULT 1;
SET v = (SELECT COUNT(*) FROM `migrations` WHERE `id`='20190116084833');
IF v=0 THEN
INSERT INTO `migrations` VALUES ('20190116084833');
-- Add your query below.


-- CUSTOM CHANGES - NOT BLIZZLIKE


-- Change patch of Ratchet flight master. Originally added in 1.11.
UPDATE `creature_template` SET `patch`=0 WHERE `entry`=16227;
UPDATE `creature` SET `patch_min`=0 WHERE `guid`=14323;

-- Change patch of Un'goro flight master. Originally added in 1.11.
UPDATE `creature_template` SET `patch`=0 WHERE `entry`=10583;
UPDATE `creature` SET `patch_min`=0 WHERE `guid`=23723;

-- Reduce spawn time for Pitted Iron Chest.
UPDATE `gameobject` SET `spawntimesecsmin`=10, `spawntimesecsmax`=10 WHERE `id`=13949;

-- Make beta mounts temporary items.
UPDATE `item_template` SET `duration`=604800 WHERE `entry` IN (1133, 2415);

-- Change patch for TCG rewards.
UPDATE `item_template` SET `patch`=0 WHERE `entry` IN (23705, 23709, 23164);


-- End of migration.
END IF;
END??
delimiter ; 
CALL add_migration();
DROP PROCEDURE IF EXISTS add_migration;
